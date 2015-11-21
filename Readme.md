# AudioPlayer
A simple audio playback library.
Only for Windows.

## Features
- Supports data larger than 4GB
- Concurrency playback
	- Supports at least 32767 playbacks on the implementation
- Loading from file
- Loading from memory
- Seeking
- Volume controlling
- Gapless audio looping
	- Track looping
	- Section looping (AB looping)
- API designed for [HSP3](http://hsp.tv/)

## Supported Input Formats
- FLAC / Ogg FLAC
	- 8-bit, 16-bit, 24-bit, 32-bit integer
- Ogg Opus
- Ogg Vorbis
- WAV (Linear PCM)
	- 8-bit, 16-bit integer
	- 24-bit, 32-bit integer (may be non-compliant)
	- 32-bit float

## Supported Output Formats
(Linear PCM only.)
- 8-bit, 16-bit integer
- 24-bit, 32-bit integer (may be non-compliant)
- 32-bit float