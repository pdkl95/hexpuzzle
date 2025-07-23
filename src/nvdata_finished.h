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

#include "blueprint_string.h"
#include "classics.h"

struct level;

enum finished_level_flag {
    FINISHED_LEVEL_FLAG_NAME         = 0x0001,
    FINISHED_LEVEL_FLAG_WIN_TIME     = 0x0002,
    FINISHED_LEVEL_FLAG_ELAPSED_TIME = 0x0004,
    FINISHED_LEVEL_FLAG_BLUEPRINT    = 0x0008,
    FINISHED_LEVEL_FLAG_SOLVER       = 0x0010,
    FINISHED_LEVEL_FLAG_CLASSIC      = 0x0020,
    FINISHED_LEVEL_FLAG_FILEREF      = 0x0040,
    FINISHED_LEVEL_FLAG_COLLECTION   = 0x0080
};
typedef enum finished_level_flag finished_level_flag_t;

char *finished_level_flag_str(flags16_t flags);

typedef struct finished_level {
    char id[ID_MAXLEN];
    char name[NAME_MAXLEN];

    time_t win_time;
    elapsed_time_parts_t elapsed_time;

    union {
        blueprint_string_t blueprint;
        classic_level_nameref_t classic_nameref;
        fileref_t fileref;
    };

    flags16_t flags;

    char rb_color;
    struct finished_level *left;
    struct finished_level *right;
} finished_level;

#ifdef NDEBUG
static inline void assert_finished_level_flag_excludes(UNUSED struct finished_level *fl, UNUSED flags16_t incompatible_flags)
#else
static inline void assert_finished_level_flag_excludes(struct finished_level *fl, flags16_t incompatible_flags)
#endif
{
    assert(0 == (fl->flags & incompatible_flags));
}

static inline void finished_level_set_name(struct finished_level *fl, const char *value)
{
    snprintf(fl->name, NAME_MAXLEN, "%s", value);
    fl->flags |= FINISHED_LEVEL_FLAG_NAME;
}

static inline void finished_level_set_win_time(struct finished_level *fl, time_t value)
{
    fl->win_time = value;
    fl->flags |= FINISHED_LEVEL_FLAG_WIN_TIME;
}

static inline void finished_level_set_elapsed_time(struct finished_level *fl, elapsed_time_parts_t *value)
{
    fl->elapsed_time = *value;
    fl->flags |= FINISHED_LEVEL_FLAG_ELAPSED_TIME;
}

static inline void finished_level_set_blueprint(struct finished_level *fl, const char *value)
{
    assert_finished_level_flag_excludes(fl, FINISHED_LEVEL_FLAG_CLASSIC | FINISHED_LEVEL_FLAG_FILEREF);
    copy_blueprint_string(fl->blueprint, value);
    fl->flags |= FINISHED_LEVEL_FLAG_BLUEPRINT;
}

static inline void finished_level_set_solver(struct finished_level *fl)
{
    fl->flags |= FINISHED_LEVEL_FLAG_SOLVER;
}

static inline void finished_level_set_classic(struct finished_level *fl, name_str_t collection_id, unique_id_t level_unique_id)
{
    assert_finished_level_flag_excludes(fl, FINISHED_LEVEL_FLAG_BLUEPRINT | FINISHED_LEVEL_FLAG_FILEREF);
    copy_name(fl->classic_nameref.collection_id, collection_id);
    copy_unique_id(fl->classic_nameref.level_unique_id, level_unique_id);
    fl->flags |= FINISHED_LEVEL_FLAG_CLASSIC;
}

static inline void finished_level_set_collection(struct finished_level *fl)
{
    fl->flags |= FINISHED_LEVEL_FLAG_COLLECTION;
}

static inline void finished_level_set_fileref(struct finished_level *fl, filename_t filename, unique_id_t level_unique_id)
{
    assert_finished_level_flag_excludes(fl, FINISHED_LEVEL_FLAG_BLUEPRINT | FINISHED_LEVEL_FLAG_CLASSIC);
    copy_filename(fl->fileref.filename, filename);
    copy_unique_id(fl->fileref.level_unique_id, level_unique_id);
    fl->flags |= FINISHED_LEVEL_FLAG_FILEREF;
}

static inline void finished_level_clear_name(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_NAME;
}

static inline void finished_level_clear_win_time(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_WIN_TIME;
}

static inline void finished_level_clear_elapsed_time(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_ELAPSED_TIME;
}

static inline void finished_level_clear_blueprint(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_BLUEPRINT;
}

static inline void finished_level_clear_solver(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_SOLVER;
}

static inline void finished_level_clear_classic(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_CLASSIC;
}

static inline void finished_level_clear_fileref(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_FILEREF;
}

static inline void finished_level_clear_collection(struct finished_level *fl)
{
    fl->flags &= ~FINISHED_LEVEL_FLAG_COLLECTION;
}

static inline bool finished_level_has_name(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_NAME;
}

static inline bool finished_level_has_win_time(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_WIN_TIME;
}

static inline bool finished_level_has_elapsed_time(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_ELAPSED_TIME;
}

static inline bool finished_level_has_blueprint(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_BLUEPRINT;
}

static inline bool finished_level_has_solver(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_SOLVER;
}

static inline bool finished_level_has_classic(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_CLASSIC;
}

static inline bool finished_level_has_fileref(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_FILEREF;
}

static inline bool finished_level_has_collection(struct finished_level *fl)
{
    return fl->flags & FINISHED_LEVEL_FLAG_COLLECTION;
}

int compare_finished_level(struct finished_level *a, struct finished_level *b);

SGLIB_DEFINE_RBTREE_PROTOTYPES(finished_level, left, right, rb_color, compare_finished_level);

struct finished_levels {
    struct finished_level *tree;
    int count;
};
typedef struct finished_levels finished_levels_t;

void init_nvdata_finished(void);
void cleanup_nvdata_finished(void);
void nvdata_mark_id_finished(struct finished_level *entry);
void nvdata_mark_finished(struct level *level);
void nvdata_unmark_finished(struct level *level);
bool nvdata_is_finished(struct level *level);
void nvdata_finished_write(FILE *f);
void load_nvdata_finished_levels(void);
void save_nvdata_finished_levels(void);
void force_save_nvdata_finished_levels(void);
bool reset_nvdata_finished_levels(void);
bool have_nvdata_finished_levels_data(void);

extern char *nvdata_state_finished_levels_file_path;
extern char *nvdata_state_finished_levels_backup_file_path;

extern finished_levels_t finished_levels;

#endif /*NVDATA_FINISHED_H*/

