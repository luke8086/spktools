/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: playspk.c - Standalone DOS player for SPK songs
 *
 * Usage: playspk SONG.SPK
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void
die(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    exit(1);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        die("usage: playspk SONG.SPK");
    }

    return 0;
}
