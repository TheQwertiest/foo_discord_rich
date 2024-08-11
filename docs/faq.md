---
title: Frequently Asked Questions
nav_order: 4
---

# Frequently Asked Questions
{: .no_toc }
---
#### Component is not working, Rich Presence is not displayed. How do I fix that?

- Make sure that you've followed all the steps for configuring Discord on [Installation page]({{ site.baseurl }}{% link docs/installation.md %}).
- If it didn't help, try resetting component settings:
    - Go to `Preferences` > `Tools` > `Discord Rich Presence Integration`.
    - Click on `Reset page` and then `Apply`.
    - Restart foobar2000.
- If it didn't help, try launching both Discord and foobar2000 as Administrator (right-click the app, and select the `Run as administrator` option).
- If it didn't help, try the following steps to check if the problem is caused by conflict with some other component:
    - Download the latest (non-beta) foobar2000.
    - Install it in portable mode to some `%FOLDER%`.
    - Download and install the latest (non-beta) `foo_discord_rich`.
    - After rebooting foobar2000, ensure that `Preferences` > `Tools` > `Discord Rich Presence Integration` > `Enabled is checked`.
    - Follow all the steps for configuring Discord in guide above.
    - Play some track in foobar2000.
    - Check the status in Discord.
    - If everything works, then it means it is caused by other components: try reinstalling them one by one and to find which component causes issues.
- If it still didn't help, then you can try searching and/or posting in [Support thread](https://hydrogenaud.io/index.php/topic,116860.new.html). Perhaps other users have a fix for your problem.
