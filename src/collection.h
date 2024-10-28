/****************************************************************************
 *                                                                          *
 * collection.h                                                             *
 *                                                                          *
 * This file is part of hexpuzzle.                                              *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or                   *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#ifndef COLLECTION_H
#define COLLECTION_H

#include "const.h"

#define IS_COLLECTION_FILENAME(filename)                            \
    (0 == strcmp(filename_ext(filename), COLLECTION_FILENAME_EXT))

#include "level.h"

#define COLLECTION_ID_LENGTH 37
struct collection {
    char id[COLLECTION_ID_LENGTH];
    bool have_id;

    char *dirpath;
    char *filename;
    int filename_seq;

    bool is_pack;

    level_t *levels;
    int level_count;

    const char **level_names;
    int level_name_count;

    int gui_list_scroll_index;
    int gui_list_active;
    int gui_list_focus;

    bool changed;

    struct collection *prev, *next;
};
typedef struct collection collection_t;

extern collection_t *current_collection;

collection_t *create_collection(void);
collection_t *load_collection_dir(const char *dirpath);
collection_t *load_collection_level_file(const char *filename);
collection_t *load_collection_pack_compressed_data(const char *name, unsigned char *compressed, int compsize);
collection_t *load_collection_pack_file(const char *filename);
collection_t *load_collection_file(const char *filename);
collection_t *load_collection_path(const char *path);

void destroy_collection(collection_t *collection);

void collection_clear_levels(collection_t *collection);
bool collection_scan_dir(collection_t *collection);

const char *collection_path(collection_t *collection);
const char *collection_name(collection_t *collection);

bool collection_level_name_exists(collection_t *collection, const char *name);
void collection_add_level(collection_t *collection, level_t *level);
bool collection_add_level_file(collection_t *collection, const char *filename);

level_t *collection_get_level_after(collection_t *collection, level_t *level);
level_t *collection_find_level_by_id(collection_t *collection, const char *id);
level_t *collection_find_level_by_filename(collection_t *collection, const char *filename);
void collection_update_level_names(collection_t *collection);

void collection_draw(collection_t *collection);

void collection_save_dir(collection_t *collection, const char *dirpath, bool changed_only);
void collection_save_pack(collection_t *collection, const char *filename);
void collection_save(collection_t *collection);

void collection_extract_level_from_grid(collection_t *collection, level_t *level);

#endif /*COLLECTION_H*/

