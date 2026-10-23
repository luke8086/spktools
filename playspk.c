/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: playspk.c - Standalone DOS player for SPK songs
 *
 * Usage: playspk SONG.SPK
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

enum {
    TICK_FREQUENCY = 100,
    MAX_NOTES = 4096,
    MAX_TITLE = 64,
};

#define TITLE_FORMAT "%63[^\n]"

typedef struct {
    uint16_t pitch;
    uint16_t ticks;
} note_st;

typedef struct {
    note_st notes[MAX_NOTES];
    char title[MAX_TITLE];
    unsigned note_count;
    int32_t total_ticks;
} song_st;

static song_st song;

static void
die(const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "playspk: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    exit(1);
}

static void
load_song(const char *path)
{
    FILE *file;
    unsigned pitch, msecs;
    int32_t ticks;
    int32_t total_msecs = 0;
    int matched;

    file = fopen(path, "r");

    if (file == NULL) {
        die("cannot read %s", path);
    }

    if (fscanf(file, TITLE_FORMAT, song.title) != 1) {
        die("no title in %s", path);
    }

    while ((matched = fscanf(file, "%u %u", &pitch, &msecs)) == 2) {
        if (song.note_count >= MAX_NOTES - 1) {
            die("%s: more than %u notes", path, MAX_NOTES - 1);
        }

        total_msecs += MAX(msecs, 1);
        ticks = ((total_msecs * TICK_FREQUENCY + 500) / 1000) - song.total_ticks;
        ticks = MAX(MIN(ticks, 0xffff), 1);

        song.notes[song.note_count].pitch = (uint16_t)pitch;
        song.notes[song.note_count].ticks = (uint16_t)ticks;
        song.total_ticks += ticks;
        song.note_count += 1;
    }

    if (matched != EOF) {
        die("%s:%u: invalid syntax", path, song.note_count + 2);
    }

    fclose(file);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s SONG.SPK\n", argv[0]);
        exit(1);
    }

    load_song(argv[1]);

    printf("Loaded '%s' (%u notes)\n", song.title, song.note_count);

    return 0;
}
