/* SPDX-FileCopyrightText: 2024 Blender Foundation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <numeric>

#include "BLI_listbase.h"
#include "BLI_string.h"
#include "BLI_string_utf8.h"

#include "RNA_enum_types.hh"

#include "UI_interface.hh"
#include "UI_resources.hh"

#include "NOD_rna_define.hh"
#include "NOD_socket_search_link.hh"

#include "node_function_util.hh"

namespace blender::nodes::node_fn_integer_math_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.is_function_node();
  b.add_input<decl::Int>("Value");
  b.add_input<decl::Int>("Value", "Value_001");
  b.add_input<decl::Int>("Value", "Value_002");
  b.add_output<decl::Int>("Value");
};

static void node_layout(uiLayout *layout, bContext * /*C*/, PointerRNA *ptr)
{
  uiItemR(layout, ptr, "operation", UI_ITEM_NONE, "", ICON_NONE);
}

static void node_update(bNodeTree *ntree, bNode *node)
{
  const bool one_input_ops = ELEM(
      node->custom1, NODE_INTEGER_MATH_ABSOLUTE, NODE_INTEGER_MATH_SIGN, NODE_INTEGER_MATH_NEGATE);
  const bool three_input_ops = ELEM(node->custom1, NODE_INTEGER_MATH_MULTIPLY_ADD);

  bNodeSocket *sockA = static_cast<bNodeSocket *>(node->inputs.first);
  bNodeSocket *sockB = sockA->next;
  bNodeSocket *sockC = sockB->next;

  bke::node_set_socket_availability(ntree, sockB, !one_input_ops);
  bke::node_set_socket_availability(ntree, sockC, three_input_ops);

  node_sock_label_clear(sockA);
  node_sock_label_clear(sockB);
  node_sock_label_clear(sockC);
  switch (node->custom1) {
    case NODE_INTEGER_MATH_MULTIPLY_ADD:
      node_sock_label(sockA, N_("Value"));
      node_sock_label(sockB, N_("Multiplier"));
      node_sock_label(sockC, N_("Addend"));
      break;
  }
}

class SocketSearchOp {
 public:
  std::string socket_name;
  NodeIntegerMathOperation operation;
  void operator()(LinkSearchOpParams &params)
  {
    bNode &node = params.add_node("FunctionNodeIntegerMath");
    node.custom1 = NodeIntegerMathOperation(operation);
    params.update_and_connect_available_socket(node, socket_name);
  }
};

static void node_gather_link_searches(GatherLinkSearchOpParams &params)
{
  if (!params.node_tree().typeinfo->validate_link(eNodeSocketDatatype(params.other_socket().type),
                                                  SOCK_INT))
  {
    return;
  }

  const bool is_integer = params.other_socket().type == SOCK_INT;
  const int weight = is_integer ? 0 : -1;

  /* Add socket A operations. */
  for (const EnumPropertyItem *item = rna_enum_node_integer_math_items;
       item->identifier != nullptr;
       item++)
  {
    if (item->name != nullptr && item->identifier[0] != '\0') {
      params.add_item(IFACE_(item->name),
                      SocketSearchOp{"Value", NodeIntegerMathOperation(item->value)},
                      weight);
    }
  }
}

static void node_label(const bNodeTree * /*ntree*/, const bNode *node, char *label, int maxlen)
{
  const char *name;
  bool enum_label = RNA_enum_name(rna_enum_node_integer_math_items, node->custom1, &name);
  if (!enum_label) {
    name = "Unknown";
  }
  BLI_strncpy(label, IFACE_(name), maxlen);
}

static const mf::MultiFunction *get_multi_function(const bNode &bnode)
{
  NodeIntegerMathOperation operation = NodeIntegerMathOperation(bnode.custom1);
  static auto exec_preset = mf::build::exec_presets::AllSpanOrSingle();
  static auto add_fn = mf::build::SI2_SO<int, int, int>(
      "Add", [](int a, int b) { return a + b; }, exec_preset);
  static auto sub_fn = mf::build::SI2_SO<int, int, int>(
      "Subtract", [](int a, int b) { return a - b; }, exec_preset);
  static auto multiply_fn = mf::build::SI2_SO<int, int, int>(
      "Multiply", [](int a, int b) { return a * b; }, exec_preset);
  static auto divide_fn = mf::build::SI2_SO<int, int, int>(
      "Divide", [](int a, int b) { return math::safe_divide(a, b); }, exec_preset);
  static auto divide_floor_fn = mf::build::SI2_SO<int, int, int>(
      "Divide Floor",
      [](int a, int b) { return int(math::floor(math::safe_divide(float(a), float(b)))); },
      exec_preset);
  static auto divide_ceil_fn = mf::build::SI2_SO<int, int, int>(
      "Divide Ceil",
      [](int a, int b) { return int(math::ceil(math::safe_divide(float(a), float(b)))); },
      exec_preset);
  static auto divide_round_fn = mf::build::SI2_SO<int, int, int>(
      "Divide Round",
      [](int a, int b) { return int(math::round(math::safe_divide(float(a), float(b)))); },
      exec_preset);
  static auto pow_fn = mf::build::SI2_SO<int, int, int>(
      "Power", [](int a, int b) { return math::pow(a, b); }, exec_preset);
  static auto madd_fn = mf::build::SI3_SO<int, int, int, int>(
      "Multiply Add", [](int a, int b, int c) { return a * b + c; }, exec_preset);
  static auto floored_mod_fn = mf::build::SI2_SO<int, int, int>(
      "Floored Modulo",
      [](int a, int b) { return b != 0 ? math::mod_periodic(a, b) : 0; },
      exec_preset);
  static auto mod_fn = mf::build::SI2_SO<int, int, int>(
      "Modulo", [](int a, int b) { return b != 0 ? a % b : 0; }, exec_preset);
  static auto abs_fn = mf::build::SI1_SO<int, int>(
      "Absolute", [](int a) { return math::abs(a); }, exec_preset);
  static auto sign_fn = mf::build::SI1_SO<int, int>(
      "Sign", [](int a) { return math::sign(a); }, exec_preset);
  static auto min_fn = mf::build::SI2_SO<int, int, int>(
      "Minimum", [](int a, int b) { return math::min(a, b); }, exec_preset);
  static auto max_fn = mf::build::SI2_SO<int, int, int>(
      "Maximum", [](int a, int b) { return math::max(a, b); }, exec_preset);
  static auto gcd_fn = mf::build::SI2_SO<int, int, int>(
      "GCD", [](int a, int b) { return std::gcd(a, b); }, exec_preset);
  static auto lcm_fn = mf::build::SI2_SO<int, int, int>(
      "LCM", [](int a, int b) { return std::lcm(a, b); }, exec_preset);
  static auto negate_fn = mf::build::SI1_SO<int, int>(
      "Negate", [](int a) { return -a; }, exec_preset);

  switch (operation) {
    case NODE_INTEGER_MATH_ADD:
      return &add_fn;
    case NODE_INTEGER_MATH_SUBTRACT:
      return &sub_fn;
    case NODE_INTEGER_MATH_MULTIPLY:
      return &multiply_fn;
    case NODE_INTEGER_MATH_DIVIDE:
      return &divide_fn;
    case NODE_INTEGER_MATH_DIVIDE_FLOOR:
      return &divide_floor_fn;
    case NODE_INTEGER_MATH_DIVIDE_CEIL:
      return &divide_ceil_fn;
    case NODE_INTEGER_MATH_DIVIDE_ROUND:
      return &divide_round_fn;
    case NODE_INTEGER_MATH_POWER:
      return &pow_fn;
    case NODE_INTEGER_MATH_MULTIPLY_ADD:
      return &madd_fn;
    case NODE_INTEGER_MATH_FLOORED_MODULO:
      return &floored_mod_fn;
    case NODE_INTEGER_MATH_MODULO:
      return &mod_fn;
    case NODE_INTEGER_MATH_ABSOLUTE:
      return &abs_fn;
    case NODE_INTEGER_MATH_SIGN:
      return &sign_fn;
    case NODE_INTEGER_MATH_MINIMUM:
      return &min_fn;
    case NODE_INTEGER_MATH_MAXIMUM:
      return &max_fn;
    case NODE_INTEGER_MATH_GCD:
      return &gcd_fn;
    case NODE_INTEGER_MATH_LCM:
      return &lcm_fn;
    case NODE_INTEGER_MATH_NEGATE:
      return &negate_fn;
  }
  BLI_assert_unreachable();
  return nullptr;
}

static void node_build_multi_function(NodeMultiFunctionBuilder &builder)
{
  const mf::MultiFunction *fn = get_multi_function(builder.node());
  builder.set_matching_fn(fn);
}

static void node_rna(StructRNA *srna)
{
  PropertyRNA *prop;

  prop = RNA_def_node_enum(srna,
                           "operation",
                           "Operation",
                           "",
                           rna_enum_node_integer_math_items,
                           NOD_inline_enum_accessors(custom1),
                           NODE_INTEGER_MATH_ADD);
  RNA_def_property_update_runtime(prop, rna_Node_socket_update);
}

static void node_register()
{
  static blender::bke::bNodeType ntype;

  fn_node_type_base(&ntype, FN_NODE_INTEGER_MATH, "Integer Math", NODE_CLASS_CONVERTER);
  ntype.declare = node_declare;
  ntype.labelfunc = node_label;
  ntype.updatefunc = node_update;
  ntype.build_multi_function = node_build_multi_function;
  ntype.draw_buttons = node_layout;
  ntype.gather_link_search_ops = node_gather_link_searches;

  blender::bke::node_register_type(&ntype);

  node_rna(ntype.rna_ext.srna);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_fn_integer_math_cc