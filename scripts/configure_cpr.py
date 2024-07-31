#!/usr/bin/env python3

import shutil
from pathlib import Path

import call_wrapper

def configure():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    submodule_dir = root_dir/"submodules"/"cpr"
    assert(submodule_dir.exists() and submodule_dir.is_dir())

    shutil.copy2(cur_dir/"additional_files"/"cpr.vcxproj", str(submodule_dir) + '/')

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Configuring CPR",
        "Configuring CPR: success",
        "Configuring CPR: failure!"
    )(configure)()
