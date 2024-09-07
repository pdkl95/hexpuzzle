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

#define COLLECTION_FILENAME_EXT "hexlevelpack"
#define IS_COLLECTION_FILENAME(filename) \
    (0 == strcmp(filename_ext(filename), COLLECTION_FILENAME_EXT))

#include "level.h"

struct collection {
    char name[NAME_MAXLEN];

    char *dirpath;
    char *filename;

    level_t *levels;
    int level_count;

    const char **level_names;
    int level_name_count;

    int gui_list_scroll_index;
    int gui_list_active;
    int gui_list_focus;

    struct collection *prev, *next;
};
typedef struct collection collection_t;

collection_t *create_collection(char *name);
collection_t *load_collection_dir(char *dirpath);
collection_t *load_collection_level_file(char *filename);
collection_t *load_collection_zip_file(char *filename);
collection_t *load_collection_file(char *filename);
collection_t *load_collection_path(char *path);

void destroy_collection(collection_t *collection);

void collection_clear_levels(collection_t *collection);
void collection_scan_dir(collection_t *collection);

void collection_add_level(collection_t *collection, level_t *level);
bool collection_add_level_file(collection_t *collection, char *filename);

level_t *collection_find_level_by_filename(collection_t *collection, char *filename);

void collection_draw(collection_t *collection);

#endif /*COLLECTION_H*/

