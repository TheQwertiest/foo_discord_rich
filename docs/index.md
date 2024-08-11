---
layout: page
title: Home
nav_order: 1
permalink: /
---

# Discord Rich Presence Integration
{: .no_toc }
[![version][version_badge]][changelog] [![Build status][appveyor_badge]](https://ci.appveyor.com/project/TheQwertiest/foo-discord-rich/branch/master) [![CodeFactor][codefactor_badge]](https://www.codefactor.io/repository/github/theqwertiest/foo_discord_rich/overview/master) [![Codacy Badge][codacy_badge]](https://app.codacy.com/gh/TheQwertiest/foo_discord_rich/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade) 

---
![foo_discord_rich](assets/img/foo_discord_rich.png)

This is a component for the [foobar2000](https://www.foobar2000.org) audio player, which displays currently played track data via Discord Rich Presence.

[Features](features.md):
- Text field configuration via [title formatting queries](http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Title_Formatting_Reference).
- Automatic album art fetching from [Cover Art Archive](https://coverartarchive.org/) (via [MusicBrainz](https://musicbrainz.org/)).
- Foobar2000 art uploading (via external tools).
- [foo_acfu](https://acfu.3dyd.com) integration.

## Getting started!

- Use [Installation Guide](installation.md).
- Check out [Frequently Asked Questions](faq.md).

## Links

[Support thread](https://hydrogenaud.io/index.php/topic,116860.new.html)  
[Changelog][changelog]  
Dev builds: [Win32][nightly_win32], [x64][nightly_x64]

## Credits

- [s0hv](https://github.com/s0hv): idea, original implementation and documentation for artwork uploader.
- [Respective authors][3rdparty_license] of the code being used in this project.

[changelog]: changelog.md
[3rdparty_license]: third_party_notices.md
[todo]: https://github.com/TheQwertiest/foo_discord_rich/projects/1
[version_badge]: https://img.shields.io/github/release/theqwertiest/foo_discord_rich.svg
[appveyor_badge]: https://ci.appveyor.com/api/projects/status/t5bhoxmfgavhq81m/branch/master?svg=true
[codacy_badge]: https://api.codacy.com/project/badge/Grade/319298ca5bd64a739d1e70e3e27d59ab
[codefactor_badge]: https://www.codefactor.io/repository/github/theqwertiest/foo_discord_rich/badge/master
[nightly_win32]: https://ci.appveyor.com/api/projects/theqwertiest/foo-discord-rich/artifacts/_result%2FWin32_Release%2Ffoo_discord_rich-Win32.fb2k-component?branch=master&pr=false&job=Configuration%3A+Release%3B+Platform%3A+Win32
[nightly_x64]: https://ci.appveyor.com/api/projects/theqwertiest/foo-discord-rich/artifacts/_result%2Fx64_Release%2Ffoo_discord_rich-x64.fb2k-component?branch=master&pr=false&job=Configuration%3A+Release%3B+Platform%3A+x64
