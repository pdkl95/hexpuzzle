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

struct cJSON;

void printvec2(Vector2 v);
void printrect(Rectangle r);
void printcolorhex(Color c);
void printcolor(Color c);

#define pbool(var) printf(QUOTE(var) "\t= %s\n", (var) ? "true" : "false");
#define psize(var) printf(QUOTE(var) "\t= % 9zd\n", (var));
#define pint(var) printf(QUOTE(var) "\t= % 9d\n", (var));
#define plong(var) printf(QUOTE(var) "\t= % 9ld\n", (var));
#define pfloat(var) printf(QUOTE(var) "\t= % 12.2f\n", (var));
#define pstr(var) printf(QUOTE(var) "\t= \"%s\"\n", (var));

#define pvec2(var) do { \
        Vector2 vtmp = (var); \
        printf(QUOTE(var) "\t= ( x=% 8.2f, y=% 8.2f )\n", \
               vtmp.x, vtmp.y); \
    } while(0)

#define pivec2(var) do { \
        IVector2 ivtmp = (var); \
        printf(QUOTE(var) "\t= ( x=%d, y=%d )\n", \
               ivtmp.x, ivtmp.y); \
    } while(0)

#define prect(var) do { \
        Rectangle rtmp = (var); \
        printf(QUOTE(var) "\t= ( x=% 8.2f, y=% 8.2f,\n", rtmp.x, rtmp.y); \
        printf(QUOTE(var) "\t    w=% 8.2f, h=% 8.2f )\n", rtmp.width, rtmp.height); \
    } while(0)

#define paxial(var) do { \
        hex_axial_t tmp = (var); \
        printf(QUOTE(var) "\t= ( q=%d, r=%d )\n", \
               tmp.q, tmp.r); \
    } while(0)

#define pcolor(var) do { \
        printf(QUOTE(var) "\t= "); \
        printcolor(var); \
        printf("\n"); \
    } while(0)

#define plevel(var) do { \
        printf(QUOTE(var) "\t= "); \
        print_level(var); \
        printf("\n"); \
    } while(0)




/*** misc stuff */

static inline Vector2 getVector2FromRectangle(Rectangle rect)
{
    Vector2 vec = {
        .x = rect.x,
        .y = rect.y
    };
    return vec;
}

static inline bool is_any_shift_down(void)
{
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static inline bool is_any_control_down(void)
{
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

static inline bool is_any_alt_down(void)
{
    return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
}

static inline bool is_any_super_down(void)
{
    return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

static inline bool is_key_pressed_with_shift(int key)
{
    return IsKeyPressed(key) && is_any_shift_down();
}

static inline bool is_key_pressed_with_control(int key)
{
    return IsKeyPressed(key) && is_any_control_down();
}

static inline bool is_key_pressed_with_alt(int key)
{
    return IsKeyPressed(key) && is_any_alt_down();
}

static inline bool is_key_pressed_with_super(int key)
{
    return IsKeyPressed(key) && is_any_super_down();
}

static inline bool is_key_pressed_with_control_or_super(int key)
{
    return IsKeyPressed(key) &&
        (is_any_control_down() || is_any_super_down());
}

/*** file I/O ***/

bool WriteCompressedFile(const char *filepath, const void *data, int data_size, const char *magic);
unsigned char *ReadCompressedFile(const char *filepath, int *data_size, const char *magic);

bool WriteUncompressedJSONFile(const char *filepath, struct cJSON *json);
bool WriteCompressedJSONFile(const char *filepath, struct cJSON *json);
struct cJSON *ReadCompressedJSONFile(const char *filepath);
struct cJSON *ReadPossiblyCompressedJSONFile(const char *filepath);

/*** Constants ***/

#define VEC2_ZERO    ((Vector2){ 0.0f, 0.0f })
#define VEC2_ONE     ((Vector2){ 1.0f, 1.0f })

#define VEC2_ORIGIN  VEC2_ZERO
#define VEC2_SHADOW  VEC2_ONE

/*** Colors ***/

extern Color TEXT_LIGHT_SHADOW;
extern Color TEXT_DARK_SHADOW;

void DrawTextWindowCenter(const char *text, int posY, int fontSize, Color color);

void DrawTextShadow(const char *text, int posX, int posY, int fontSize, Color color);
void DrawTextDropShadow(const char *text, int posX, int posY, int fontSize, Color fgcolor, Color bgcolor);

Color ColorLerp(Color start, Color end, float amount);

bool ColorEq(Color a, Color b);

#define NormColor(r,g,b,a) (Vector4){ .x = (r), .y = (g), .z = (b), .w = (a) }
#define RGBAf(r,g,b,a) ColorFromNormalized(NormColor((r),(g),(b),(a)))
#define RGBf(r,g,b) ColorFromNormalized(NormColor((r),(g),(b),1.0f))
#define HSV(h,s,v) (Vector3){ .x = (h), .y = (s), .z = (v) }

Color ColorRelHSV(Color color, Vector3 relhsv);

static inline Color ColorRelSaturation(Color color, float relsat)
{
    return ColorRelHSV(color, HSV(0.0f, relsat, 0.0f));
}

static inline Color ColorRelHue(Color color, float relhue)
{
    return ColorRelHSV(color, HSV(relhue, 0.0f, 0.0f));
}

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

static inline Vector2 Vector2MaxLength(Vector2 a, Vector2 b)
{
    float la = Vector2Length(a);
    float lb = Vector2Length(b);
    if (la > lb) {
        return a;
    } else {
        return b;
    }
}

Vector2 Vector2RotateAroundPoint(Vector2 v, float angle, Vector2 p);
Rectangle BoundingBoxRotatedRectangle(Rectangle rect, float angle);
Vector2 RectangleCenter(Rectangle rect);

Rectangle ExpandRectangle(Rectangle rect, float margin);
Rectangle ExpandRectangleWH(Rectangle rect, float width_margin, float height_margin);
Rectangle move_rect_to_screen_center(Rectangle rect);

static inline Rectangle RectangleMoveX(Rectangle rect, float xoffset)
{
    rect.x += xoffset;
    return rect;
}

static inline Rectangle RectangleMoveY(Rectangle rect, float yoffset)
{
    rect.y += yoffset;
    return rect;
}

static inline Rectangle RectangleMove(Rectangle rect, float xoffset, float yoffset)
{
    rect.x += xoffset;
    rect.y += yoffset;
    return rect;
}

static inline Rectangle RectangleMoveV(Rectangle rect, Vector2 offset)
{
    rect.x += offset.x;
    rect.y += offset.y;
    return rect;
}

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

/*** Custom RayGui widgets ***/
void GuiDrawRectangle(Rectangle rec, int borderWidth, Color borderColor, Color color);
void GuiSimpleTabBar(Rectangle bounds, const char **text, int count, int *active);
int GuiButtonMultiLine(Rectangle bounds, const char **text, int count, int icon);

/*** custom variations of raylib functions ***/

void OpenURLBackground(const char *url);


#endif /*RAYLIB_HELPER_H*/

