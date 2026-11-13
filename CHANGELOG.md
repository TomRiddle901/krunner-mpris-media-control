# Changelog

All notable changes to this project are documented in this file.

---

## [2.1.1] - 2026-11-13

### Added

- Window-local keyboard shortcuts:
  - `Ctrl+Q` to quit the application.
  - `Space` to toggle play/pause.
  - `Ctrl+Right` to play the next track.
  - `Ctrl+Left` to play the previous track.
  - `Esc` to hide the controller.

### Changed

- Playback shortcuts do not interfere with the player selector, playback slider
  or playback buttons when one of them has focus.
- Documented that these shortcuts work only while the controller window is
  active; `Alt+M` remains the KDE custom shortcut for showing it.

## [2.1.0] - 2026-11-13

### Added

- Improved MPRIS metadata handling through the standard `GetAll` D-Bus call.
- Fallback metadata extraction from local media files when player metadata is incomplete.

### Changed

- Prefer Elisa when multiple MPRIS players are available and no player is selected.
- Refresh player data immediately when the controller window is activated.
- Preserve the tray-resident background behavior when the controller loses focus.
- Improved artist and album label readability with theme-aware text color and wrapping.

### Fixed

- Restored player selector signals after refreshing the available players.
- Preserved the selected player across periodic player list refreshes.

## [2.0.0] - 2026-11-06

### Changed

- Replaced the KRunner plugin with a standalone Qt Widgets application.
- Removed the KRunner, Milou and Plasma environment integration.
- Added a tray-resident controller with `--toggle` support.

### Added

- Player selection for multiple MPRIS-compatible media players.
- Previous, play/pause and next playback controls.
- Clickable and draggable playback position slider.
- Automatic playback information updates.

## [1.0.1] - 2026-11-06

### Added
- Standalone Qt Widgets media controller.
- Player selection for multiple MPRIS-compatible media players.
- Previous, play/pause and next playback controls.
- Clickable and draggable playback position slider.
- Automatic playback information updates.
- System tray integration.
- Popup hiding when focus is lost.
- `--toggle` command for showing or hiding the controller.
- Support for building and installing the standalone media controller with CMake.

## [1.0.0] - 2026-10-05

### Added

- Initial public release of KRunner MPRIS Media Control.
- Support for generic MPRIS-compatible media players through D-Bus.
- Support for the `player`, `media` and `mpris` KRunner queries.
- Support for searches using `player <query>`, `media <query>` and `mpris <query>`.
- Automatic selection of the currently playing MPRIS player.
- Fallback to paused or available players when no player is currently playing.
- Display of the media player icon.
- Display of the current track title and artist.
- Playback progress bar.
- Previous track control.
- Play/pause control.
- Next track control.
- Custom Milou QML layout for the MPRIS media-control panel.
- Support for local user installation through CMake.
- Plasma environment configuration for loading the custom KRunner and Milou components.
- GPL-3.0-or-later licensing.

### Technical

- C++17 implementation.
- Qt 6.
- KDE Frameworks 6.
- KRunner.
- MPRIS over D-Bus.
- CMake build system.
- Custom QML/Milou result delegate.