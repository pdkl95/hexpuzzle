/****************************************************************************
 *                                                                          *
 * range.c                                                                  *
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
#include "range.h"

#define RANGE_STR_MAX 256
const char *int_range_string(int_range_t *ir)
{
    static char buf[RANGE_STR_MAX];
    if (ir->min == ir->max) {
        snprintf(buf, RANGE_STR_MAX, "%d", ir->max);
    } else {
        snprintf(buf, RANGE_STR_MAX, "%d..%d", ir->min, ir->max);
    }

    return buf;
}

cJSON *int_range_to_json(int_range_t *ir)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "min", ir->min) == NULL) {
        goto int_range_json_error;
    }
    
    if (cJSON_AddNumberToObject(json, "max", ir->max) == NULL) {
        goto int_range_json_error;
    }

    return json;

  int_range_json_error:
    cJSON_Delete(json);
    return NULL;
}

bool int_range_from_json(cJSON *json, int_range_t *ir)
{
    if (!cJSON_IsObject(json)) {
        errmsg("int range JSON should be an Object");
        return false;
    }

    cJSON *min_json = cJSON_GetObjectItem(json, "min");
    if (!min_json) {
        errmsg("int range JSON Object should have Number 'min'");
        return false;
    }
    if (!cJSON_IsNumber(min_json)) {
        errmsg("int range JSON Object item 'min' is not a Number");
        return false;
    }

    cJSON *max_json = cJSON_GetObjectItem(json, "max");
    if (!max_json) {
        errmsg("int range JSON Object should have Number 'max'");
        return false;
    }
    if (!cJSON_IsNumber(max_json)) {
        errmsg("int range JSON Object item 'max' is not a Number");
        return false;
    }

    ir->min = min_json->valueint;
    ir->max = max_json->valueint;

    return true;
}

void int_range_clamp(int_range_t *ir, int min, int max)
{
    if (ir->min > ir->max) {
        int tmp = ir->min;
        ir->min = ir->max;
        ir->max = tmp;
    }

    CLAMPVAR(ir->min, min, max);
    CLAMPVAR(ir->max, min, max);
}
