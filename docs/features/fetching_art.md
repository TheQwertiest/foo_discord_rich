---
title: Fetching artwork
parent: Features
nav_order: 1
---

# Fetching artwork
{: .no_toc }

This page contains implementation details on how automatic album art fething works.

**TLDR**: If track contains MBID, component will try using it. If not, it will try to find MBID by album and artist.

## Algorithm

When artwork is requested, component does the following:
- Check if track contains either `MUSICBRAINZ_ALBUMID` or `MUSICBRAINZ ALBUM ID` tag (malformed tags are skipped) and use it as MBID for next steps.
- If track does not contain MBID tag, then try to find MBID by album and artist via `MusicBrainz` API:
    - Find release group: `https://www.musicbrainz.org/ws/2/release-group?artist:"{ARTIST}"+releasegroup:"{ALBUM}"`.
    - Select the first `release-groups` from response and try to find entry in `releases` that contains artwork:
      - Get release info: `https://www.musicbrainz.org/ws/2/release/{MBID}`.
      - Check if `cover-art-archive[artwork]` is `true`.
- Fetch album art url from `Cover Art Archive`:
    - `http://coverartarchive.org/release/{MBID}/front-1200`
