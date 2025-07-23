/****************************************************************************
 *                                                                          *
 * fsdir.c                                                                  *
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
#include "nvdata.h"
#include "fsdir.h"

SGLIB_DEFINE_DL_LIST_FUNCTIONS(fsdir, compare_fsdir, prev, next);

fsdir_t *search_dirs = NULL;
bool search_dirs_changed = false;;

fsdir_t *alloc_fsdir(void)
{
    return calloc(1, sizeof(fsdir_t));
}

void free_fsdir(fsdir_t *fsdir)
{
    if (fsdir) {
        free(fsdir);
    }
}

void init_fsdir(fsdir_t *fsdir, const char *dirpath)
{
    assert_not_null(fsdir);
    assert_not_null(dirpath);

    snprintf(fsdir->path, FULL_PATH_MAXLEN, "%s", dirpath);

    fsdir->exists = DirectoryExists(fsdir->path);
    fsdir->enabled = fsdir->exists;
}

fsdir_t *create_fsdir(const char *dirpath)
{
    fsdir_t *fsdir = alloc_fsdir();
    init_fsdir(fsdir, dirpath);
    return fsdir;
}

void destroy_fsdir(fsdir_t *fsdir)
{
    free_fsdir(fsdir);
}


/*************************************************************************/

void init_search_dirs(void)
{
    search_dirs = NULL;
    search_dirs_changed = false;
}

void cleanup_search_dirs(void)
{
    if (search_dirs) {
        destroy_fsdir(search_dirs);
        search_dirs = NULL;
    }
}

void reset_search_dirs(void)
{
    if (options->verbose) {
        infomsg("resetting search_dirs to an empty list");
    }

    if (search_dirs) {
        fsdir_t *p = search_dirs->prev;
        while (p) {
            fsdir_t *fsdir = p;
            p = p->prev;
            destroy_fsdir(fsdir);
        }

        p = search_dirs->next;
        while (p) {
            fsdir_t *fsdir = p;
            p = p->next;
            destroy_fsdir(fsdir);
        }

        destroy_fsdir(search_dirs);
        search_dirs = NULL;
    }
}

fsdir_t *find_current_search_dir(const char *dirpath)
{
    assert_not_null(dirpath);

    struct fsdir *e = NULL;
    struct sglib_fsdir_iterator it;

    for(e = sglib_fsdir_it_init(&it, search_dirs);
        e != NULL;
        e = sglib_fsdir_it_next(&it)
    ) {
        if (0 == strcmp(e->path, dirpath)) {
            return e;
        }
    }

    return NULL;
}

bool move_search_dir(const char *oldpath, const char *newpath)
{
    assert_not_null(newpath);

    if (!oldpath) {
        return add_search_dir(newpath);
    }

    fsdir_t *newdir = find_current_search_dir(newpath);
    if (newdir) {
        if (options->verbose) {
            warnmsg("Cannot move search path \"%s\" to \"%s\" - new path already exists",
                    oldpath, newpath);
        }
        return false;
    }

    fsdir_t *olddir = find_current_search_dir(oldpath);
    if (!olddir) {
        errmsg("Cannot move search path \"%s\" to \"%s\" - old path does not exist",
               oldpath, newpath);
        return false;
    }

    if (options->verbose) {
        infomsg("changed search dir from \"%s\" to \"%s\"",
                oldpath, newpath);
    }

    copy_full_path(olddir->path, newpath);

    return true;
}

bool add_search_dir(const char *dirpath)
{
    fsdir_t *olddir = find_current_search_dir(dirpath);
    if (olddir) {
        return false;
    }

    fsdir_t *dir = create_fsdir(dirpath);
    dir->index = sglib_fsdir_len(search_dirs);
    sglib_fsdir_add(&search_dirs, dir);
    sglib_fsdir_sort(&search_dirs);

    search_dirs_changed = true;

    if (options->verbose) {
        infomsg("add search dir: \"%s\"", dir->path);
    }

    return true;
}

void add_default_search_dir(void)
{
    add_search_dir(nvdata_default_browse_path);
}

const char *find_file_in_search_dirs(const char *filename)
{
    struct fsdir *e = NULL;
    struct sglib_fsdir_iterator it;

    for(e = sglib_fsdir_it_init(&it, search_dirs);
        e != NULL;
        e = sglib_fsdir_it_next(&it)
    ) {
        const char *path = concat_dir_and_filename(e->path, filename);
        if (FileExists(path)) {
            return path;
        }
    }
    return NULL;
}

