/****************************************************************************
 *                                                                          *
 * const.h                                                                  *
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

#ifndef CONST_H
#define CONST_H

#define COLLECTION_ZIP_INDEX_FILENAME "levels.index"

#define COLLECTION_FILENAME_EXT "hexlevelpack"
#define LEVEL_FILENAME_EXT      "hexlevel"

#define COLLECTION_DEFAULT_FILENAME_PREFIX "level-"
#define COLLECTION_DEFAULT_FILENAME_SUFFIX "." LEVEL_FILENAME_EXT

#define LEVEL_DEFAULT_NAME "Untitled"
#define LEVEL_DEFAULT_RADIUS LEVEL_MIN_RADIUS

#define LEVEL_MIN_RADIUS 1
#define LEVEL_MAX_RADIUS 4

#define TILE_RESET_TIME 0.35

#define TILE_LEVEL_WIDTH  ((2 * LEVEL_MAX_RADIUS) + 1)
#define TILE_LEVEL_HEIGHT TILE_LEVEL_WIDTH

#define LEVEL_MAXTILES (TILE_LEVEL_HEIGHT * TILE_LEVEL_WIDTH)

#define LEVEL_CENTER_POSITION  \
    ((hex_axial_t){            \
        .q = LEVEL_MAX_RADIUS, \
        .r = LEVEL_MAX_RADIUS  \
     })

#define LEVEL_FADE_TRANSITION_LENGTH 0.7
#define LEVEL_FADE_TRANSITION_FRAMES (LEVEL_FADE_TRANSITION_LENGTH * options->max_fps)
#define LEVEL_FADE_DELTA (1.0 / (LEVEL_FADE_TRANSITION_FRAMES))

#define PATH_TYPE_COUNT 5
#define NAME_MAXLEN 22
#define UI_NAME_MAXLEN  (6 + NAME_MAXLEN)

#define ID_MAXLEN 255

#define FINISHED_HUE_STEP 4.0
#define FINISHED_ANIM_PREDELAY_LENGTH 0.5
#define FINISHED_ANIM_LENGTH 5.0

// 1MB
#define LEVEL_SERIALIZE_BUFSIZE (1024 * 1024)



#ifndef RAYGUI_ICON_SIZE
#define RAYGUI_ICON_SIZE 16
#endif

#if !defined(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT)
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT        24
#endif

#define WINDOW_MARGIN RAYGUI_ICON_SIZE
#define PANEL_INNER_MARGIN 12

#define BUTTON_MARGIN 4
#define ICON_BUTTON_SIZE (RAYGUI_ICON_SIZE + (2 * BUTTON_MARGIN))
#define ICON_FONT_SIZE RAYGUI_ICON_SIZE

#define NAME_FONT         font20
#define NAME_FONT_SIZE    20
#define NAME_FONT_SPACING 2.0

#if 1
# define DEFAULT_GUI_FONT      font18
# define DEFAULT_GUI_FONT_SIZE 18
#else
# define DEFAULT_GUI_FONT      font20
# define DEFAULT_GUI_FONT_SIZE 20
#endif
#define DEFAULT_GUI_FONT_SPACING 2.0

#define CURSOR_MIN_SCALE 1
#define CURSOR_MAX_SCALE 4

#define PANEL_LABEL_FONT         font16
#define PANEL_LABEL_FONT_SIZE    16
#define PANEL_LABEL_FONT_SPACING 2.0

#define PANEL_ROUNDNES 0.2
#define BUTTON_ROUNDNES 0.1
#define TOOL_BUTTON_HEIGHT DEFAULT_GUI_FONT_SIZE
#define TOOL_BUTTON_WIDTH RAYGUI_ICON_SIZE

#define CREATE_DIR_MODE (S_IRWXU | S_IRWXG | (S_IROTH | S_IXOTH))

#define NVDATA_STATE_FILE_NAME "state.json"
#define NVDATA_FINISHED_LEVEL_FILE_NAME "finished_levels.dat"
#define NVDATA_DEFAULT_BROWSE_PATH_NAME "levels"

#ifndef MAX_FILEPATH_LENGTH
    #if defined(_WIN32)
        #define MAX_FILEPATH_LENGTH      256
    #else
        #define MAX_FILEPATH_LENGTH     4096
    #endif
#endif

#define SOLVER_SOLVE_SWAP_TIME 0.2
#define SOLVER_UNDO_SWAP_TIME  0.15

#endif /*CONST_H*/

