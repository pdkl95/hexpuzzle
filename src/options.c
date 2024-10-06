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

#include "options.h"

/* command line options */
static char short_options[] = "Cc:F:H:t:wvVW:hj";

static struct option long_options[] = {
    {      "no-config", required_argument, 0, 'C' },
    {     "config-dir", required_argument, 0, 'c' },
    {            "fps", required_argument, 0, 'F' },
    {         "height", required_argument, 0, 'H' },
    {          "width", required_argument, 0, 'W' },
    {     "animate-bg",       no_argument, 0, 'b' },
    {  "no-animate-bg",       no_argument, 0, 'B' },
    {    "animate-win",       no_argument, 0, 'i' },
    { "no-animate-win",       no_argument, 0, 'I' },
    {    "wait-events",       no_argument, 0, 'w' },
    {        "verbose",       no_argument, 0, 'v' },
    {        "version",       no_argument, 0, 'V' },
    {           "help",       no_argument, 0, 'h' },
    {                0,                 0, 0,  0  }
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
    "  -C, --no-config             Skip loading of all config files (\"Save Mode\")\n"
    "\n"
    "      --animate-bg\n"
    "   --no-animate-bg\n"
    "      --animate-win\n"
    "   --no-animate-win\n"
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
    "  -v, --verbose               More logging output (including raylib)\n"
    "\n"
    "  -V, --version               Show version information and exit\n"
    "  -h, --help                  Show this help and exit\n"
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
    printf("%s", help_text);
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
    char **opt,
    const char *src
) {
    assert_not_null(opt);
    assert_not_null(src);

    if (*opt != NULL) {
        FREE(*opt);
    }

    *opt = strdup(src);
}

static void
options_set_long(
    long *opt,
    const char *src
) {
    assert_not_null(opt);
    assert_not_null(src);

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

void
options_set_defaults(
    options_t *options
) {
    assert_this(options);

    options->verbose               = OPTIONS_DEFAULT_VERBOSE;
    options->wait_events           = OPTIONS_DEFAULT_WAIT_EVENTS;
    options->animate_bg            = OPTIONS_DEFAULT_ANIMATE_BG;
    options->animate_win           = OPTIONS_DEFAULT_ANIMATE_WIN;
    options->max_fps               = OPTIONS_DEFAULT_MAX_FPS;
    options->initial_window_width  = OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH;
    options->initial_window_height = OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT;

    options->load_state_animate_bg  = true;
    options->load_state_animate_win = true;

    options->safe_mode = false;

    if (options->nvdata_dir) {
        options->nvdata_dir = NULL;
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
        case 'C':
            options->safe_mode = true;
            break;

        case 'c':
            options_set_string(&options->nvdata_dir, optarg);
            break;

        case 'F':
            options_set_long(&options->max_fps, optarg);
            break;

        case 'W':
            options_set_long(&options->initial_window_width, optarg);
            break;

        case 'H':
            options_set_long(&options->initial_window_height, optarg);
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

        default:
            fprintf(stderr, "ERROR: getopt returned character code 0%o", c);
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
