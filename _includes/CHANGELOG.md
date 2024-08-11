# Changelog

#### Table of Contents
- [Unreleased](#unreleased)
- [2.0.2](#202---2024-08-11)
- [2.0.1](#201---2024-08-11)
- [2.0.0](#200---2024-08-11)
- [1.2.0](#120---2019-09-11)
- [1.1.0](#110---2018-11-07)
- [1.0.0](#100---2018-11-06)

___

## [Unreleased][]

## [2.0.2][] - 2024-08-11

### Fixed
- Fixed wrong art request data being used when processing fetched url (#65).

## [2.0.1][] - 2024-08-11

### Added
- Added debug logging (`Preferences`>`Advanced`>`Tools`>`Discord Rich Presence Integration`).

### Changed
- User MBID is now skipped if it's malformed.

### Fixed
- Fixed wrong album/artist values being used in MusicBrainz fetcher (#63).
- Fixed incorrect path being passed to uploader when using embedded art (#64).

## [2.0.0][] - 2024-08-11

### Added
- Added x64 support (#39).
- Added dark-mode support (#61).
- Added option to fetch and display album art from MusicBrainz (#6).
- Added option to upload and display art from foobar2000 (requires external tools, not included in component) (#62).
- Added text refresh when dynamic track info changes (no more than once in 30 seconds, due to Discord API limitations) (#50).

### Changed
- !!! Now requires foobar2000 v2.0+ !!!
- Changed Rich Presence activity type from `Playing a game` to `Listening to` (#2).
  This change has also other effects, due to the way it's implemented in Discord API:
	- Additional middle text field was added.
	- Playback time is no longer displayed.
	- Big image hover text is the same as middle text.

### Fixed
- Fixed inconsistent behaviour when pausing/stoping playback (#21).
- Fixed tabs not receiving focus on `tab` press in Preferences (#23).
- Fixed typo in component name in Preferences (#41).
- Fixed various corner cases when multibyte characters were used in text queries (#57)

## [1.2.0][] - 2019-09-11
### Added
- Added playback status images.
- Added new options to `main` Preferences tab:
  - Playback status image: light, dark, disabled.
  - Disable Rich Presence when playback is paused.
  - Swap `paused` and `playing` images.
- Added `advanced` Preferences tab with options to customize component:
  - Discord application key.
  - Resource IDs for corresponding images in the component.
- Added a link to the title formatting help in `main` Preferences tab.

### Changed
- Improved the frequency of presence updates.

### Fixed
- Fixed title formatting not updating when pausing and resuming playback.
- Fixed one-character text not displaying.

## [1.1.0][] - 2018-11-07
### Added
- Added Preferences page with the following settings:
  - Text fields configuration via title formatting queries.
  - Track duration: elapsed, remaining, disabled.
  - Foobar2000 image: light, dark, disabled.
- Added main menu command to toggle component.

### Fixed
- Fixed some bugs with persistent track info.

## [1.0.0][] - 2018-11-06
Initial release.

[unreleased]: https://github.com/TheQwertiest/foo_discord_rich/compare/v2.0.2...HEAD
[2.0.2]: https://github.com/TheQwertiest/foo_discord_rich/compare/v2.0.1...v2.0.2
[2.0.1]: https://github.com/TheQwertiest/foo_discord_rich/compare/v2.0.0...v2.0.1
[2.0.0]: https://github.com/TheQwertiest/foo_discord_rich/compare/v1.2.0...v2.0.0
[1.2.0]: https://github.com/TheQwertiest/foo_discord_rich/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/TheQwertiest/foo_discord_rich/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/TheQwertiest/foo_discord_rich/commits/v1.0.0
