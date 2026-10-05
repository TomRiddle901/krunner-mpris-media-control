# KRunner MPRIS Media Control

A custom KRunner for controlling MPRIS-compatible media players directly from KDE Plasma

The runner provides a compact media-control panel inside KRunner, with the current player, track information and playback controls.

---

## Features

- Supports MPRIS-compatible media players;
- Works with players such as Elisa, VLC, Dragon Player and other MPRIS players;
- Open the controller by typing `player`, `media` or `mpris` in KRunner;
- Shows:
    - media player icon
    - track title
    - artists
    - playback progress
    - previous track
    - play/pause
    - next track
- Automatically selects the currently playing player;
- Falls back to paused or available players when nothing is playing;
- Uses D-Bus to communicate directly with MPRIS players;
- Custom KRunner/Milou layout for the media-control panel;
- Designed for KDE Plasma 6 and KF6.

---

## Requirements

- KDE Plasma 6
- Qt 6
- KDE Frameworks 6
- KRunner
- An MPRIS-compatible media player
- CMake
- C++17 compiler

---

## Build

Clone the repository:
```bash
git clone https://github.com/TomRiddle901/krunner-mpris-media-control.git
cd krunner-mpris-media-control
```

Configure and build
```bash
cmake -B build
cmake --build build -j$(nproc)
```

## Installation

Install the KRunner plugin into user's local directory
```bash
cmake --install build --prefix "$HOME/.local"
```

The plugin will be installed under:
```bash
~/.local/lib64/plugins/kf6/krunner/krunner_mpris.so
```

## Plasma environment

The runner also uses a custom Milou QML implementation for its interface.

Create the Plasma environment script:
```bash
mkdir -p ~/.config/plasma-workspace/env
nano ~/.config/plasma-workspace/env/krunner-mpris.sh
```

Add:
```bash
#!/bin/sh

export QT_PLUGIN_PATH="$HOME/.local/lib64/plugins:/usr/lib64/qt6/plugins"
export QML2_IMPORT_PATH="$HOME/path/to/krunner-mpris-media-control/qml:/usr/lib64/qt6/qml"
```
Replace `path/to/krunner-mpris-media-control` with the actual path where you cloned the repository.

Make it executable
```bash
chmod +x ~/.config/plasma-workspace/env/krunner-mpris.sh
```

Log out and log back into Plasma.

---

## Usage

Open KRunner with:<br>
`Alt + Space`<br>

Then type:<br>
`player`

The MPRIS controller will appear directly in KRunner.

The runner also supports the following queries:
```
player
media
mpris
```
and searches using:
```
player <query>
media <query>
mpris <query>
```

---

## Architecture

The project consists of two main parts:

### C++ KRunner runner

`mprisrunner.cpp` communicates with MPRIS players through D-Bus and provides KRunner with:
* player information
* track metadata
* playback status
* position
* duration
* playback actions

### Milou QML interface

The `qml/org/kde/milou/` directory contains the customized KRunner result delegate used to display the MPRIS controller as a vertical media panel.

### MPRIS

The runner communicates with players through the standard MPRIS D-Bus interfaces.

No player-specific integration is required.

---

## License

This project is licensed under the GNU General Public License v3.0 or later.
See [LICENSE](LICENSE) for details.