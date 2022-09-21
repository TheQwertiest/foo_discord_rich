---
title: Frequesntly Asked Questions
nav_order: 3
---

# Installation
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

#### How do I display album art instead of fb2k logo?

Discord limitation: see [#6](https://github.com/TheQwertiest/foo_discord_rich/issues/6) for more info.
<br><br>

#### Is it possible to change "Playing" to "Listening to"?

Discord limitation: see [#2](https://github.com/TheQwertiest/foo_discord_rich/issues/2) for more info.

#### Why is the status not showing up? I can't see the song being played!

There can be a couple of reasons why, for example:
- your Discord account is set to "Invisible", please set it to anything else in order for this to work.
- "Display current activity as a status message" is disabled. This can be enabled in Discord's settings, under the Activity Privacy section.
- Foobar2000 has been added manually through the "Registered Games" section of Discord's settings, this must be removed in order for the module to work. The module sends data directly to Discord, and does not need a manual add.
- If none of these still work, please see [#33](https://github.com/TheQwertiest/foo_discord_rich/issues/33), or [open an issue](https://github.com/TheQwertiest/foo_discord_rich/issues/new).
