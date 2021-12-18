<img width="1162" alt="transportr-ss" src="https://user-images.githubusercontent.com/62677644/146621596-6e5988bf-ac85-4e91-a807-31f0c1f10d37.png">

# transportr

Retro-themed command-line application providing basic transport controls for WAV files.

## Run and Compile

* Ensure `build.sh` is an executable:
```bash
chmod +x build.sh
```

* Compile all code:
```bash
./build.sh
```

* Run transporter:
```bash
./transporter
```

## Getting Started
After running the program, the tracks in `./audio/` will be loaded into memory before reaching the main ncurses display.

Track 0 is the default initial selection, however the user can switch between tracks by pressing their respective key. Hitting spacebar will begin playback. The user may then follow the on-screen instructions regarding the other transport controls. It is worth noting that, though `<` and `>` are displayed for jump-back and jump-forward, respectively, these controls are technically `,` and `.`.

## Audio
Two audio tracks are already in `./audio/` folder for ease of use. These may be replaced by the user, but it is important that all filepaths are included in the `audioPaths.txt` file. All audio files must have the same sample rate and be in WAV format.
