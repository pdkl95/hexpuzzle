#!/bin/bash

: ${RAYLIBDIR:=${HOME}/build/ray/raylib-5.0-web/src}

do_configure() {
    emconfigure ./configure \
                RAYLIB_CFLAGS="-I${RAYLIBDIR}" \
                RAYLIB_LIBS="-L${RAYLIBDIR} -lraylib" \
                --enable-web \
                "$@"
}

do_configure "$@" && make
