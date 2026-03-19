<img width="1162" alt="transportr-ss" src="https://user-images.githubusercontent.com/62677644/146621596-6e5988bf-ac85-4e91-a807-31f0c1f10d37.png">

# transportr

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg)]()

Retro-themed command-line audio player with transport controls for WAV files. Built with C++, PortAudio, libsndfile, and ncurses.

```
        .------------------------.
        |\////////         G.M.  |
        | \/  __  ______  __     |
        |    /  \|\.....|/  \    |
        |    \__/|/_____|\_\/    |
        |     = TRANSPORTR =     |
        |    ________________    |
        |___/_._o________o_._\___|
```

## Features

- Load and switch between up to 8 WAV tracks at runtime
- Transport controls: play/pause, stop, rewind, fast-forward, 1-second jump, and loop
- Real-time position display (`MM:SS / MM:SS`)
- Thread-safe track switching during playback via a lock-free atomic

## Architecture

transportr uses two concurrent threads:

- **Main thread** — runs the ncurses event loop, reads keyboard input, and updates shared state.
- **PortAudio callback thread** — a real-time audio thread invoked by PortAudio approximately every 23ms (1024 frames at 44.1 kHz). It reads shared state and writes audio samples to the output buffer.

The critical design constraint is that the callback thread must be **non-blocking** — no mutexes, no memory allocation, no I/O. Track selection is therefore communicated via a single `atomic_int` in the shared `Buf` struct, which guarantees a consistent read across threads without a lock.

All audio data is loaded into heap-allocated buffers (`float*`) before playback begins, so the callback only reads pre-loaded memory. Transport state flags (`playback`, `rew`, `fwd`, etc.) are plain booleans written by the main thread and read by the callback — safe in practice because each flag is a single word and the main thread only writes one at a time, though formally `atomic<bool>` would be more correct.

## Dependencies

Install via Homebrew on macOS:

```bash
brew install portaudio libsndfile ncurses
```

Or on Debian/Ubuntu:

```bash
sudo apt-get install libportaudio2 libportaudio-dev libsndfile1 libsndfile1-dev libncurses-dev
```

## Build and Run

Make the build script executable (first time only):

```bash
chmod +x build.sh
```

Compile:

```bash
./build.sh
```

Run:

```bash
./transportr
```

## Adding Your Own Audio

1. Place WAV files in the `./audio/` directory.
2. Add each file path (one per line) to `audioPaths.txt`.

```
./audio/track1.wav
./audio/track2.wav
```

**Requirements:** all files must share the same sample rate and have at most 2 channels (stereo). A maximum of 8 tracks can be loaded at once.

Two sample tracks are included in `./audio/` to get started.

## Controls

| Key     | Action                  |
|---------|-------------------------|
| `SPACE` | Play / Pause            |
| `S`     | Stop (resets to start)  |
| `A`     | Rewind (2× reverse)     |
| `D`     | Fast-forward (2×)       |
| `,`     | Jump back 1 second      |
| `.`     | Jump forward 1 second   |
| `L`     | Toggle loop             |
| `0`–`7` | Select track            |
| `Q`     | Quit                    |
