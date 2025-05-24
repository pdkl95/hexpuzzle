/****************************************************************************
 *                                                                          *
 * nvdata_finished.h                                                        *
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

#ifndef NVDATA_FINISHED_H
#define NVDATA_FINISHED_H

#include "sglib/sglib.h"

struct level;

typedef struct finished_level {
    char id[ID_MAXLEN];

    char rb_color;
    struct finished_level *left;
    struct finished_level *right;
} finished_level;

int compare_finished_level(struct finished_level *a, struct finished_level *b);

SGLIB_DEFINE_RBTREE_PROTOTYPES(finished_level, left, right, rb_color, compare_finished_level);

struct finished_levels {
    struct finished_level *tree;
};
typedef struct finished_levels finished_levels_t;

void init_nvdata_finished(void);
void cleanup_nvdata_finished(void);
void nvdata_mark_id_finished(char *id);
void nvdata_mark_finished(struct level *level);
void nvdata_unmark_finished(struct level *level);
bool nvdata_is_finished(struct level *level);
void nvdata_finished_write(FILE *f);
void load_nvdata_finished_levels(void);
void save_nvdata_finished_levels(void);
bool reset_nvdata_finished_levels(void);
bool have_nvdata_finished_levels_data(void);

extern char *nvdata_state_finished_levels_file_path;
extern char *nvdata_state_finished_levels_backup_file_path;

#endif /*NVDATA_FINISHED_H*/

