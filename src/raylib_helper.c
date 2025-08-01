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

#include "cJSON/cJSON.h"

#include "raygui/raygui.h"
#include "raylib_helper.h"

bool WriteCompressedFile(const char *filepath, const void *data, int data_size, const char *magic)
{
    assert_not_null(filepath);
    assert_not_null(data);
    assert_not_null(magic);

    int magiclen = strlen(magic);
    assert(magiclen > 3);
    assert(magiclen <= 32);

    int compsize = 0;
    unsigned char *compressed = CompressData(data, data_size, &compsize);
    if (NULL == compressed) {
        errmsg("Error compressing \"%s\" (%d bytes)", filepath, data_size);
        return false;
    }

    int compfilesize = compsize + magiclen;
    unsigned char *filedata = MemRealloc(compressed, compfilesize);
    compressed += magiclen;
    memmove(compressed, filedata, compsize);
    memcpy(filedata, magic, magiclen);

    bool ret = SaveFileData(filepath, filedata, compfilesize);
    MemFree(filedata);
    return ret;
}

unsigned char *ReadCompressedFile(const char *filepath, int *data_size, const char *magic)
{
    assert_not_null(filepath);
    assert_not_null(data_size);
    assert_not_null(magic);

    int magiclen = strlen(magic);
    assert(magiclen > 3);
    assert(magiclen <= 32);

    int compfilesize = 0;
    unsigned char *filedata = LoadFileData(filepath, &compfilesize);
    if (NULL == filedata) {
        errmsg("Error loading \"%s\"", filepath);
        return NULL;
    }

    //infomsg("Read %d bytes of compressed data from \"%s\"", compsize, filepath);
    if (0 != memcmp(filedata, magic, magiclen)) {
        errmsg("Error loading \"%s\" - file does not start with %d byte magic identifier \"%s\"",
               filepath, magiclen, magic);
        UnloadFileData(filedata);
        return NULL;
    }

    int compsize = compfilesize - magiclen;
    unsigned char *compressed = filedata + magiclen;

    unsigned char *data = DecompressData(compressed, compsize, data_size);
    UnloadFileData(filedata);
    if ((NULL == data) || (*data_size < 1)) {
        errmsg("Error decompressing \"%s\"", filepath);
        return NULL;
    }

    return data;
}

#ifndef COMPRESSED_JSON_MAGIC
# define COMPRESSED_JSON_MAGIC PACKAGE_NAME ":json/deflate:"
#endif
const char *magio_json_deflate = COMPRESSED_JSON_MAGIC;

bool WriteUncompressedJSONFile(const char *filepath, struct cJSON *json)
{
    char *json_str = cJSON_Print(json);
    if (!json_str) {
        return false;
    }

    bool ret = SaveFileData(filepath, json_str, strlen(json_str));

    SAFEFREE(json_str);

    return ret;
}

bool WriteCompressedJSONFile(const char *filepath, struct cJSON *json)
{
    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str) {
        return false;
    }

    bool ret = WriteCompressedFile(filepath, json_str, strlen(json_str), magio_json_deflate);

    SAFEFREE(json_str);

    return ret;
}

struct cJSON *ReadCompressedJSONFile(const char *filepath)
{
    int size = 0;
    unsigned char *json_str = ReadCompressedFile(filepath, &size, magio_json_deflate);
    if (NULL == json_str) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(( char *)json_str);

    SAFEFREE(json_str);

    if (NULL == json) {
        errmsg("Error parsing \"%s\" as JSON", filepath);
    }

    return json;
}

struct cJSON *ReadPossiblyCompressedJSONFile(const char *filepath)
{
    assert_not_null(filepath);

    int magiclen = strlen(magio_json_deflate);
    assert(magiclen > 3);
    assert(magiclen <= 32);

    int compfilesize = 0;
    unsigned char *filedata = LoadFileData(filepath, &compfilesize);
    if (NULL == filedata) {
        errmsg("Error loading \"%s\"", filepath);
        return NULL;
    }

    unsigned char *data = NULL;

    if (0 == memcmp(filedata, magio_json_deflate, magiclen)) {
        /* file is compressed */

        int compsize = compfilesize - magiclen;
        unsigned char *compressed = filedata + magiclen;
        int data_size = 0;
        data = DecompressData(compressed, compsize, &data_size);
        if ((NULL == data) || (data_size < 1)) {
            errmsg("Error decompressing \"%s\"", filepath);
            data = NULL;
        }

    } else if ((filedata[0] == '{') && (filedata[compfilesize - 1] == '}')) {
        /* file is (probaly) JSON */
        // just pass 'data' straight to cJSON_Parse */
        data = filedata;
    } else {
        /* cannot parse file */
        errmsg("Error decompressing \"%s\"", filepath);
        data = NULL;
    }

    if (data) {
        cJSON *json = cJSON_Parse(( char *)data);

        if (NULL == json) {
            errmsg("Error parsing \"%s\" as JSON", filepath);
        }

        if (data == filedata) {
            UnloadFileData(filedata);
        } else {
            SAFEFREE(data);
            UnloadFileData(filedata);
        }
        return json;
    } else {
        if (data == filedata) {
            UnloadFileData(filedata);
        } else {
            SAFEFREE(data);
            UnloadFileData(filedata);
        }
        return NULL;
    }
}

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

Color ColorRelHSV(Color color, Vector3 relhsv)
{
    Vector3 hsv = ColorToHSV(color);

    hsv = Vector3Add(hsv, relhsv);

    if (hsv.x < 0.0f) {
        hsv.x += 360.0f;
    }

    if (hsv.x >= 360.0f) {
        hsv.x -= 360.0f;
    }

    hsv.y = Clamp(hsv.y, 0.0f, 1.0f);
    hsv.z = Clamp(hsv.z, 0.0f, 1.0f);

    return ColorFromHSV(hsv.x, hsv.y, hsv.z);
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

Rectangle ExpandRectangle(Rectangle rect, float margin)
{
    rect.x -= margin;
    rect.y -= margin;

    margin *= 2.0f;

    rect.width  += margin;
    rect.height += margin;

    return rect;
}

Rectangle ExpandRectangleWH(Rectangle rect, float width_margin, float height_margin)
{
    rect.x -= width_margin;
    rect.y -= height_margin;

    width_margin *= 2.0f;
    height_margin *= 2.0f;

    rect.width  += width_margin;
    rect.height += height_margin;

    return rect;
}

Rectangle move_rect_to_screen_center(Rectangle rect)
{
    rect.x = window_center.x;
    rect.y = window_center.y;

    rect.x -= 0.5f * rect.width;
    rect.y -= 0.5f * rect.height;

    return rect;
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

void GuiDrawRectangle(Rectangle rec, int borderWidth, Color borderColor, Color color)
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

    float line_height = bounds.height - (2 * border_width) - 2;

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
        float icon_space = RAYGUI_ICON_SIZE + GuiGetStyle(DEFAULT, TEXT_PADDING) + border_width;
        line_bounds.x     += icon_space;
        line_bounds.width -= icon_space;

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
    // Draw tabs as toggle controls
    GuiDrawRectangle(outer_bounds, border_width, GetColor(GuiGetStyle(BUTTON, BORDER + (state*3))), GetColor(GuiGetStyle(BUTTON, BASE + (state*3))));

    //GuiDrawIcon(icon, icon_pos.x + 1, icon_pos.y + 1, 1, ColorAlpha(BLACK, 0.5));

    GuiDrawIcon(icon, icon_pos.x, icon_pos.y, 1, RAYWHITE);

    for (int i=0; i<count; i++) {
        GuiDrawText(text[i], line_bounds, GuiGetStyle(BUTTON, TEXT_ALIGNMENT), GetColor(GuiGetStyle(BUTTON, TEXT + (state*3))));
        line_bounds.y += line_height - (2 * border_width);
    }

    if (state == STATE_FOCUSED) GuiTooltip(bounds);
    //------------------------------------------------------------------

    return result;
}

/* same as OpenURL() but with & */
void OpenURLBackground(const char *url)
{
    // Security check to (partially) avoid malicious code
    if (strchr(url, '\'') != NULL) TRACELOG(LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        char *cmd = (char *)RL_CALLOC(strlen(url) + 32, sizeof(char));
#if defined(_WIN32)
        sprintf(cmd, "explorer \"%s\"", url);
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
        sprintf(cmd, "xdg-open '%s' &", url); // Alternatives: firefox, x-www-browser
#endif
#if defined(__APPLE__)
        sprintf(cmd, "open '%s' &", url);
#endif
        int result = system(cmd);
        if (result == -1) TRACELOG(LOG_WARNING, "OpenURL() child process could not be created");
        RL_FREE(cmd);
    }
}
