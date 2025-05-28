/****************************************************************************
 *                                                                          *
 * shader.h                                                                 *
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

#ifndef SHADER_H
#define SHADER_H

extern Shader win_border_shader;
typedef struct win_border_shader_loc {
    int resolution;
    int time;
    int fade;
    int effect_amount1;
    int effect_amount2;
} win_border_shader_loc_t;
extern win_border_shader_loc_t win_border_shader_loc;

extern Shader postprocessing_shader;
typedef struct postprocessing_shader_loc {
    int resolution;
    int time;
    int effect_amount1;
    int effect_amount2;
} postprocessing_shader_loc_t;
extern postprocessing_shader_loc_t postprocessing_shader_loc;

extern Shader background_shader;
typedef struct background_shader_loc {
    int resolution;
    int time;
    int warp;
    int effect_amount1;
    int effect_amount2;
} background_shader_loc_t;
extern background_shader_loc_t background_shader_loc;

void load_shaders(void);
void unload_shaders(void);

#endif /*SHADER_H*/

