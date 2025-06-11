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

static const char *serialize_prefix(void)
{
    return BLUEPRINT_STRING_PREFIX;
}

static const char *serialize_suffix(void)
{
    return BLUEPRINT_STRING_SUFFIX;
}

static const char *serialize_color(generate_level_param_t *param)
{
    static char buf[BLUEPRINT_STRING_COLOR_MAXLEN];
    buf[0] = 'p';
    buf[2] = '\0';

    int bits = 0;
    each_path_color {
        int bit = param->color[color] << (color - 1);
        bits |= bit;
    }

    buf[1] = hex_digit_str[bits];

    return buf;
}

static const char *serialize_tile_radius(generate_level_param_t *param)
{
    static char buf[BLUEPRINT_STRING_RADIUS_MAXLEN];
    snprintf(buf,
             BLUEPRINT_STRING_RADIUS_MAXLEN,
             "r%X",
             param->tile_radius);
    return buf;
}

static const char *serialize_fixed(generate_level_param_t *param)
{
    static char buf[BLUEPRINT_STRING_FIXED_MAXLEN];
    snprintf(buf,
             BLUEPRINT_STRING_FIXED_MAXLEN,
             "i%X",
             param->fixed_count);
    return buf;
}

static const char *serialize_hidden(generate_level_param_t *param)
{
    static char buf[BLUEPRINT_STRING_HIDDEN_MAXLEN];
    snprintf(buf,
             BLUEPRINT_STRING_HIDDEN_MAXLEN,
             "h%X",
             param->hidden_count);
    return buf;
}

static const char *serialize_seed(generate_level_param_t *param)
{
    static char buf[BLUEPRINT_STRING_SEED_MAXLEN];
    snprintf(buf,
             BLUEPRINT_STRING_SEED_MAXLEN,
             "s%lX",
             param->seed);
    return buf;
}

static const char *serialize_path_density(generate_level_param_t *param)
{
    int density = (int)param->path_density;
    static char buf[BLUEPRINT_STRING_DENSITY_MAXLEN];
    snprintf(buf,
             BLUEPRINT_STRING_DENSITY_MAXLEN,
             "n%X",
             density);
    return buf;
}

static const char *serialize_fill(generate_level_param_t *param)
{
    if (param->fill_all_tiles) {
        return "L";
    } else {
        return "";
    }
}

static const char *serialize_symmetry(generate_level_param_t *param)
{
    switch (param->symmetry_mode) {
    case SYMMETRY_MODE_NONE:
        return "";
    case SYMMETRY_MODE_REFLECT:
        return "yR";
    case SYMMETRY_MODE_ROTATE:
        return "yT";
    default:
        errmsg("Unknown blueprint string representation for symmetry mode %d", param->mode);
        return NULL;
    }
}

static const char *serialize_mode(generate_level_param_t *param)
{
    switch (param->mode) {
    case GENERATE_LEVEL_BLANK:
        return "mB";
    case GENERATE_LEVEL_RANDOM_CONNECT_TO_POINT:
        return "mC";
    default:
        errmsg("Unknown blueprint string representation for mode %d", param->mode);
        return NULL;
    }
}

const char *serialize_generate_level_params(generate_level_param_t param)
{
    static char buf[BLUEPRINT_STRING_MAXLEN];

    const char *prefix_str = serialize_prefix();
    if (!prefix_str) { goto serialize_failure; }

    const char *mode_str  = serialize_mode(&param);
    if (!mode_str) { goto serialize_failure; }

    const char *symmetry_str  = serialize_symmetry(&param);
    if (!symmetry_str) { goto serialize_failure; }

    const char *fill_str  = serialize_fill(&param);
    if (!fill_str) { goto serialize_failure; }

    const char *color_str  = serialize_color(&param);
    if (!color_str) { goto serialize_failure; }

    const char *radius_str  = serialize_tile_radius(&param);
    if (!radius_str) { goto serialize_failure; }

    const char *fixed_str  = serialize_fixed(&param);
    if (!fixed_str) { goto serialize_failure; }

    const char *hidden_str  = serialize_hidden(&param);
    if (!hidden_str) { goto serialize_failure; }

    const char *density_str  = serialize_path_density(&param);
    if (!density_str) { goto serialize_failure; }

    const char *seed_str  = serialize_seed(&param);
    if (!seed_str) { goto serialize_failure; }

    const char *suffix_str = serialize_suffix();
    if (!suffix_str) { goto serialize_failure; }

    int ret = snprintf(buf,
                       BLUEPRINT_STRING_MAXLEN,
                       "%s%s%s%s%s%s%s%s%s%s%s",
                       prefix_str,
                       mode_str,
                       symmetry_str,
                       fill_str,
                       color_str,
                       radius_str,
                       fixed_str,
                       hidden_str,
                       density_str,
                       seed_str,
                       suffix_str);

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

static void deserial_error(const char *str, int str_length, int err_idx, const char *field, const char *reason)
{
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

    if (field) {
        errmsg("Failed to parse \"%s\" as param.%s", buf, field);
    } else {
        errmsg("Failed to parse \"%s\"", buf);
    }

    if (err_idx >= 0) {
        if (err_idx > 0) {
            memset(buf, ' ', err_idx);
        }
        buf[err_idx] = '\0';
        errmsg("                 %s^", buf);
    }

    if (reason) {
        errmsg("Reason: %s\n", reason);
    }
}

static bool deserialize_match_string_prefix(const char **strp, const char *prefix, const char *field, const char *reason)
{
    const char *str = *strp;
    int prefix_length = strlen(prefix);

    int cmp = -1;
    for (int i=0; i<prefix_length; i++) {
        if (str[i] != prefix[i]) {
            cmp = i;
            break;
        }
    }

    if (-1 != cmp) {
        deserial_error(str, prefix_length, cmp, field, reason);
        return false;
    }

    *strp += prefix_length;

    return true;
}

static bool deserialize_prefix(const char **strp)
{
    return deserialize_match_string_prefix(strp,
                                           BLUEPRINT_STRING_PREFIX,
                                           "prefix",
                                           "expected \"" BLUEPRINT_STRING_PREFIX " \"");
}

static bool deserialize_suffix(const char **strp)
{
    return deserialize_match_string_prefix(strp,
                                           BLUEPRINT_STRING_SUFFIX,
                                           "suffix",
                                           "expected \"" BLUEPRINT_STRING_SUFFIX " \"");
}

static bool deserialize_mode(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    if (str[0] != 'm') {
        deserial_error(str, 2, 0, "mode", "expected 'm'");
        return false;
    }

    switch (str[1]) {
    case 'B':
        param->mode = GENERATE_LEVEL_BLANK;
        break;
    case 'C':
        param->mode = GENERATE_LEVEL_RANDOM_CONNECT_TO_POINT;
        break;
    default:
        deserial_error(str, 2, 1, "mode", "unknown mode type");
        return false;
    }

    *strp += 2;

    return true;
}

static bool deserialize_colors(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    if (str[0] != 'p') {
        deserial_error(str, 2, 0, "colors", "expected 'p'");
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

static int deserislize_get_hex_number_field_length(const char *str)
{
    int len = 1;
    str++;

    while (isxdigit(*str)) {
        str++;
        len++;
    }

    return len;
}

static bool deserialize_get_hex_number(const char *str, int *result)
{
    unsigned int value = 0;
    errno = 0;
    int ret = sscanf(str, "%X", &value);
    if (errno) {
        errmsg("sscanf() failed: $s", strerror(errno));
        return false;
    }
    if (ret == 1) {
        *result = value;
        return true;
    } else {
        errmsg("sscanf() failed to find a hexadecimal number in \"%.8s\"", str);
        return false;
    }
}

static bool deserialize_seed(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    int field_length = deserislize_get_hex_number_field_length(str);
    if (str[0] != 's') {
        deserial_error(str, field_length, 0, "seed", "expected 's'");
        return false;
    }

    str++;
    int value = 0;
    bool ret = deserialize_get_hex_number(str, &value);
    param->seed = value;

    *strp += field_length;

    return ret;
}

static bool deserialize_path_density(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    int field_length = deserislize_get_hex_number_field_length(str);
    if (str[0] != 'n') {
        deserial_error(str, field_length, 0, "path_density", "expected 'n'");
        return false;
    }

    str++;
    int value = 0;
    bool ret = deserialize_get_hex_number(str, &value);
    param->path_density = value;

    *strp += field_length;

    return ret;
}

static bool deserialize_tile_radius(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    int field_length = deserislize_get_hex_number_field_length(str);
    if (str[0] != 'r') {
        deserial_error(str, field_length, 0, "tile_radius", "expected 'r'");
        return false;
    }

    str++;
    bool ret = deserialize_get_hex_number(str, &param->tile_radius);

    *strp += field_length;

    return ret;
}

static bool deserialize_fixed(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    int field_length = deserislize_get_hex_number_field_length(str);
    if (str[0] != 'i') {
        deserial_error(str, field_length, 0, "fixed", "expected 'i'");
        return false;
    }

    str++;
    bool ret = deserialize_get_hex_number(str, &param->fixed_count);

    *strp += field_length;

    return ret;
}

static bool deserialize_hidden(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    int field_length = deserislize_get_hex_number_field_length(str);
    if (str[0] != 'h') {
        deserial_error(str, field_length, 0, "hidden", "expected 'h'");
        return false;
    }

    str++;
    bool ret = deserialize_get_hex_number(str, &param->hidden_count);

    *strp += field_length;

    return ret;
}

static bool deserialize_fill(const char **strp, generate_level_param_t *param)
{
    const char *str = *strp;
    if (str[0] != 'L') {
        deserial_error(str, 1, 0, "hidden", "expected 'L'");
        return false;
    }

    param->fill_all_tiles = true;

    *strp += 1;

    return true;
}

bool deserialize_generate_level_params(const char *str, generate_level_param_t *result)
{
    generate_level_param_t param = *result;

    if (options->verbose) {
        infomsg("deserializing blueprint string: \"%s\"", str);
    }

    const char *p = str;

    if (!deserialize_prefix(&p))       { return false; }
    if (!deserialize_mode(&p, &param)) { return false; }

    while (p && *p) {
        printf("deserialize parse[%ld]: \"%.8s\"\n", p-str, p);
        const char *loop_start_p = p;

        switch (*p) {
        case 'p':
            if (!deserialize_colors(&p, &param)) { return false; }
            break;

        case 'r':
            if (!deserialize_tile_radius(&p, &param)) { return false; }
            break;

        case 'i':
            if (!deserialize_fixed(&p, &param)) { return false; }
            break;

        case 'h':
            if (!deserialize_hidden(&p, &param)) { return false; }
            break;

        case 'L':
            if (!deserialize_fill(&p, &param)) { return false; }
            break;

        case 's':
            if (!deserialize_seed(&p, &param)) { return false; }
            break;

        case 'n':
            if (!deserialize_path_density(&p, &param)) { return false; }
            break;

        case 'z':
            if (!deserialize_suffix(&p)) { return false; }
            /* end of blueprint string - stop reading */
            p = NULL;
            break;

        default:
            errmsg("Unexpected character '%c' in blueprint string \"%.8s\"", *p, p);
            return false;
        }

        assert(p != loop_start_p);
    }

    *result = param;

    return true;
}
