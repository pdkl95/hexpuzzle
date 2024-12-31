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
#include "raygui/raygui.h"
#include "raylib_helper.h"

typedef enum { BORDER = 0, BASE, TEXT, OTHER } GuiPropertyElement;

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

static Color GuiFade(Color color, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;

    Color result = { color.r, color.g, color.b, (unsigned char)(color.a*alpha) };

    return result;
}

static void GuiDrawRectangle(Rectangle rec, int borderWidth, Color borderColor, Color color)
{
    if (color.a > 0)
    {
        // Draw rectangle filled with color
        DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height, GuiFade(color, GuiGetAlpha()));
    }

    if (borderWidth > 0)
    {
        float alpha = GuiGetAlpha();
        // Draw rectangle border lines with color
        DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, borderWidth, GuiFade(borderColor, alpha));
        DrawRectangle((int)rec.x, (int)rec.y + borderWidth, borderWidth, (int)rec.height - 2*borderWidth, GuiFade(borderColor, alpha));
        DrawRectangle((int)rec.x + (int)rec.width - borderWidth, (int)rec.y + borderWidth, borderWidth, (int)rec.height - 2*borderWidth, GuiFade(borderColor, alpha));
        DrawRectangle((int)rec.x, (int)rec.y + (int)rec.height - borderWidth, (int)rec.width, borderWidth, GuiFade(borderColor, alpha));
    }
}

void GuiSimpleTabBar(Rectangle bounds, const char **text, int count, int *active)
{
#define RAYGUI_TABBAR_ITEM_WIDTH    120

    Rectangle tabBounds = { bounds.x, bounds.y, RAYGUI_TABBAR_ITEM_WIDTH, bounds.height };

    if (*active < 0) *active = 0;
    else if (*active > count - 1) *active = count - 1;

    int offsetX = 0;    // Required in case tabs go out of screen
    offsetX = (*active + 2)*RAYGUI_TABBAR_ITEM_WIDTH - GetScreenWidth();
    if (offsetX < 0) offsetX = 0;

    bool toggle = false;    // Required for individual toggles

    // Draw control
    //--------------------------------------------------------------------
    for (int i = 0; i < count; i++)
    {
        tabBounds.x = bounds.x + (RAYGUI_TABBAR_ITEM_WIDTH + 4)*i - offsetX;

        if (tabBounds.x < GetScreenWidth())
        {
            // Draw tabs as toggle controls
            int textAlignment = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
            int textPadding = GuiGetStyle(TOGGLE, TEXT_PADDING);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 8);

            if (i == (*active))
            {
                toggle = true;
                GuiToggle(tabBounds, text[i], &toggle);
            }
            else
            {
                toggle = false;
                GuiToggle(tabBounds, text[i], &toggle);
                if (toggle) *active = i;
            }

            GuiSetStyle(TOGGLE, TEXT_PADDING, textPadding);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, textAlignment);
        }
    }

    // Draw tab-bar bottom line
    GuiDrawRectangle((Rectangle){ bounds.x, bounds.y + bounds.height - 1, bounds.width, 1 }, 0, BLANK, GetColor(GuiGetStyle(TOGGLE, BORDER_COLOR_NORMAL)));
    //--------------------------------------------------------------------
}

int GuiButtonMultiLine(Rectangle bounds, const char **text, int count, int icon)
{
    int result = 0;
    GuiState state = GuiGetState();

    bool locked = GuiIsLocked();
    bool slider_dragging = GuiIsSliderDragging();

    int border_width = GuiGetStyle(BUTTON, BORDER_WIDTH);

    float line_height = bounds.height - (2 * border_width);

    Rectangle line_bounds = GetTextBounds(BUTTON, bounds);
    Rectangle outer_bounds = {
        .x      = bounds.x,
        .y      = bounds.y,
        .width  = bounds.width,
        .height = (line_height * count) + ((count)* border_width)
    };

    Vector2 icon_pos = {
        .x = line_bounds.x,
        .y = outer_bounds.y + (outer_bounds.height / 2) - (RAYGUI_ICON_SIZE / 2)
    };

    if (icon) {
        line_bounds.x     += RAYGUI_ICON_SIZE + GuiGetStyle(DEFAULT, TEXT_PADDING) + border_width;
        line_bounds.width -= RAYGUI_ICON_SIZE;
    }

    // Update control
    //--------------------------------------------------------------------
    if ((state != STATE_DISABLED) && !locked && !slider_dragging)
    {
        Vector2 mousePoint = GetMousePosition();

        // Check button state
        if (CheckCollisionPointRec(mousePoint, outer_bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
            else state = STATE_FOCUSED;

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) result = 1;
        }
    }
    //--------------------------------------------------------------------

    // Draw control
    //--------------------------------------------------------------------
    GuiDrawRectangle(outer_bounds, border_width, GetColor(GuiGetStyle(BUTTON, BORDER + (state*3))), GetColor(GuiGetStyle(BUTTON, BASE + (state*3))));

    //GuiDrawIcon(icon, icon_pos.x + 1, icon_pos.y + 1, 1, ColorAlpha(BLACK, 0.5));

    GuiDrawIcon(icon, icon_pos.x, icon_pos.y, 1, RAYWHITE);

    for (int i=0; i<count; i++) {
        GuiDrawText(text[i], line_bounds, GuiGetStyle(BUTTON, TEXT_ALIGNMENT), GetColor(GuiGetStyle(BUTTON, TEXT + (state*3))));
        line_bounds.y += line_height - border_width;
    }

    if (state == STATE_FOCUSED) GuiTooltip(bounds);
    //------------------------------------------------------------------

    return result;
}
