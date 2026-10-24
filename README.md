# spktools

Tools and sample songs for the PC speaker, used by
[GentleOS/16](https://github.com/luke8086/gentleos)
and [GentleOS/32](https://github.com/luke8086/gentleos32).

## SPK format

An SPK file contains a plain text title and a list of notes, for example:

```
Mozart WA - Turkish March
659 200
622 200
659 200
0 200
```

The first line is always the title of the song.

The note lines contain the frequency (Hz) and the duration (ms)
of a note, separated by a single space.

A frequency of 0 means rest.

## Conversion from MusicXML

SPK files can be generated from the uncompressed MuxicXML format, using `mkspk.py`.
The author only tested files exported from MuseScore, but other tools may also work.

The score must only contain a single staff, with a single voice and no chords.
The only recognized elements are: plain notes, rests, grace notes, ties, staccato dots and tempo marks.

The `mkspk` script requires Python 3.10+


To convert a score, run:

```
./mkspk.py -i mysong.musicxml -o mysong.spk
```

To rebuild all scores in this repo, run:

```
make
```

### Playing SPK files

You can play SPK files on a DOS machine or under DOSBox, just by running:

```
playspk song.spk
```
