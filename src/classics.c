/****************************************************************************
 *                                                                          *
 * classics.c                                                               *
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
#include "collection.h"
#include "level.h"
#include "classics.h"
#include "data_classics_levels.h"

classic_collection_t classic_collections[] = {
    {
        .index      = 1,
        .id         = "classic/1-red",
        .filename   = "1-red.hexlevelpack",
        .data       = levels_classics_1_red_hexlevelpack,
        .dlen       = &levels_classics_1_red_hexlevelpack_len,
        .collection = NULL
    },
    {
        .index      = 2,
        .id         = "classic/2-blue",
        .filename   = "2-blue.hexlevelpack",
        .data       = levels_classics_2_blue_hexlevelpack,
        .dlen       = &levels_classics_2_blue_hexlevelpack_len,
        .collection = NULL
    },
    {
        .index      = 3,
        .id         = "classic/3-green",
        .filename   = "3-green.hexlevelpack",
        .data       = levels_classics_3_green_hexlevelpack,
        .dlen       = &levels_classics_3_green_hexlevelpack_len,
        .collection = NULL
    },
    {
        .index      = 4,
        .id         = "classic/4-yellow",
        .filename   = "4-yellow.hexlevelpack",
        .data       = levels_classics_4_yellow_hexlevelpack,
        .dlen       = &levels_classics_4_yellow_hexlevelpack_len,
        .collection = NULL
    }
};
#define NUM_CLASSIC_COLLECTIONS ((int)NUM_ELEMENTS(classic_collection_t, classic_collections))

static classic_collection_t *find_classic_collection_by_index(int n)
{
    for (int i=0; i<NUM_CLASSIC_COLLECTIONS; i++) {
        classic_collection_t *cc = &(classic_collections[i]);
        if (cc->index == n) {
            return cc;
        }
    }
    return NULL;
}

static classic_collection_t *find_classic_collection_by_id(const char *id)
{
    for (int i=0; i<NUM_CLASSIC_COLLECTIONS; i++) {
        classic_collection_t *cc = &(classic_collections[i]);
        if (0 == strcmp(cc->id, id)) {
            return cc;
        }
    }
    return NULL;
}

static void load_classic_collection(classic_collection_t *cc)
{
    assert_not_null(cc);

    if (cc->collection) {
        return;
    }

    int len = (int)(*cc->dlen);

    if (options->verbose) {
        infomsg("Load %d byte built-in level pack %d (\"%s\") from %p\n",
                len, cc->index, cc->filename, cc->data);
    }

    cc->collection = load_collection_pack_compressed_data(cc->filename, cc->data, *cc->dlen);
    if (!cc->collection) {
        errmsg("Error loading \"%s\"", cc->filename);
        return;
    }

    cc->collection->is_classic      = true;
    cc->collection->prevent_destroy = true;

    for (level_t *p = cc->collection->levels;
         p;
         p = p->next)
    {
        p->classic_collection = cc;
    }

    collection_set_id(cc->collection, cc->id);
}

void open_classics_game_pack(int n)
{
    assert(n >= 1);
    assert(n <= NUM_CLASSIC_COLLECTIONS);

    classic_collection_t *cc = find_classic_collection_by_index(n);
    load_classic_collection(cc);
    assert_not_null(cc->collection);

    if (current_collection) {
        destroy_collection(current_collection);
    }
    current_collection = cc->collection;
    set_game_mode(GAME_MODE_PLAY_COLLECTION);
}

struct level *find_classic_level_by_nameref(classic_level_nameref_t *ref)
{
    classic_collection_t *cc = find_classic_collection_by_id(ref->collection_id);
    if (!cc) {
        return NULL;
    }

    load_classic_collection(cc);
    assert_not_null(cc->collection);

    return collection_find_level_by_unique_id(cc->collection, ref->level_unique_id);
}

#define NAMEREF_STRING_MAXLEN (sizeof(name_str_t) + sizeof(unique_id_t) + 1)
#define NAMEREF_STRING_BUFSIZE (NAMEREF_STRING_MAXLEN + 1)
const char *classic_level_nameref_string(classic_level_nameref_t *ref)
{
    static char buf[NAMEREF_STRING_BUFSIZE];
    snprintf(buf, NAMEREF_STRING_BUFSIZE, "%s:%s", ref->collection_id, ref->level_unique_id);
    return buf;
}

bool play_classic_level_nameref(classic_level_nameref_t *ref)
{
    level_t *level = find_classic_level_by_nameref(ref);
    if (!level) {
        errmsg("load failed for classic nameref \"%s\"",
               classic_level_nameref_string(ref));
        return false;
    }

    level_play(level);
    return true;
}
