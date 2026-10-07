# spktools

Tools and sample songs for the PC speaker, used by
[GentleOS/16](https://github.com/luke8086/gentleos)
and [GentleOS/32](https://github.com/luke8086/gentleos32).

## SPK format

An SPK file contains a plain text list of events and metadata, for example:

```
title: Mozart WA - Turkish March
659, 200
622, 200
659, 200
0, 200
```

A metadata line contains a `key: value` pair. Currently only `title` is recognized.

An event line contains the frequency (Hz) and the duration (ms) of a note. A frequency of 0 means rest.

## Conversion from MusicXML

SPK files can be generated from the uncompressed MuxicXML format, using `mkspk.py`.
The author only tested files exported from MuseScore, but other tools may also work.

The score must only contain a single staff, with a single voice and no chords.
The only recognized elements are: plain notes, rests, grace notes, ties, staccato dots and tempo marks.

The `mkspk` script requires Python 3.10+, preferably managed by [uv](https://docs.astral.sh/uv/). If not using uv, you'll optionally need `sounddevice` Python library for playback.


To convert a score, run:

```
uv run mkspk.py -i mysong.musicxml -o mysong.spk
```

To preview a score, optionally starting from a given measure, run:

```
uv run mkspk.py -i mysong.musicxml -p
uv run mkspk.py -i mysong.musicxml -p -s 12
```
