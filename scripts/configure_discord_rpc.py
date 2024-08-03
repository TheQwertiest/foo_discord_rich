#!/usr/bin/env python3

import shutil
from pathlib import Path

import call_wrapper

def configure():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    discord_dir = root_dir/"submodules"/"discord-rpc"
    assert(discord_dir.exists() and discord_dir.is_dir())

    shutil.copy2(cur_dir/"additional_files"/"discord-rpc.vcxproj", str(discord_dir) + '/')

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Configuring Discord RPC",
        "Configuring Discord RPC: success",
        "Configuring Discord RPC: failure!"
    )(configure)()
