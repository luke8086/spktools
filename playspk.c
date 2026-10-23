/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: playspk.c - Standalone DOS player for SPK songs
 *
 * Usage: playspk SONG.SPK
 */

#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Silence IDE warnings */
#ifdef __clang__
#define __far
#define __interrupt
unsigned inp(unsigned port);
unsigned outp(unsigned port, unsigned value);
void _enable(void);
void _disable(void);
unsigned _bios_keybrd(unsigned service);
#define _KEYBRD_READ 0
#define _KEYBRD_READY 1
void (__interrupt __far *_dos_getvect(unsigned))();
void _dos_setvect(unsigned, void (__interrupt __far *)());
#define SIGBREAK SIGINT
#else
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#endif

/* Watcom's stack overflow checks are incompatible with interrupt handlers */
#pragma off (check_stack)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define PIT_FREQUENCY 1193180UL
#define TICK_FREQUENCY 100
#define REST_PITCH 59659U

enum {
    PIT_INTNO = 0x08,
    PIT_CR0 = 0x40, /* PIT counter 0 data port */
    PIT_CR2 = 0x42, /* PIT counter 2 data port */
    PIT_CWR = 0x43, /* PIT control word register */
    PPI_PB = 0x61,  /* Port B of the 8255A-5 PPI */
    PIC_CMD = 0x20, /* Cmd port of the master 8259A PIC */
};

enum {
    SPEAKER_STATE_STOPPED = 0,
    SPEAKER_STATE_PLAYING = 1,
};

enum {
    MAX_NOTES = 4096,
    MAX_TITLE = 64,
};

#define TITLE_FORMAT "%63[^\n]"

typedef void (__interrupt __far *isr_handler_fn)();

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

typedef struct {
    uint8_t state;

    const note_st *note;
    uint16_t note_ticks_left;
    int32_t song_elapsed_ticks;
} speaker_st;

typedef struct {
    isr_handler_fn saved_isr;
    int initialized;
} timer_st;

static timer_st timer;
static volatile speaker_st speaker;
static song_st song;

void cpu_hlt(void);
#pragma aux cpu_hlt = "hlt";

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
ignore_signal(int sig)
{
    (void)sig;
}

static void
speaker_set_freq(uint16_t hz)
{
    unsigned val;
    uint32_t divisor;

    /* If hz is 0, turn off the speaker */
    if (hz == 0) {
        val = inp(PPI_PB);
        outp(PPI_PB, val & ~0x03);
        return;
    }

    divisor = PIT_FREQUENCY / hz;
    divisor = MIN(divisor, 0xffff);

    /* Configure counter 2 of PIT to mode 3 (square wave) */
    outp(PIT_CWR, 0xB6);

    /* Set counter 2 to the desired frequency */
    outp(PIT_CR2, (unsigned)divisor & 0xFF);
    outp(PIT_CR2, ((unsigned)divisor >> 8) & 0xFF);

    /* Enable speaker by setting bits 0 (speaker enable) and 1 (gate) on port 0x61 */
    val = inp(PPI_PB);
    outp(PPI_PB, val | 0x03);
}

/* Must be called with interrupts disabled or in interrupt context */
static void
speaker_start_note(void)
{
    const note_st *note = speaker.note;

    if (note == NULL || note->ticks == 0) {
        /* Keep elapsed time for inspection */
        speaker.state = SPEAKER_STATE_STOPPED;
        speaker.note = NULL;
        speaker_set_freq(0);
        return;
    }

    speaker.note_ticks_left = note->ticks;

    speaker_set_freq(note->pitch ? note->pitch : REST_PITCH);
}

static void
speaker_get_state(speaker_st *out)
{
    _disable();

    memcpy(out, (const void *)&speaker, sizeof(*out));

    _enable();
}

static void
speaker_play_song(const note_st *notes)
{
    _disable();

    speaker.state = SPEAKER_STATE_PLAYING;
    speaker.note = notes;
    speaker.song_elapsed_ticks = 0;

    speaker_start_note();

    _enable();
}

static void
speaker_stop(void)
{
    _disable();

    speaker.state = SPEAKER_STATE_STOPPED;
    speaker.note = NULL;
    speaker.note_ticks_left = 0;
    speaker.song_elapsed_ticks = 0;

    speaker_set_freq(0);

    _enable();
}

/* Must be called in interrupt context */
static void
speaker_on_tick(void)
{
    if (speaker.state != SPEAKER_STATE_PLAYING) {
        return;
    }

    ++speaker.song_elapsed_ticks;
    --speaker.note_ticks_left;

    if (speaker.note_ticks_left > 0) {
        return;
    }

    ++speaker.note;
    speaker_start_note();
}

static void __interrupt __far
timer_handle_intr(void)
{
    /* Note, default handler is not called, so the DOS clock hangs */

    speaker_on_tick();

    outp(PIC_CMD, 0x20);
}

static void
timer_set_counter_0(uint16_t div)
{
    /* Set Counter 0, write both LSB and MSB, use mode 3 (square wave), binary counter */
    outp(PIT_CWR, 0x36);

    /* Write LSB and MSB for counter 0 */
    outp(PIT_CR0, div & 0xFF);
    outp(PIT_CR0, (div >> 8) & 0xFF);
}

static void
timer_init(void)
{
    timer.saved_isr = _dos_getvect(PIT_INTNO);

    _disable();

    timer_set_counter_0((unsigned)(PIT_FREQUENCY / TICK_FREQUENCY));
    _dos_setvect(PIT_INTNO, timer_handle_intr);
    timer.initialized = 1;

    _enable();
}

static void
timer_deinit(void)
{
    if (!timer.initialized) {
        return;
    }

    _disable();

    timer_set_counter_0(0);
    _dos_setvect(PIT_INTNO, timer.saved_isr);
    timer.initialized = 0;

    _enable();
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

static void
deinit(void)
{
    speaker_stop();
    timer_deinit();
}

int
main(int argc, char **argv)
{
    speaker_st st;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SONG.SPK\n", argv[0]);
        exit(1);
    }

    atexit(deinit);

    /* Prevent Ctrl-C and Ctrl-Break from stopping the app without deinitialization */
    signal(SIGINT, ignore_signal);
    signal(SIGBREAK, ignore_signal);

    load_song(argv[1]);

    printf("Loaded '%s' (%u notes)\n", song.title, song.note_count);

    timer_init();
    speaker_play_song(song.notes);

    for (;;) {
        speaker_get_state(&st);

        if (st.state != SPEAKER_STATE_PLAYING) {
            break;
        }

        if (_bios_keybrd(_KEYBRD_READY)) {
            (void)_bios_keybrd(_KEYBRD_READ);
            break;
        }

        cpu_hlt();
    }

    deinit();

    return 0;
}
