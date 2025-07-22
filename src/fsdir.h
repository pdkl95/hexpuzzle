/****************************************************************************
 *                                                                          *
 * fsdir.h                                                                  *
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

#ifndef FSDIR_H
#define FSDIR_H

#include "sglib/sglib.h"

typedef struct fsdir {
    int index;

    full_path_t path;

    bool exists;
    bool enabled;

    struct fsdir *next;
    struct fsdir *prev;
} fsdir;
typedef struct fsdir fsdir_t;

#define compare_fsdir(a, b) (((a)->index) - ((b)->index))

SGLIB_DEFINE_DL_LIST_PROTOTYPES(fsdir, compare_fsdir, prev, next);

fsdir_t *alloc_fsdir(void);
void free_fsdir(fsdir_t *fsdir);
void init_fsdir(fsdir_t *fsdir, const char *dirpath);
fsdir_t *create_fsdir(const char *dirpath);
void destroy_fsdir(fsdir_t *fsdir);

void init_search_dirs(void);
void cleanup_search_dirs(void);

void reset_search_dirs(void);
void add_search_dir(const char *dirpath);
void add_default_search_dir(void);
const char *find_file_in_search_dirs(const char *filename);

extern fsdir_t *search_dirs;
extern bool search_dirs_changed;

#endif /*FSDIR_H*/

