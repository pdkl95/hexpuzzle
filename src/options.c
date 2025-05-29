/******************************************************************************
 *                                                                            *
 *  options.c                                                                 *
 *                                                                            *
 *  This file is part of hexpuzzle.                                           *
 *                                                                            *
 *  hexpuzzle is free software: you can redistribute it and/or                *
 *  modify it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation, either version 3 of the License,         *
 *  or (at your option) any later version.                                    *
 *                                                                            *
 *  hexpuzzle is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  *
 *  Public License for more details.                                          *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License along   *
 *  with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.              *
 *                                                                            *
 ******************************************************************************/

#include "common.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else
# warning "Missing <getopt.h> - trying to compile with our \"gnugetopt.h\""
# warning "This fallback is untested, and may not work!"
# include "gnugetopt.h"
#endif

#include <limits.h>
#include <time.h>


#include "options.h"

options_t *options = NULL;

/* command line options */
static char short_options[] = "Cc:e:EF:H:p:s:SwvVW:PR:UL::r::hj";

static struct option long_options[] = {
    {  "create-random-level",       no_argument, 0, 'L' },
    {   "create-blank-level",       no_argument, 0, 'K' },
    {            "no-config", required_argument, 0, 'C' },
    {           "config-dir", required_argument, 0, 'c' },
    {                  "fps", required_argument, 0, 'F' },
    {               "height", required_argument, 0, 'H' },
    {                "width", required_argument, 0, 'W' },
    {         "level-radius", required_argument, 0, 'R' },
    {      "level-min-fixed", required_argument, 0, '[' },
    {      "level-max-fixed", required_argument, 0, ']' },
    {     "level-min-hidden", required_argument, 0, '(' },
    {     "level-max-hidden", required_argument, 0, ')' },
    {                 "seed", required_argument, 0, 's' },
    {                 "play", required_argument, 0, 'p' },
    {               "random", optional_argument, 0, 'r' },
    {                 "edit", required_argument, 0, 'e' },
    {        "cheat-autowin",       no_argument, 0, 'A' },
    {         "cheat-solver",       no_argument, 0, 'S' },
    {                "force",       no_argument, 0, '!' },
    {                 "pack",       no_argument, 0, 'P' },
    {               "unpack",       no_argument, 0, 'U' },
    {           "animate-bg",       no_argument, 0, 'b' },
    {        "no-animate-bg",       no_argument, 0, 'B' },
    {          "animate-win",       no_argument, 0, 'i' },
    {       "no-animate-win",       no_argument, 0, 'I' },
#ifdef USE_PHYSICS
    {          "use-physics",       no_argument, 0, 'y' },
    {       "no-use-physics",       no_argument, 0, 'Y' },
#endif
    {   "use-shader-effects",       no_argument, 0, 'x' },
    {"no-use-shader-effects",       no_argument, 0, 'X' },
    {        "show-previews",       no_argument, 0, '-' },
    {     "no-show-previews",       no_argument, 0, '_' },
    {        "show-tooltips",       no_argument, 0, 't' },
    {     "no-show-tooltips",       no_argument, 0, 'T' },
    {       "extra-rainbows",       no_argument, 0, ':' },
    {    "no-extra-rainbows",       no_argument, 0, ';' },
    {      "allow-edit-mode",       no_argument, 0, 'E' },
    {                 "demo",       no_argument, 0, 'd' },
    {             "demo-win",       no_argument, 0, 'D' },
    {          "wait-events",       no_argument, 0, 'w' },
    {              "verbose",       no_argument, 0, 'v' },
    {              "version",       no_argument, 0, 'V' },
    {                 "help",       no_argument, 0, 'h' },
    {           "help-cheat",       no_argument, 0, '*' },
    {                      0,                 0, 0,  0  }
};

static char usage_args[] = "[<file>." LEVEL_FILENAME_EXT  " | <file>." COLLECTION_FILENAME_EXT "]";

static char help_text[] =
    "\n"
    "Hex Puzzle\n"
    "----------\n"
    "\n"
    "A hex-tile based puzzle game.\n"
    "\n"
    "OPTIONS\n"
    "  -c, --config-dir=DIRECTORY  Directory to save/load persistant data.\n"
    "                              (default: ${XDG_CONFIG_HOME}/" PACKAGE_NAME "/\n"
    "                                which is usually ~/.config/" PACKAGE_NAME "/)\n"
    "  -C, --no-config             Skip loading of all config files (\"Safe Mode\")\n"
    "\n"
    "      --allow-edit-mode       Enable the edit button when browsing local files\n"
    "\n"
    "  -v, --verbose               More logging output (including raylib)\n"
    "\n"
    "  -V, --version               Show version information and exit\n"
    "  -h, --help                  Show this help and exit\n"
    "\n"
    "GRAPHICS OPTIONS\n"
    "      --animate-bg            Enable background animation (default: on)\n"
    "   --no-animate-bg            Disable background animation\n"
    "      --animate-win           Enable animation on level win (default: on)\n"
    "   --no-animate-win           Disable animation on level win\n"
    "      --use-physics           Enable the physics engine (default: on)\n"
    "   --no-use-physics           Disable the physucs engine\n"
    "      --use-shader-effects    Enable shader postprocesing effects (default: on)\n"
    "   --no-use-shader-effects    Disable shader postprocesing effects\n"
    "      --show-previews         Enable showing small level previews (default: on)\n"
    "   --no-show-previews         Disable showing small level previews\n"
    "      --show-tooltips         Enable popup tooltips on mouse-hover default: on)\n"
    "   --no-show-tooltips         Disable popup tooltips\n"
    "\n"
    "  -W, --width=NUMBER          Window width (default: " STR(OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH) ")\n"
    "  -H, --height=NUMBER         Window height (default: " STR(OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT) ")\n"
    "  -F, --fps=NUMBER            Maximum FPS (default: " STR(OPTIONS_DEFAULT_MAX_FPS) ")\n"
    "\n"
    "  -w, --wait-events           Use GLFW's WaitEvents to let the program\n"
    "                              sleep instead of polling for events\n"
    "                              every frame. Less CPU usage, but might\n"
    "                              not awaken immediately during resize events.\n"
    "\n"
    "ACTIONS\n"
    "  -p, --play <file>                Play the given ." LEVEL_FILENAME_EXT "\n"
    "                                     or ." COLLECTION_FILENAME_EXT " file\n"
    "  -r, --random[=SEED]              Play a random level, optionally using\n"
    "                                     a given RNG seed (see --seed)\n"
    "  -e, --edit <file>                Edit the given ." LEVEL_FILENAME_EXT "\n"
    "                                     or ." COLLECTION_FILENAME_EXT " file\n"
    "      --create-blank-level <NAME>  Create a new level file that is blank.\n"
    "      --create-random-level <NAME> Create a new level file that is\n"
    "                                     randomly generated puzzle \n"
    "  -P, --pack <dir>                 Packasge a directory of ." LEVEL_FILENAME_EXT " files\n"
    "                                     into a ." COLLECTION_FILENAME_EXT "\n"
    "  -U, --unpack <file." COLLECTION_FILENAME_EXT "> Unpack a " COLLECTION_FILENAME_EXT " file\n"
    "                                     into a directory of ." LEVEL_FILENAME_EXT "\n"
    "      --demo                       Show an auto-solving demo (\"attract\") mode.\n"
    "                                     No user input accepted except SPACE to advance\n"
    "                                     the demo and ESC/q to quit.\n"
    "      --demo-win                   Same as --demo, but only show the win animations.\n"
    "\n"
    "ACTION OPTIONS\n"
    "      --force                   Allow files to be overwritten (dangerous!)\n"
    "  -s, --seed <SEED_INT_OR_STR>    Set the RNG seed used for level creation.\n"
    "                                  Integers are used directly as the seed;\n"
    "                                  non-integer strings are hashed.\n"
    "  -R, --level-radius=NUMBER     Tile radius of created levels.\n"
    "                                  Min: " STR(LEVEL_MIN_RADIUS) ", Max: " STR(LEVEL_MAX_RADIUS) ", Default: " STR(OPTIONS_DEFAULT_CREATE_LEVEL_RADIUS) "\n"
    "      --level-min-fixed=NUMBER  Minimum number of fixed tiles.  (default: " STR(OPTIONS_DEFAULT_CREATE_LEVEL_MIN_FIXED) ")\n"
    "      --level-max-fixed=NUMBER  Maximum number of fixed tiles.  (default: " STR(OPTIONS_DEFAULT_CREATE_LEVEL_MAX_FIXED) ")\n"
    "      --level-min-hidden=NUMBER Minimum number of hidden tiles. (default: " STR(OPTIONS_DEFAULT_CREATE_LEVEL_MIN_HIDDEN) ")\n"
    "      --level-max-hidden=NUMBER Maximum number of hidden tiles. (default: " STR(OPTIONS_DEFAULT_CREATE_LEVEL_MAX_HIDDEN) ")\n"
    ;

static char help_cheat_text[] =
    "CHEAT OPTIONS\n"
    "      --cheat-autowin         Skip solving the puzzle; jump straight to the\n"
    "                                solved state and win animation.\n"
    "  -S, --cheat-solver          Enable the auto-solver hotkeys:\n"
    "                                <F5> Toggle running the solver\n"
    "                                <F6> Stop the solver\n"
    "                                <F7> Reverse (UNDO) the direction of the solver\n"
    ;

void
show_usage(
    char *usage_args
) {
    printf("Usage: %s [OPTIONS] %s\n",
           progname, usage_args);
}

void
show_help(
    char *help_text,
    char *usage_args
) {
    show_usage(usage_args);
    printf("%s\n", help_text);
}

void
show_help_cheat(
    char *help_text,
    char *usage_args
) {
    show_help(help_text, usage_args);
    printf("%s\n", help_cheat_text);
}

void
show_version(
    void
) {
    printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    printf("%s\n", PACKAGE_COPYRIGHT_STRING);
    printf("License %s: %s\n", PACKAGE_LICENCE, PACKAGE_LICENCE_DESC);
    printf("This is free software: you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n");
    printf("\n");
    printf("Written by %s.\n", PACKAGE_AUTHOR);
    printf("%s\n", PACKAGE_URL);
}

static options_t *
alloc_options(
    void
) {
    options_t *options = calloc(1, sizeof(options_t));

    options->nvdata_dir = NULL;

    return options;
}

static void
free_options(
    options_t *options
) {
    if (options) {
        SAFEFREE(options->nvdata_dir);
        FREE(options);
    }
}

options_t *
create_options(
    void
) {
    options_t *options = alloc_options();

    options_set_defaults(options);

    return options;
}

void
destroy_options(
    options_t *options
) {
    if (options) {
        free_options(options);
    }
}

static void
options_set_string(
    char **opt
) {
    assert_not_null(opt);
    assert_not_null(optarg);
    const char *src = optarg;

    if (*opt != NULL) {
        FREE(*opt);
    }

    *opt = strdup(src);
}

static void
options_set_long(
    long *opt
) {
    assert_not_null(opt);
    assert_not_null(optarg);

    const char *src = optarg;
    char *endptr;

    errno = 0;
    *opt = strtol(src, &endptr, 10);

    if (  ( errno == ERANGE &&
            (  *opt == LONG_MAX ||
               *opt == LONG_MIN )  ) ||
          ( errno != 0 && *opt == 0)
    ) {
        perror("strtol");
        DIE("cannot parse numewric option");
    }

    if (endptr == src) {
        DIE("No digits were found");
    }
}

static bool
options_set_long_bounds(
    long *opt,
    long min,
    long max
) {
    long value = 0;
    options_set_long(&value);
    if ((value < min) || (value > max)) {
        return false;
    } else {
        *opt = value;
        return true;
    }
}

static void
options_set_int(
    int *opt
) {
    assert_not_null(opt);
    assert_not_null(optarg);

    const char *src = optarg;
    char *endptr;

    long value = 0;

    errno = 0;
    value = strtol(src, &endptr, 10);

    if (  ( errno == ERANGE &&
            (  value == LONG_MAX ||
               value == LONG_MIN )  ) ||
          ( errno != 0 && value == 0)
    ) {
        perror("strtol");
        DIE("cannot parse numewric option");
    }

    if (endptr == src) {
        DIE("No digits were found");
    }

    *opt = (int)value;
}

static bool
options_set_int_bounds(
    int *opt,
    int min,
    int max
) {
    int value = 0;
    options_set_int(&value);
    if ((value < min) || (value > max)) {
        return false;
    } else {
        *opt = value;
        return true;
    }
}

void
options_set_defaults(
    options_t *options
) {
    assert_this(options);

    options->verbose               = OPTIONS_DEFAULT_VERBOSE;
    options->wait_events           = OPTIONS_DEFAULT_WAIT_EVENTS;

    options->allow_edit_mode       = OPTIONZ_DEFAULT_ALLOW_EDIT_MODE;

    options->animate_bg            = OPTIONS_DEFAULT_ANIMATE_BG;
    options->animate_win           = OPTIONS_DEFAULT_ANIMATE_WIN;
    options->use_physics           = OPTIONS_DEFAULT_USE_PHYSICS;
    options->use_postprocessing    = OPTIONS_DEFAULT_USE_POSTPROCESSING;
    options->show_level_previews   = OPTIONS_DEFAULT_SHOW_LEVEL_PREVIEWS;
    options->show_tooltips         = OPTIONS_DEFAULT_SHOW_TOOLTIPS;
    options->extra_rainbows        = OPTIONS_DEFAULT_EXTRA_RAINBOWS;
    options->max_fps               = OPTIONS_DEFAULT_MAX_FPS;
    options->initial_window_width  = OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH;
    options->initial_window_height = OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT;
    options->cursor_scale          = OPTIONS_DEFAULT_CURSOR_SCALE;
    options->double_click_ms       = OPTIONS_DEFAULT_DOUBLE_CLICK_MS;
    options->max_win_radius        = OPTIONS_DEFAULT_MAX_WIN_RADIUS;

    options->load_state_animate_bg  = true;
    options->load_state_animate_win = true;
    options->load_state_use_physics = true;
    options->load_state_use_postprocessing = true;
    options->load_state_show_level_previews = true;
    options->load_state_show_tooltips = true;
    options->load_color_opt = true;

    options->safe_mode = false;

    options->force = false;

    options->startup_action             = OPTIONS_DEFAULT_STARTUP_ACTION;
    options->create_level_mode          = OPTIONS_DEFAULT_CREATE_LEVEL_MODE;
    options->create_level_radius        = OPTIONS_DEFAULT_CREATE_LEVEL_RADIUS;
    options->create_level_fixed         = OPTIONS_DEFAULT_CREATE_LEVEL_FIXED;
    options->create_level_hidden        = OPTIONS_DEFAULT_CREATE_LEVEL_HIDDEN;
    options->create_level_expoints      = OPTIONS_DEFAULT_CREATE_LEVEL_EXPOINTS;
    options->create_level_symmetry_mode = OPTIONS_DFFAULT_CREATE_LEVEL_SYMMETRY_MODE;
    options->create_level_minimum_path_density = OPTIONS_DFFAULT_CREATE_LEVEL_MINIMUM_PATH_DENSITY;

    options->load_state_create_level_mode          = true;
    options->load_state_create_level_radius        = true;
    options->load_state_create_level_fixed_min     = true;
    options->load_state_create_level_fixed_max     = true;
    options->load_state_create_level_hidden_min    = true;
    options->load_state_create_level_hidden_max    = true;
    options->load_state_create_level_symmetry_mode = true;
    options->load_state_create_level_minimum_path_density = true;

    color_option_set(&(options->path_color[0]), OPTIONS_DEFAULT_PATH_COLOR_0);
    color_option_set(&(options->path_color[1]), OPTIONS_DEFAULT_PATH_COLOR_1);
    color_option_set(&(options->path_color[2]), OPTIONS_DEFAULT_PATH_COLOR_2);
    color_option_set(&(options->path_color[3]), OPTIONS_DEFAULT_PATH_COLOR_3);
    color_option_set(&(options->path_color[4]), OPTIONS_DEFAULT_PATH_COLOR_4);

    options->path_color[0].highlight_color = OPTIONS_DEFAULT_PATH_HL_COLOR_0;
    options->path_color[1].highlight_color = OPTIONS_DEFAULT_PATH_HL_COLOR_1;
    options->path_color[2].highlight_color = OPTIONS_DEFAULT_PATH_HL_COLOR_2;
    options->path_color[3].highlight_color = OPTIONS_DEFAULT_PATH_HL_COLOR_3;
    options->path_color[4].highlight_color = OPTIONS_DEFAULT_PATH_HL_COLOR_4;

    float dim_factor = -0.25;
    for (int i=0; i<PATH_TYPE_COUNT; i++) {
        options->path_color[i].color =
            ColorBrightness(options->path_color[i].color, dim_factor);
        options->path_color[i].default_color = options->path_color[i].color;
    }

    options->file_path = NULL;
    options->rng_seed_str = NULL;

    options->cheat_autowin = false;
    options->cheat_solver  = false;

    if (options->nvdata_dir) {
        options->nvdata_dir = NULL;
    }

    time_t current_time = time(NULL);
    struct tm tm;
    if (localtime_r(&current_time, &tm)) {
        if (tm.tm_mon == 5) {
            // pride month
            infomsg("It's pride month! Automatically enabling --extra-rainbows");
            options->extra_rainbows = true;
        }
    } else {
        errmsg("localtime_r failed: %s", strerror(errno));
    }
}

bool
options_parse_args(
    options_t *options,
    int argc,
    char *argv[]
) {
    int c;

    for (;;) {
        int option_index = 0;

        c = getopt_long(argc, argv, short_options, long_options, &option_index);

        if (-1 == c) {
            break;
        }

        switch (c) {
        case 'p':
            options_set_string(&options->file_path);
            options->startup_action = STARTUP_ACTION_PLAY;
            break;

        case 'r':
            if (optarg) {
                options_set_string(&options->rng_seed_str);
            }
            options->startup_action = STARTUP_ACTION_RANDOM;
            break;

        case 's':
            options_set_string(&options->rng_seed_str);
            break;

        case 'e':
            options_set_string(&options->file_path);
            options->startup_action = STARTUP_ACTION_EDIT;
            break;

        case 'E':
            options->allow_edit_mode = true;
            break;

        case '!':
            options->force = true;
            break;

        case 'A':
            options->cheat_autowin = true;
            warnmsg("CHEAT ENABLED: auto-win levels");
            break;

        case 'S':
            options->cheat_solver = true;
            warnmsg("CHEAT ENABLED: auto-solver tool is available");
            break;

        case 'L':
            options->create_level_mode = CREATE_LEVEL_MODE_RANDOM;
            options->startup_action = STARTUP_ACTION_CREATE_LEVEL;
            break;

        case 'K':
            options->create_level_mode = CREATE_LEVEL_MODE_BLANK;
            options->startup_action = STARTUP_ACTION_CREATE_LEVEL;
            break;

        case 'R':
            if (!options_set_long_bounds(&options->create_level_radius, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS)) {
                errmsg("bad value for --level-radius (expected %d - %d)",
                       LEVEL_MIN_RADIUS,
                       LEVEL_MAX_RADIUS);
                return false;
            }
            options->load_state_create_level_radius = false;
            break;

        case '[':
            if (!options_set_int_bounds(&options->create_level_fixed.min,
                                        LEVEL_MIN_FIXED,
                                        LEVEL_MAX_FIXED)) {
                errmsg("bad value for --level-min-fixed (expected %d - %d)",
                       LEVEL_MIN_FIXED,
                       LEVEL_MAX_FIXED);
                return false;
            }
            options->load_state_create_level_fixed_min = false;
            if (options->create_level_fixed.max < options->create_level_fixed.min) {
                options->create_level_fixed.max = options->create_level_fixed.min;
                options->load_state_create_level_fixed_max = false;
            }
            break;

        case ']':
            if (!options_set_int_bounds(&options->create_level_fixed.max,
                                        LEVEL_MIN_FIXED,
                                        LEVEL_MAX_FIXED)) {
                errmsg("bad value for --level-max-fixed (expected %d - %d)",
                       LEVEL_MIN_FIXED,
                       LEVEL_MAX_FIXED);
                return false;
            }
            options->load_state_create_level_fixed_max = false;
            if (options->create_level_fixed.min > options->create_level_fixed.max) {
                options->create_level_fixed.min = options->create_level_fixed.max;
                options->load_state_create_level_fixed_min = false;
            }
            break;

        case '(':
            if (!options_set_int_bounds(&options->create_level_hidden.min,
                                        LEVEL_MIN_HIDDEN,
                                        LEVEL_MAX_HIDDEN)) {
                errmsg("bad value for --level-min-hidden (expected %d - %d)",
                       LEVEL_MIN_HIDDEN,
                       LEVEL_MAX_HIDDEN);
                return false;
            }
            options->load_state_create_level_hidden_min = false;
            if (options->create_level_hidden.max < options->create_level_hidden.min) {
                options->create_level_hidden.max = options->create_level_hidden.min;
                options->load_state_create_level_hidden_max = false;
            }
            break;

        case ')':
            if (!options_set_int_bounds(&options->create_level_hidden.max,
                                        LEVEL_MIN_HIDDEN,
                                        LEVEL_MAX_HIDDEN)) {
                errmsg("bad value for --level-max-hidden (expected %d - %d)",
                       LEVEL_MIN_HIDDEN,
                       LEVEL_MAX_HIDDEN);
                return false;
            }
            options->load_state_create_level_hidden_max = false;
            if (options->create_level_hidden.min > options->create_level_hidden.max) {
                options->create_level_hidden.min = options->create_level_hidden.max;
                options->load_state_create_level_hidden_min = false;
            }
            break;

        case 'P':
            options->startup_action = STARTUP_ACTION_PACK_COLLECTION;
            break;

        case 'U':
            options->startup_action = STARTUP_ACTION_UNPACK_COLLECTION;
            break;

        case 'C':
            options->safe_mode = true;
            break;

        case 'c':
            options_set_string(&options->nvdata_dir);
            break;

        case 'F':
            options_set_long(&options->max_fps);
            break;

        case 'W':
            options_set_long(&options->initial_window_width);
            break;

        case 'H':
            options_set_long(&options->initial_window_height);
            break;

        case 'b':
            options->animate_bg = true;
            options->load_state_animate_bg = false;
            break;

        case 'B':
            options->animate_bg = false;
            options->load_state_animate_bg = false;
            break;

        case 'i':
            options->animate_win = true;
            options->load_state_animate_win = false;
            break;

        case 'I':
            options->animate_win = false;
            options->load_state_animate_win = false;
            break;
#ifdef USE_PHYSICS
        case 'y':
            options->use_physics = true;
            options->load_state_use_physics = false;
            break;

        case 'Y':
            options->use_physics = false;
            options->load_state_use_physics = false;
            break;
#endif
        case 'x':
            options->use_postprocessing = true;
            options->load_state_use_postprocessing = false;
            break;

        case 'X':
            options->use_postprocessing = false;
            options->load_state_use_postprocessing = false;
            break;

        case '-':
            options->show_level_previews = true;
            options->load_state_show_level_previews = false;
            break;

        case '_':
            options->show_level_previews = false;
            options->load_state_show_level_previews = false;
            break;

        case 't':
            options->show_tooltips = true;
            options->load_state_show_tooltips = false;
            break;

        case 'T':
            options->show_tooltips = false;
            options->load_state_show_tooltips = false;
            break;

        case ':':
            options->extra_rainbows = true;
            break;

        case ';':
            options->extra_rainbows = false;
            break;

        case 'd':
            options->startup_action = STARTUP_ACTION_DEMO_SOLVE;
            break;

        case 'D':
            options->startup_action = STARTUP_ACTION_DEMO_WIN_ANIM;
            break;

        case 'w':
            options->wait_events = true;
            break;

        case 'v':
            options->verbose = true;
            break;

        case 'V':
            show_version();
            exit(0);
            break;

        case 'h':
            show_help(help_text, usage_args);
            exit(0);
            break;

        case '*':
            show_help_cheat(help_text, usage_args);
            exit(0);
            break;

        default:
            errmsg("getopt returned character code 0%o", c);
            return false;
        }
    }

    if (optind < argc) {
        options->extra_argc = argc - optind;
        options->extra_argv = calloc(options->extra_argc, sizeof(char *));
        int oi = optind;
        int ei = 0;
        for (; oi<argc; oi++, ei++) {
            options->extra_argv[ei] = strdup(argv[oi]);
        }
    }

    return true;
}
