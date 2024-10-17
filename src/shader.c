/****************************************************************************
 *                                                                          *
 * shader.c                                                                 *
 *                                                                          *
 * This file is part of hexpuzzle.                                          *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.             *
 *                                                                          *
 ****************************************************************************/

#include "common.h"
#include "shader.h"

#include "data_shaders.h"

Shader win_border_shader;
win_border_shader_loc_t win_border_shader_loc;
char *win_border_shader_src;

void load_shaders(void)
{
    win_border_shader_src = strdup_xxd_include(
        shaders_win_border_frag_glsl,
        shaders_win_border_frag_glsl_len);

    win_border_shader = LoadShaderFromMemory(0, win_border_shader_src);
    win_border_shader_loc.resolution = GetShaderLocation(win_border_shader, "resolution");
    win_border_shader_loc.time       = GetShaderLocation(win_border_shader, "time");
    win_border_shader_loc.fade       = GetShaderLocation(win_border_shader, "fade");
}

void unload_shaders(void)
{
    UnloadShader(win_border_shader);
    free(win_border_shader_src);
}
