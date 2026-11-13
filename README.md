# MPRIS Media Controller

A standalone Qt Widgets application for controlling MPRIS-compatible media players.

## Features

- Supports multiple MPRIS-compatible players;
- Player selection when multiple players are available;
- Title, artist, playback position and duration;
- Previous, play/pause and next controls;
- Clickable and draggable playback slider;
- Automatic playback information updates;
- Popup hiding when the window loses focus;
- System tray integration;
- `--toggle` support for custom global shortcuts;
- Native Qt light and dark theme support.

## Requirements

- Qt 6
- An MPRIS-compatible media player
- CMake
- C++17 compiler

## Build

Clone the repository:

```bash
git clone https://github.com/TomRiddle901/krunner-mpris-media-control.git
cd krunner-mpris-media-control
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

## Installation

Install the application locally:

```bash
cmake --install build --prefix "$HOME/.local"
```

The executable is installed as:

```text
~/.local/bin/mpris_media_controller
```

## Usage

Start the controller directly from the build directory:

```bash
./build/mpris_media_controller
```

After local installation, start the controller with:

```bash
mpris_media_controller
```

Toggle the popup:

```bash
mpris_media_controller --toggle
```

The `--toggle` command can be assigned to a KDE custom shortcut, for example:

```text
Alt + M
```

When the controller window is active, the following shortcuts are available:

| Shortcut | Function |
| --- | --- |
| `Ctrl + Q` | Quit the application |
| `Space` | Play / pause |
| `Ctrl + Right` | Next track |
| `Ctrl + Left` | Previous track |
| `Esc` | Hide the controller |

These shortcuts only work while the controller window is active, not when it is
hidden in the system tray.

The controller remains active in the system tray while its popup is hidden.

## MPRIS

The application communicates with players through the standard MPRIS D-Bus interfaces. No player-specific integration is required.

## License

This project is licensed under the GNU General Public License v3.0 or later.
See [LICENSE](LICENSE) for details.
