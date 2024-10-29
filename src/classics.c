/****************************************************************************
 *                                                                          *
 * classics.c                                                               *
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

#include "common.h"
#include "options.h"
#include "collection.h"
#include "classics.h"
#include "data_classics_levels.h"

void open_classics_game_pack(int n)
{
    assert(n >= 1);
    assert(n <= 4);

    unsigned char *data = NULL;
    unsigned int ulen = 0;
    char *filename = NULL;

    switch (n) {
    case 1:
        filename = "1-red.hexlevelpack";
        data = levels_classics_1_red_hexlevelpack;
        ulen = levels_classics_1_red_hexlevelpack_len;
        break;
    case 2:
        filename = "2-blue.hexlevelpack";
        data = levels_classics_2_blue_hexlevelpack;
        ulen = levels_classics_2_blue_hexlevelpack_len;
        break;
    case 3:
        filename = "3-green.hexlevelpack";
        data = levels_classics_3_green_hexlevelpack;
        ulen = levels_classics_3_green_hexlevelpack_len;
        break;
    case 4:
        filename = "4-yellow.hexlevelpack";
        data = levels_classics_4_yellow_hexlevelpack;
        ulen = levels_classics_4_yellow_hexlevelpack_len;
        break;
    }

    int len = (int)ulen;

    if (options->verbose) {
        infomsg("Load %d byte built-in level pack %d (\"%s\") from %p\n", len, n, filename, data);
    }

    collection_t *collection = load_collection_pack_compressed_data(filename, data, len);
    if (!collection) {
        errmsg("Error loading \"%s\"", filename);
        return;
    }

    if (current_collection) {
        destroy_collection(current_collection);
    }
    current_collection = collection;
    set_game_mode(GAME_MODE_PLAY_COLLECTION);
}

