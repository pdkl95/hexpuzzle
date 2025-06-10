/****************************************************************************
 *                                                                          *
 * blueprint_string.c                                                       *
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
#include "path.h"
#include "blueprint_string.h"

#include <ctype.h>

char hex_digit_str[] = "0123456789ABCDEF";

/*************************************************************************
 *  SERIALIZE
 */

static const char *serialize_color(generate_level_param_t *param)
{
    static char buf[3];
    buf[0] = 'c';
    buf[2] = '\0';

    int bits = 0;
    each_path_color {
        int bit = param->color[color] << (color - 1);
        bits |= bit;
    }

    buf[1] = hex_digit_str[bits];

    return buf;
}

static const char *serialize_prefix(void)
{
    return BLUEPRINT_STRING_PREFIX;
}

const char *serialize_generate_level_params(generate_level_param_t param)
{
    static char buf[BLUEPRINT_STRING_MAXLEN];

    const char *prefix_str = serialize_prefix();
    if (!prefix_str) { goto serialize_failure; }

    const char *color_str  = serialize_color(&param);
    if (!color_str) { goto serialize_failure; }

    int ret = snprintf(buf,
                       BLUEPRINT_STRING_MAXLEN,
                       "%s%s",
                       prefix_str,
                       color_str);

    if (ret < 0) {
      serialize_failure:
        errmsg("Failed to serialize params into blueprint string.");
        return NULL;
    } else {
        return buf;
    }
}


/*************************************************************************
 *  DESERIALIZE
 */

void deserial_error(const char *str, int str_length, int err_idx, const char *field, const char *reason)
{
    assert_not_null(field);
    assert_not_null(reason);

    assert(str_length + 1 < 256);
    assert(err_idx < str_length);

    char buf[256];

    if (str_length > 255) {
        str_length = 255;
    }

    if (err_idx >= str_length) {
        err_idx = -1;
    }

    memcpy(buf, str, str_length);
    buf[str_length + 1] = '\0';

    errmsg("Failed to parse \"%s\" as param.%s", buf, field);
    if (err_idx >= 0) {
        if (err_idx > 0) {
            memset(buf, ' ', err_idx);
        }
        buf[err_idx] = '\0';
        errmsg("                 %s^", buf);
    }
    errmsg("Reason: %s\n", reason);
}

bool deserialize_colors(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    if (str[0] != 'c') {
        deserial_error(str, 2, 0, "colors", "expected 'c'");
        return false;
    }

    if (!isxdigit(str[1])) {
        deserial_error(str, 2, 1, "colors", "expected [0-8a-fA-F]");
        return false;
    }

    int c = -1;

    for (int i=0; i<16; i++) {
        if (str[1] == hex_digit_str[i]) {
            c = i;
            break;
        }
    }

    param->color_count = 0;

    each_path_color {
        int bit = 1 << color;
        if (c & bit) {
            param->color[color] = true;
            param->color_count += 1;
        } else {
            param->color[color] = false;
        }
    }

    if (param->color_count < 1) {
        deserial_error(str, 2, 1, "colors", "expected to be non-zero");
        return false;
    }


    *strp += 2;
    return  true;
}

bool deserialize_prefix(const char **strp)
{
    const char *str = *strp;
    const char *prefix = BLUEPRINT_STRING_PREFIX;
    int prefix_length = strlen(prefix);

    int cmp = -1;
    for (int i=0; i<prefix_length; i++) {
        if (str[i] != prefix[i]) {
            cmp = i;
            break;
        }
    }

    if (-1 != cmp) {
        deserial_error(str, prefix_length, cmp, "prefix", "expected \"" BLUEPRINT_STRING_PREFIX " \"");
        return false;
    }

    *strp += prefix_length;

    return true;
}

bool deserialize_generate_level_params(const char *str, generate_level_param_t *result)
{
    generate_level_param_t param = *result;

    const char *p = str;

    if (!deserialize_prefix(&p)) { return false; }

    while (*p) {
        switch (*p) {
        case 'c':
            if (!deserialize_colors(&p, &param)) { return false; }
            break;

        default:
            deserial_error(p, strlen(p), 0, NULL, NULL);
            errmsg("Unexpected character in blueprint string: \"%c\"", *p);
            return false;
        }
    }

    *result = param;

    return true;
}
