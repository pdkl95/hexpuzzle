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


Shader win_border_shader;
win_border_shader_loc_t win_border_shader_loc;

void load_shaders(void)
{
    win_border_shader = LoadShader(0, "shaders/win_border.frag.glsl");
    win_border_shader_loc.resolution = GetShaderLocation(win_border_shader, "resolution");
    win_border_shader_loc.time       = GetShaderLocation(win_border_shader, "time");
    win_border_shader_loc.fade       = GetShaderLocation(win_border_shader, "fade");
}

void unload_shaders(void)
{
    UnloadShader(win_border_shader);
}
