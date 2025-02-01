/****************************************************************************
 *                                                                          *
 * numeric.h                                                                *
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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#ifndef NUMERIC_H
#define NUMERIC_H

enum numeric_type {
    NUMERIC_NULL = 0,
    NUMERIC_INT,
    NUMERIC_FLOAT,
    NUMERIC_PTR_INT,
    NUMERIC_PTR_FLOAT
};
typedef enum numeric_type numeric_type_t;

const char *numeric_type_to_string(numeric_type_t type);

#define is_numeric_value_type(x) \
    (((x) == NUMERIC_INT) ||     \
     ((x) == NUMERIC_FLOAT))

#define is_numeric_ptr_type(x)   \
    (((x) == NUMERIC_PTR_INT) || \
     ((x) == NUMERIC_PTR_FLOAT))

#define is_numeric_type(x) \
    (is_numeric_value_type(x) || \
     is_numeric_ptr_type(x))

#define assert_numeric_type(x) \
    assert(is_numeric_type(x))

#define assert_valid_numeric(n) \
    assert_numeric_type(n.type)

#define is_numeric_type_like_int(x) \
    (((x) == NUMERIC_INT) ||        \
     ((x) == NUMERIC_PTR_INT))

#define is_numeric_type_like_float(x) \
    (((x) == NUMERIC_FLOAT) ||        \
     ((x) == NUMERIC_PTR_FLOAT))

#define is_numeric_like_int(n) \
    is_numeric_type_like_int(n.type)
#define is_numeric_like_float(n) \
    is_numeric_type_like_float(n.type)

struct numeric {
    numeric_type_t type;
    union {
        int i;
        float f;
        int *ip;
        float *fp;
    };
};
typedef struct numeric numeric_t;

static inline numeric_t numeric_float(float f)
{
    numeric_t n = {
        .type = NUMERIC_FLOAT,
        .f    = f
    };
    return n;
}

static inline numeric_t numeric_int(int i)
{
    numeric_t n = {
        .type = NUMERIC_INT,
        .i    = i
    };
    return n;
}

static inline numeric_t numeric_float_ptr(float *fp)
{
    numeric_t n = {
        .type = NUMERIC_PTR_FLOAT,
        .fp   = fp
    };
    return n;
}

static inline numeric_t numeric_int_ptr(int *ip)
{
    numeric_t n = {
        .type = NUMERIC_PTR_INT,
        .ip   = ip
    };
    return n;
}

float numeric_to_float(numeric_t n);
int numeric_to_int(numeric_t n);

numeric_t numeric_copy(numeric_t n, numeric_t other);
numeric_t numeric_set(numeric_type_t type, float f, int i);
numeric_t numeric_set_type(numeric_type_t type, numeric_t n);
numeric_t numeric_zero(numeric_type_t type);
numeric_t numeric_one(numeric_type_t type);

numeric_t numeric_add(numeric_t a, numeric_t b);
numeric_t numeric_sub(numeric_t a, numeric_t b);
numeric_t numeric_mul(numeric_t a, numeric_t b);
numeric_t numeric_div(numeric_t a, numeric_t b);
numeric_t numeric_mod(numeric_t a, numeric_t b);
numeric_t numeric_pow(numeric_t a, numeric_t b);

bool numeric_gt(numeric_t a, numeric_t b);
bool numeric_lt(numeric_t a, numeric_t b);
bool numeric_gte(numeric_t a, numeric_t b);
bool numeric_lte(numeric_t a, numeric_t b);

const char *numeric_text(numeric_t n);

#endif /*NUMERIC_H*/

