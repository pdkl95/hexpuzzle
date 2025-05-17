/****************************************************************************
 *                                                                          *
 * compile_to_c_src.c                                                       *
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

#include "raylib.h"
#include "common.h"

int main(int argc, char **argv)
{
    //SetTraceLogLevel(LOG_INFO);

    if (argc != 3) {
        printf("usage: %s <input.png|ttf|otf> <output.c>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *infile = argv[1];
    char *outfile = argv[2];

    printf("Compiling \"%s\" into \"%s\"\n",
           infile, outfile);

    int rv = EXIT_FAILURE;

    if (IsFileExtension(infile, ".pdf")) {
        Image img = LoadImage(infile);
        if (ExportImageAsCode(img, outfile)) {
            rv = EXIT_SUCCESS;
        }

    } else if (IsFileExtension(infile, ".ttf") ||
               IsFileExtension(infile, ".otf")) {
        SetConfigFlags(0);
        InitWindow(800,800,"compile_to_c_src");
        Font fnt = LoadFont(infile);
        if (ExportFontAsCode(fnt, outfile)) {
            rv = EXIT_SUCCESS;
        } else {
            printf("failed!\n");
        }

    } else {
        printf("ERROR: Unsupported file type: %s\n",
               GetFileExtension(infile));
    }

    return rv;
}
