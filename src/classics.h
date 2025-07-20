/****************************************************************************
 *                                                                          *
 * classics.h                                                               *
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

#ifndef CLASSICS_H
#define CLASSICS_H

struct collection;
struct level;

struct classic_level_nameref {
    name_str_t collection_id;
    unique_id_t level_unique_id;
};
typedef struct classic_level_nameref classic_level_nameref_t;
#define CLASSIC_LEVEL_NAMEREF(collection_id, level_unique_id) \
    ((classic_level_nameref_t){                               \
        .collection_id   = collection_id,                     \
        .level_unique_id = level_unique_id                    \
    })

struct classic_collection {
    int index;
    char *id;
    char *filename;
    unsigned char *data;
    unsigned int *dlen;
    struct collection *collection;
};
typedef struct classic_collection classic_collection_t;

extern classic_collection_t classic_collections[];


const char *classic_level_nameref_string(classic_level_nameref_t *ref);

void open_classics_game_pack(int n);

int open_classic_level_nameref(classic_level_nameref_t *ref);

struct level *find_classic_level_by_nameref(classic_level_nameref_t *ref);

#endif /*CLASSICS_H*/

