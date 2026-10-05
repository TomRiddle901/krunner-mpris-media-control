# Changelog

All notable changes to this project are documented in this file.

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