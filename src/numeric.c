/****************************************************************************
 *                                                                          *
 * numeric.c                                                                *
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

#include "common.h"
#include "numeric.h"

#define both_like_int(a, b)    \
    (is_numeric_like_int(a) && \
     is_numeric_like_int(b))

#define both_like_float(a, b)    \
    (is_numeric_like_float(a) && \
     is_numeric_like_float(b))

#define assert_valid_numerics(a,b) \
    assert_valid_numeric(a);       \
    assert_valid_numeric(b)

const char *numeric_type_to_string(numeric_type_t type)
{
    switch (type) {
    case NUMERIC_NULL:      return "NUMERIC_NULL";
    case NUMERIC_FLOAT:     return "NUMERIC_FLOAT";
    case NUMERIC_INT:       return "NUMERIC_INT";
    case NUMERIC_PTR_FLOAT: return "NUMERIC_PTR_FLOAT";
    case NUMERIC_PTR_INT:   return "NUMERIC_PTR_INT";
    default:                return "(INVALID)";
    }
}

numeric_t numeric_copy(numeric_t n, numeric_t other)
{
    switch (n.type) {
    case NUMERIC_FLOAT:
        n.f = numeric_to_float(other);
        break;

    case NUMERIC_INT:
        n.i = numeric_to_int(other);
        break;

    case NUMERIC_PTR_FLOAT:
        *n.fp = numeric_to_float(other);
        break;

    case NUMERIC_PTR_INT:
        *n.ip = numeric_to_int(other);
        break;

    default:
        DIE("invalid numeric_t type");
    }

    return n;
}


numeric_t numeric_set(numeric_type_t type, float f, int i)
{
    assert_numeric_type(type);

    numeric_t n = {
        .type = type
    };

    switch (type) {
    case NUMERIC_FLOAT:
        n.f = f;
        break;

    case NUMERIC_INT:
        n.i = i;
        break;

    case NUMERIC_PTR_FLOAT:
        *n.fp = f;
        break;

    case NUMERIC_PTR_INT:
        *n.ip = i;
        break;

    default:
        DIE("invalid numeric_t type");
    }

    return n;
}

numeric_t numeric_set_type(numeric_type_t type, numeric_t n)
{
    assert_numeric_type(type);

    switch (type) {
    case NUMERIC_FLOAT:
        return numeric_float(numeric_to_float(n));

    case NUMERIC_INT:
        return numeric_int(numeric_to_int(n));

    default:
        DIE("invalid numeric_t type");
    }
}

numeric_t numeric_zero(numeric_type_t type)
{
    assert_numeric_type(type);

    return numeric_set(type, 0.0, 0);
}

numeric_t numeric_one(numeric_type_t type)
{
    assert_numeric_type(type);

    return numeric_set(type, 1.0, 1);
}


float numeric_to_float(numeric_t n)
{
    switch (n.type) {
    case NUMERIC_FLOAT:
        return n.f;

    case NUMERIC_INT:
        return (float)(n.i);

    case NUMERIC_PTR_FLOAT:
        return *n.fp;

    case NUMERIC_PTR_INT:
        return (float)(*n.ip);

    default:
        DIE("invalid numeric_t type");
    }
}

int numeric_to_int(numeric_t n)
{
    switch (n.type) {
    case NUMERIC_FLOAT:
        return (int)(n.f);

    case NUMERIC_INT:
        return n.i;

    case NUMERIC_PTR_FLOAT:
        return (int)(*n.fp);

    case NUMERIC_PTR_INT:
        return *n.ip;

    default:
        DIE("invalid numeric_t type");
    }
}

numeric_t numeric_add(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) + numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = numeric_to_float(a) + numeric_to_float(b);
    }

    return n;
}

numeric_t numeric_sub(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) - numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = numeric_to_float(a) - numeric_to_float(b);
    }

    return n;
}

numeric_t numeric_mul(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) * numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = numeric_to_float(a) * numeric_to_float(b);
    }

    return n;
}


numeric_t numeric_div(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) / numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = numeric_to_float(a) / numeric_to_float(b);
    }

    return n;
}

numeric_t numeric_mod(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) & numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = fmodf(numeric_to_float(a), numeric_to_float(b));
    }

    return n;
}


numeric_t numeric_pow(numeric_t a, numeric_t b)
{
    numeric_t n;

    assert_valid_numerics(a,b);

    if (both_like_int(a, b)) {
        n.type = NUMERIC_INT;
        n.i = numeric_to_int(a) ^ numeric_to_int(b);
    } else {
        n.type = NUMERIC_FLOAT;
        n.f = powf(numeric_to_float(a), numeric_to_float(b));
    }

    return n;
}

bool numeric_gt(numeric_t a, numeric_t b)
{
    if (both_like_int(a, b)) {
        return (numeric_to_int(a) > numeric_to_int(b));
    } else {
        return (numeric_to_float(a) > numeric_to_float(b));
    }
}

bool numeric_lt(numeric_t a, numeric_t b)
{
    if (both_like_int(a, b)) {
        return (numeric_to_int(a) < numeric_to_int(b));
    } else {
        return (numeric_to_float(a) < numeric_to_float(b));
    }
}

const char *numeric_text(numeric_t n)
{
    assert_valid_numeric(n);

    switch (n.type) {
    case NUMERIC_FLOAT:
        return TextFormat("%f", n.f);

    case NUMERIC_INT:
        return TextFormat("%d", n.i);

    case NUMERIC_PTR_FLOAT:
        return TextFormat("%3.2f", *n.fp);

    case NUMERIC_PTR_INT:
        return TextFormat("%d", *n.ip);

    default:
        DIE("invalid numeric_t type");
    }    
}

