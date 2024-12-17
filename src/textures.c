/****************************************************************************
 *                                                                          *
 * textures.c                                                               *
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
#include "textures.h"
#include "data_textures.h"

Texture2D bg_grid_texture;

#define load_tex(name) \
    Image name##_img = LoadImageFromMemory(".png", textures_##name##_png, textures_##name##_png_len); \
    name##_texture = LoadTextureFromImage(name##_img); \
    UnloadImage(name##_img);

void load_textures(void)
{
    load_tex(bg_grid);
    SetTextureWrap(bg_grid_texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(bg_grid_texture, TEXTURE_FILTER_BILINEAR);
                     //TEXTURE_FILTER_ANISOTROPIC_8X);
    GenTextureMipmaps(&bg_grid_texture);
}

void unload_textures(void)
{
    UnloadTexture(bg_grid_texture);
}
