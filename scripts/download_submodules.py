#!/usr/bin/env python3

import subprocess
from pathlib import Path

import call_wrapper

def download_submodule(root_dir, submodule_name):
    print(f"Downloading {submodule_name}...")
    try:
        subprocess.check_call(f"git submodule update --init --depth=10 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
    except subprocess.CalledProcessError:
        try:
            subprocess.check_call(f"git submodule update --init --depth=50 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
        except subprocess.CalledProcessError:
            # Shallow copy does not honour default branch config
            subprocess.check_call("git config --add remote.origin.fetch +refs/heads/*:refs/remotes/origin/*",
                                  cwd=root_dir/"submodules"/submodule_name,
                                  shell=True)
            subprocess.check_call(f"git submodule deinit --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)
            subprocess.check_call(f"git submodule update --init --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)

def download():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent

    subprocess.check_call("git submodule sync", cwd=root_dir, shell=True)
    subprocess.check_call("git submodule foreach git reset --hard", cwd=root_dir, shell=True)
    for subdir in [f for f in (root_dir/"submodules").iterdir() if f.is_dir()]:
        download_submodule(root_dir, subdir.name)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Downloading submodules",
        "Downloading submodules: success",
        "Downloading submodules: failure!"
    )(download)()
