---
title: Uploading artwork
parent: For advanced users
---

# Building component
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

## Introduction

This section will help you set up automatic artwork uploading to your hosting service of choice and use the resulting url to display the image in Discord Rich Presence.

Note: third-party tools are required for this to work.

## Setup

First, open `Preferences` > `Discord Rich Presence Integration` > `Advanced` tab.

Check the box in `Art uploader` > `Upload and display art`.

Then, in the `Upload command` you need to write the full path to the program (with all the necessary parameters) that will handle the uploading.
It is recommended that you surround any paths with quotes, as paths containing spaces will most likely cause problems.

The exact commands that you need to input here change based on the script and are explained with more detail in the [Upload scripts](#upload-scripts) section.  
The last option is called `Art pin query`, which is better left untouched in most cases (see [Descriptions of options](#options-description) for more info).

## Uploaders

**Disclaimer**: uploading is implemented via third-party software, which is maintained and updated, if at all, separately from this plugin by their respective authors. Use them at your own discretion, we are not responsible for any damages or other problems the scripts might cause.

### Imgur ([imgur.com](https://imgur.com))

#### Standalone binary

This one is the easiest to install and use as it's just a single exe that you need download.

You can find the latest release here [https://github.com/s0hv/rust-imgur-upload/releases/latest](https://github.com/s0hv/rust-imgur-upload/releases/latest)

Then, to use it, just copy the path to the Artwork upload command like so `"C:\path\to\imgur-uploader.exe"`.  
After this the album art feature should start working.

#### Python 3.7 (or later) script

This one requires a bit of technical knowledge but it's easier to customize for other needs if you have basic coding skills. 

You need to have python 3.7 or later installed along with the requests library.

Also, you need to obtain an imgur API token from here [https://api.imgur.com/oauth2/addclient](https://api.imgur.com/oauth2/addclient) (more instructions can be found here [https://apidocs.imgur.com/#intro](https://apidocs.imgur.com/#intro)).  
Fill the form and for authorization type make sure to select "OAuth 2 authorization without a callback URL".  
After obtaining the API client id you can move on to the code.

The code can be found in [this gist](https://gist.github.com/s0hv/5c07cfb4b939ee619d0efcc047991ceb).

After saving the script on your machine, replace the part that says 
`Insert imgur api client id here` with the client id you obtained earlier while keeping the single quotes in place (`'`)

After you have installed python, saved the script on your machine and inserted the client id to the script, you need to set the Artwork upload command as follows.
`"C:\path\to\python" "C:\path\to\imgur_upload.py"`

After these steps you should start seeing the album art show up in your discord status.

### Catbox ([catbox.moe](https://catbox.moe))

#### Powershell 7 script

To use this script you must have Powershell 7 installed (might work with 6, but I have not tested this). 
One way of installing it is with [winget](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows?view=powershell-7.3#winget)
Also, for full UTF-8 support you must enable UTF-8 support from 
`Language` > `Administrative language settings` > `Change system locale` > `Make sure this option is checked "Beta: Use Unicode UTF-8 for worldwide language support".`

The code can be found from [this gist](https://gist.github.com/vt-idiot/8a7161a48dc6f7f7719423e938217267).

After downloading set the Artwork upload command to the following value (assumes powershell 7 has been installed in the default location)
`"C:\Program Files\PowerShell\7\pwsh.exe" "C:\path\to\catbox.ps1"`

#### Python 3.7 (or later) script

The python setup for this is the same as for [imgur script](#imgur-upload-script-for-python-37-or-later).
The code can be found from [this gist](https://gist.github.com/mechabubba/db1200c05fbbecf753b23c92ee8e9271).

After you have installed python and saved the script on your machine, you need to set the Artwork upload command as follows.
`"C:\path\to\python" "C:\path\to\catbox_upload.py"`

#### Standalone binary

The code for this uploader can be found [here](https://github.com/realoksi/foobar2000-catbox)
and the releases can be found [here](https://github.com/realoksi/foobar2000-catbox/releases).
The installation instructions are the same as for [Standalone imgur uploader](#standalone-binary)

### Amazon S3

#### Python 3.7 (or later) script

This script allows you to upload images to an Amazon S3 bucket, 
and it can be found [here](https://gist.github.com/okdargy/a55f40c7b339ee0a8b10a9827015453b).
The author recommends the [ShareX Amazon S3 guide](https://getsharex.com/docs/amazon-s3) 
if you don't know how to set up a publicly accessible S3 bucket.

## Options description

This section contains brief description for the different options in the `Preferences` > `Discord Rich Presence Integration` > `Advanced` > `Art upload` section of preferences page.

`Upload and display` checkbox determines whether album art is uploaded and used as the image or not.
If this option is enabled, it will override and disable the built-in art fetcher from MusicBrainz.

`Upload command` contains the command that will be run to upload the image.
If this field is left empty and the checkbox is checked, the plugin will use the album art for images
that have already been uploaded but the default image will be shown otherwise.

`Art pin query` determines a unique key based on properties of the track,
which will be used to determine if the track shares album artwork with another track.
By default, the value is `%album%|%artist%` which assumes that every track on the same album has the same artwork.
This can be changed if necessary. The value accepts normal foobar2000 query syntax.

## Troubleshooting

#### I set up everything but it still is not working

There is most likely a problem with your upload script. Open up the console from `View` -> `Console` and start playing a new track.
Lines prefixed with `Discord Rich Presence Integration` are logs from this plugin and they can be used to troubleshoot what is going wrong and where.

Note that upload results are cached (regardless if successful or not), so you might need to edit and/or clear the cache. See [Manipulating art cache](#manipulating-art-cache) section for more info.

#### I changed the artwork, but it was not updated on discord

This is because the plugin caches the url after it has uploaded the image.
Thus it won't be updated unless you manually edit and/or clear the old url from cache. See [Manipulating art cache](#manipulating-art-cache) section for more info.

## Manipulating art cache

Component stores all processed upload results in memory cache, which is loaded from disk upon foobar2000 start-up and saved to disk upon exit.

These operations may be triggered manually by using buttons in `Preferences` > `Discord Rich Presence Integration` > `Advanced` > `Art Cache`.

Let's say, for example, that you want to force reupload of art for the `Coal Chamber` artist.

- Press `Save to disk`.
- Press `Open containing folder...`, which will open and select the art cache file.
- Open said file in editor, find the track and remove the corresponding value. You need to remove the track as well and not just the corresponding url value, since empty url is considered to be a valid value (e.g. in case there is no art or if upload operation failed).
  
  Before (empty value in `Coal Chamber` means that art won't be reuploaded):
    
    ```json
    {
      "Cloudkicker|The Discovery": "http://my_url.jpg",
      "Coal Chamber|Coal Chamber": ""
    }
    ```

  After:
    ```json
    {
      "Cloudkicker|The Discovery": "http://my_url.jpg"
    }
    ```

- Press `Reload from disk`.
- Start playing the track again to trigger the reupload.

Note: cache file format and it's path are subject to change.

## Technical details

This section is meant for those who want to create their own upload scripts.
The installation script will receive a filepath to the album art encoded in UTF-8 from the stdin pipe.
This filepath might point to a temporary file or a permanent file depending on where the file is originally stored.
Thus you can't rely on the presense of this file after the process exits, since it might be deleted (or ooverwritten) afterwards.

The rough pipeline of the uploading process is as follows.
The `plugin` means `foo_discord_rich` component and the `upload process` is the program that is run to upload the image.

1. The plugin writes the full filepath to `stdin` of the upload process.
1. The upload process should then handle generating a web url that contains a copy of the given artwork file.
   **Important:** the resulting url must be no longer than 254 bytes due to requirements by Discord API.
1. The plugin then waits 10 seconds for the upload process to write anything to the `stdout`.
    1. If something is written to `stdout`, the plugin reads at maximum 2048 bytes of it.
    1. If nothing is written or the process times out, it is considered a failure.
1. The plugin checks the exit code of the upload process. If the exit code is nonzero, it is considered a failure and `stdout` content is disregarded (though it will be logged in `Console`).
