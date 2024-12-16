#!/bin/bash

do_configure() {
    emconfigure ./configure \
                --enable-web \
                "$@"
}

do_configure "$@" && make
