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
static char short_options[] = "F:H:t:wvVW:hj";

static struct option long_options[] = {
    {         "fps", required_argument, 0, 'F' },
    {      "height", required_argument, 0, 'H' },
    {       "width", required_argument, 0, 'W' },
    { "wait-events",       no_argument, 0, 'w' },
    {     "verbose",       no_argument, 0, 'v' },
    {     "version",       no_argument, 0, 'V' },
    {        "help",       no_argument, 0, 'h' },
    {             0,                 0, 0,  0  }
};

static char usage_args[] = "";

static char help_text[] =
    "\n"
    "Mandelbrot Iterations\n"
    "---------------------\n"
    "\n"
    "A hex-tile based puzzle game.\n"
    "\n"
    "OPTIONS\n"
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
    return options;
}

static void
free_options(
    options_t *options
) {
    if (options) {
        free(options);
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
    options->max_fps               = OPTIONS_DEFAULT_MAX_FPS;
    options->initial_window_width  = OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH;
    options->initial_window_height = OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT;
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
        case 'F':
            options_set_long(&options->max_fps, optarg);
            break;

        case 'W':
            options_set_long(&options->initial_window_width, optarg);
            break;

        case 'H':
            options_set_long(&options->initial_window_height, optarg);
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

    return true;
}
