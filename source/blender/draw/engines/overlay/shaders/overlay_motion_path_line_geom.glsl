/* SPDX-FileCopyrightText: 2018-2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "common_view_clipping_lib.glsl"
#include "common_view_lib.glsl"

vec2 compute_dir(vec2 v0, vec2 v1)
{
  vec2 dir = normalize(v1 - v0 + 1e-8);
  dir = vec2(-dir.y, dir.x);
  return dir;
}

void main()
{
  vec2 t;
  vec2 edge_dir = compute_dir(interp_flat[0].ss_pos, interp_flat[1].ss_pos) * sizeViewportInv;

  bool is_persp = (drw_view.winmat[3][3] == 0.0);
  float line_size = float(lineThickness) * sizePixel;

  view_clipping_distances_set(gl_in[0]);
  interp_out.color = interp_in[0].color;
  t = edge_dir * (line_size * (is_persp ? gl_in[0].gl_Position.w : 1.0));
  gl_Position = gl_in[0].gl_Position + vec4(t, 0.0, 0.0);
  gpu_EmitVertex();
  gl_Position = gl_in[0].gl_Position - vec4(t, 0.0, 0.0);
  gpu_EmitVertex();

  view_clipping_distances_set(gl_in[1]);
  interp_out.color = interp_in[1].color;
  t = edge_dir * (line_size * (is_persp ? gl_in[1].gl_Position.w : 1.0));
  gl_Position = gl_in[1].gl_Position + vec4(t, 0.0, 0.0);
  gpu_EmitVertex();
  gl_Position = gl_in[1].gl_Position - vec4(t, 0.0, 0.0);
  gpu_EmitVertex();
  EndPrimitive();
}
