/****************************************************************************
 *                                                                          *
 * raylib_helper.c                                                          *
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
#include "raylib_helper.h"

Color TEXT_LIGHT_SHADOW = { 0x11, 0x11, 0x11, 0x33 };
Color TEXT_DARK_SHADOW  = {    0,    0,    0, 0xAA };

void DrawTextWindowCenter(const char *text, int posY, int fontSize, Color color)
{
    Font font = GuiGetFont();
    Vector2 text_size = MeasureTextEx(font, text, (float)fontSize, 1.0);
    int margin_left = (window_size.x - text_size.x) / 2;
    Vector2 pos = {
        .x = margin_left,
        .y = posY
    };
    DrawTextEx(font, text, pos, fontSize, 1.0, color);
}

void DrawTextShadow(const char *text, int posX, int posY, int fontSize, Color color)
{
    Font font = GuiGetFont();
    Vector2 pos = {
        .x = posX + 2,
        .y = posY + 2
    };
    DrawTextEx(font, text, pos, fontSize, 1.0, TEXT_LIGHT_SHADOW);
    pos.x -= 1;
    pos.y -= 1;
    DrawTextEx(font, text, pos, fontSize, 1.0, TEXT_DARK_SHADOW);
    pos.x -= 1;
    pos.y -= 1;
    DrawTextEx(font, text, pos, fontSize, 1.0, color);
}

void DrawTextDropShadow(const char *text, int posX, int posY, int fontSize, Color fgcolor, Color bgcolor)
{
    Font font = GuiGetFont();
    Vector2 shadow_pos = { .x = posX + 1, .y = posY + 1 };
    Vector2        pos = { .x = posX,     .y = posY     };
    DrawTextEx(font, text, shadow_pos, fontSize, 1.0, bgcolor);
    DrawTextEx(font, text,        pos, fontSize, 1.0, fgcolor);
}

Color ColorLerp(Color start, Color end, float amount)
{
    Color result = {
        .r = start.r + (unsigned char)(amount * (float)(end.r - start.r)),
        .g = start.g + (unsigned char)(amount * (float)(end.g - start.g)),
        .b = start.b + (unsigned char)(amount * (float)(end.b - start.b)),
        .a = start.a + (unsigned char)(amount * (float)(end.a - start.a)),
    };

    return result;
}

bool ColorEq(Color a, Color b)
{
    return (
        (a.r == b.r) &&
        (a.g == b.g) &&
        (a.b == b.b) &&
        (a.a == b.a)
    );
}

void printrect(Rectangle r)
{
    printf("Rect: x = %f, y = %f\n", r.x, r.y);
    printf("      w = %f, h = %f\n", r.width, r.height);
}

void printcolorhex(Color c)
{
    printf("%02x%02x%02x%02x", c.r, c.g, c.b, c.a);
}
void printcolor(Color c)
{
    printf("Color{ #");
    printcolorhex(c);
    printf(" }");
}

float nsinf(float x)
{
    float value = sinf(x);
    value += 1.0;
    value *= 0.5;
    return value;
}

static RenderTexture2D create_and_setup_render_texture_2d(int width, int height)
{
    RenderTexture2D rt = LoadRenderTexture(width, height);

    //GenTextureMipmaps(&rt.texture);
    //SetTextureFilter(rt.texture,  TEXTURE_FILTER_TRILINEAR);

    BeginTextureMode(rt);
    {
        ClearBackground(BLACK);
    }
    EndTextureMode();

    return rt;
}

static RenderTexture2D create_and_setup_render_texture_2d_with_format(int width, int height, int pixel_format)
{
    RenderTexture2D rt = LoadRenderTextureWithFormat(width, height, pixel_format);

    SetTextureFilter(rt.texture,  TEXTURE_FILTER_POINT);
    //SetTextureWrap(rt.texture, TEXTURE_WRAP_CLAMP);
    GenTextureMipmaps(&rt.texture);

    BeginTextureMode(rt);
    {
        ClearBackground(BLACK);
    }
    EndTextureMode();

    return rt;
}

Vector2 Vector2Project(Vector2 v, Vector2 vproj)
{
    float dotprod = Vector2DotProduct(v, vproj);
    return Vector2Scale(vproj, dotprod);
}

Vector2 Vector2RotateAroundPoint(Vector2 v, float angle, Vector2 p)
{
    float s = sinf(angle);
    float c = cosf(angle);

    Vector2 vv = Vector2Subtract(v, p);

    Vector2 v_rot;
    v_rot.x = vv.x * c - vv.y * s;
    v_rot.y = vv.x * s + vv.y * c;

    return Vector2Add(v_rot, p);
}

Poly4 RotatedRectToPoly4(Rectangle rect, float angle)
{
    Poly4 poly;
    Vector2 v;

    v.x = rect.x;
    v.y = rect.y;
    poly.vert[0] = Vector2Rotate(v, angle);

    v.x = rect.x + rect.width;
    v.y = rect.y;
    poly.vert[1] = Vector2Rotate(v, angle);

    v.x = rect.x + rect.width;
    v.y = rect.y + rect.height;
    poly.vert[2] = Vector2Rotate(v, angle);

    v.x = rect.x;
    v.y = rect.y + rect.height;
    poly.vert[3] = Vector2Rotate(v, angle);

    return poly;
}

Rectangle BoundingBoxRotatedRectangle(Rectangle rect, float angle)
{
    Vector2 r;

    r.x = rect.x;
    r.y = rect.y;
    Vector2 c1 = Vector2Rotate(r, angle);

    r.x = rect.x + rect.width;
    r.y = rect.y;

    Vector2 c2 = Vector2Rotate(r, angle);

    r.x = rect.x;
    r.y = rect.y + rect.height;
    Vector2 c3 = Vector2Rotate(r, angle);

    r.x = rect.x + rect.width;
    r.y = rect.y + rect.height;
    Vector2 c4 = Vector2Rotate(r, angle);


    Vector2 min, max;
    min = Vector2Min(c1, Vector2Min(c2, Vector2Min(c3, c4)));
    max = Vector2Max(c1, Vector2Max(c2, Vector2Max(c3, c4)));

    Rectangle result;
    result.x = min.x;
    result.y = min.y;
    result.width  = max.x - min.x;
    result.height = max.y - min.y;
    return result;
}

bool CheckCollisionPointRotatedRec(Vector2 p, Rectangle rect, float angle)
{
    Poly4 poly = RotatedRectToPoly4(rect, angle);
    return CheckCollisionPointPoly(p, poly.vert, 4);
}

Vector2 RectangleCenter(Rectangle rect)
{
    Vector2 v;
    v.x = rect.x + (0.5 * rect.width);
    v.y = rect.y + (0.5 * rect.height);
    return v;
}

void doublebuffer_load(doublebuffer_t *db, int width, int height)
{
    doublebuffer_unload(db);
    db->buf1 = create_and_setup_render_texture_2d(width, height);
    db->buf2 = create_and_setup_render_texture_2d(width, height);
    db->cur  = &db->buf1;
    db->prev = &db->buf2;
}

void doublebuffer_load_with_format(doublebuffer_t *db, int width, int height, int pixel_format)
{
    doublebuffer_unload(db);
    db->buf1 = create_and_setup_render_texture_2d_with_format(width, height, pixel_format);
    db->buf2 = create_and_setup_render_texture_2d_with_format(width, height, pixel_format);
    db->cur  = &db->buf1;
    db->prev = &db->buf2;
}

void doublebuffer_unload(doublebuffer_t *db)
{
    if (IsRenderTextureReady(db->buf1)) {
        UnloadRenderTexture(db->buf1);
    }

    if (IsRenderTextureReady(db->buf2)) {
        UnloadRenderTexture(db->buf2);
    }

    db->cur = db->prev = NULL;
}

void doublebuffer_resize(doublebuffer_t *db, int width, int height)
{
    doublebuffer_unload(db);
    doublebuffer_load(db, width, height);
}

void doublebuffer_swap_buffers(doublebuffer_t *db)
{
   RenderTexture2D *tmp = db->cur;
    db->cur = db->prev;
    db->prev = tmp;
}

Texture2D
LoadTextureWithFormat(
    int width,
    int height,
    int format
) {
    Texture2D texture = { 0 };
    int mipmaps = 1;

    if ((width != 0) && (height != 0))
    {
        texture.id = rlLoadTexture(NULL, width, height, format, mipmaps);
    }
    else TRACELOG(LOG_WARNING, "IMAGE: Texture width/height cannot be 0");

    texture.width = width;
    texture.height = height;
    texture.mipmaps = mipmaps;
    texture.format = format;

    return texture;
}

RenderTexture2D
LoadRenderTextureWithFormat(
    int width,
    int height,
    int pixel_format
) {
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.texture.id = rlLoadTexture(NULL, width, height, pixel_format, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = pixel_format;
        target.texture.mipmaps = 1;

        // Create depth renderbuffer/texture
        target.depth.id = rlLoadTextureDepth(width, height, true);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) {
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully with PixelFormat %d", target.id, pixel_format);
        } else {
            TRACELOG(LOG_WARNING, "FBO: [ID %i] Framebuffer object is NOT ready! (PixelFormat %d)", target.id, pixel_format);
        }

        rlDisableFramebuffer();
    } else {
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");
    }

    return target;
}
