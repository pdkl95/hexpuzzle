/****************************************************************************
 *                                                                          *
 * raylib_helper.h                                                          *
 *                                                                          *
 * This file is part of hexpuzzle                                           *
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

#ifndef RAYLIB_HELPER_H
#define RAYLIB_HELPER_H

void printrect(Rectangle r);
void printcolorhex(Color c);
void printcolor(Color c);

/*** Constants ***/

#define VEC2_ZERO    ((Vector2){ 0.0f, 0.0f })
#define VEC2_ORIGIN  VEC2_ZERO

/*** Colors ***/

extern Color TEXT_LIGHT_SHADOW;
extern Color TEXT_DARK_SHADOW;

void DrawTextWindowCenter(const char *text, int posY, int fontSize, Color color);

void DrawTextShadow(const char *text, int posX, int posY, int fontSize, Color color);
void DrawTextDropShadow(const char *text, int posX, int posY, int fontSize, Color fgcolor, Color bgcolor);

Color ColorLerp(Color start, Color end, float amount);

/*** Misc Math ***/

float nsinf(float x);

/*** Vector Math ***/

Vector2 Vector2Project(Vector2 v, Vector2 vproj);

typedef struct Poly4 {
    Vector2 vert[4];
} Poly4;

Poly4 RotatedRectToPoly4(Rectangle rect, float angle);
bool CheckCollisionPointRotatedRec(Vector2 p, Rectangle rect, float angle);

static inline Vector2 Vector2Min(Vector2 a, Vector2 b)
{
    Vector2 result = {0};
    result.x = fmin(a.x, b.x);
    result.y = fmin(a.y, b.y);
    return result;
}

static inline Vector2 Vector2Max(Vector2 a, Vector2 b)
{
    Vector2 result = {0};
    result.x = fmax(a.x, b.x);
    result.y = fmax(a.y, b.y);
    return result;
}

Vector2 Vector2RotateAroundPoint(Vector2 v, float angle, Vector2 p);
Rectangle BoundingBoxRotatedRectangle(Rectangle rect, float angle);
Vector2 RectangleCenter(Rectangle rect);

/*** Double Buffered Textures ***/

struct doublebuffer {
    RenderTexture2D buf1, buf2;
    RenderTexture2D *cur, *prev;
};
typedef struct doublebuffer doublebuffer_t;

void doublebuffer_resize(doublebuffer_t *db, int width, int height);
void doublebuffer_load(doublebuffer_t *db, int width, int height);
void doublebuffer_load_with_format(doublebuffer_t *db, int width, int height, int pixel_format);
void doublebuffer_unload(doublebuffer_t *db);
void doublebuffer_swap_buffers(doublebuffer_t *db);

/*** Other Texture Stuff ***/

Texture2D LoadTextureWithFormat(int width, int height, int format);
RenderTexture2D LoadRenderTextureWithFormat(int width, int height, int pixel_format);


#endif /*RAYLIB_HELPER_H*/

