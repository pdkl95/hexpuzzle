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
#include "options.h"
#include "shader.h"

#include "data_shaders.h"

Shader win_border_shader;
win_border_shader_loc_t win_border_shader_loc;
char *win_border_shader_src;

Shader postprocessing_shader;
postprocessing_shader_loc_t postprocessing_shader_loc;
char *postprocessing_shader_src;

Shader background_shader;
background_shader_loc_t background_shader_loc;
char *background_shader_vert_src;
char *background_shader_frag_src;

void load_shaders(void)
{
    win_border_shader_src = strdup_xxd_include(
        shaders_win_border_frag_glsl,
        shaders_win_border_frag_glsl_len);

    if (options->extra_rainbows) {
        char *orig_src = win_border_shader_src;
        char *version_line = strsep(&orig_src, "\n");;
        win_border_shader_src = NULL;
        safe_asprintf(&win_border_shader_src,
                      "%s\n#define EXTRA_RAINBOW 1\n\n%s",
                      version_line,
                      orig_src);
        free(version_line);
    }

    win_border_shader = LoadShaderFromMemory(0, win_border_shader_src);
    win_border_shader_loc.resolution     = GetShaderLocation(win_border_shader, "resolution");
    win_border_shader_loc.time           = GetShaderLocation(win_border_shader, "time");
    win_border_shader_loc.fade           = GetShaderLocation(win_border_shader, "fade");
    win_border_shader_loc.effect_amount1 = GetShaderLocation(win_border_shader, "effect_amount1");
    win_border_shader_loc.effect_amount2 = GetShaderLocation(win_border_shader, "effect_amount2");

    postprocessing_shader_src = strdup_xxd_include(
        shaders_postprocessing_frag_glsl,
        shaders_postprocessing_frag_glsl_len);

    postprocessing_shader = LoadShaderFromMemory(0, postprocessing_shader_src);
    postprocessing_shader_loc.resolution     = GetShaderLocation(postprocessing_shader, "resolution");
    postprocessing_shader_loc.time           = GetShaderLocation(postprocessing_shader, "time");
    postprocessing_shader_loc.effect_amount1 = GetShaderLocation(postprocessing_shader, "effect_amount1");
    postprocessing_shader_loc.effect_amount2 = GetShaderLocation(postprocessing_shader, "effect_amount2");

    background_shader_vert_src = strdup_xxd_include(
        shaders_background_vert_glsl,
        shaders_background_vert_glsl_len);

    background_shader_frag_src = strdup_xxd_include(
        shaders_background_frag_glsl,
        shaders_background_frag_glsl_len);

    background_shader = LoadShaderFromMemory(background_shader_vert_src, background_shader_frag_src);
    background_shader_loc.resolution     = GetShaderLocation(background_shader, "resolution");
    background_shader_loc.time           = GetShaderLocation(background_shader, "time");
    background_shader_loc.warp           = GetShaderLocation(background_shader, "warp");
    background_shader_loc.effect_amount1 = GetShaderLocation(background_shader, "effect_amount1");
    background_shader_loc.effect_amount2 = GetShaderLocation(background_shader, "effect_amount2");
}

void unload_shaders(void)
{
    UnloadShader(win_border_shader);
    FREE(win_border_shader_src);

    UnloadShader(postprocessing_shader);
    FREE(postprocessing_shader_src);

    UnloadShader(background_shader);
    FREE(background_shader_vert_src);
    FREE(background_shader_frag_src);
}
