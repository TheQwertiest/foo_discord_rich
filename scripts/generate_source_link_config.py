#!/usr/bin/env python3

import argparse
import json
import subprocess
from pathlib import Path
from pathlib import PureWindowsPath
from typing import Union

import call_wrapper

PathLike = Union[str, Path]

def generate_config_custom( base_dir: PathLike,
                            output_dir: PathLike,
                            repo: str = "theqwertiest/foo_discord_rich",
                            commit_hash: str = ""):
    if (not commit_hash):
        commit_hash = subprocess.check_output("git rev-parse --short HEAD", shell=True).decode('ascii').strip()

    base_dir = Path(base_dir).resolve()
    assert(base_dir.exists() and base_dir.is_dir())

    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    output_file = output_dir/'source_link.json'
    output_file.unlink(missing_ok=True)

    data = {}
    data['documents'] = { f'{PureWindowsPath(base_dir)}*': f'https://raw.githubusercontent.com/{repo}/{commit_hash}/*' }

    with open(output_file, 'w') as output:
        json.dump(data, output)

    print(f"Generated file: {output_file}")

def generate_config():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    output_dir = root_dir/"_result"/"AllPlatforms"/"generated"

    generate_config_custom(root_dir, output_dir)

if __name__ == '__main__':
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    output_dir = root_dir/"_result"/"AllPlatforms"/"generated"

    parser = argparse.ArgumentParser(description='Generate source link configuration file')
    parser.add_argument('--base_dir', default=root_dir)
    parser.add_argument('--output_dir', default=output_dir)
    parser.add_argument('--repo', default="theqwertiest/foo_discord_rich")
    parser.add_argument('--commit_hash', default="")

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Generating source link configuration file",
        "Source link configuration file was successfully generated!",
        "Source link configuration file failed!"
    )(
    generate_config_custom
    )(
        args.base_dir,
        args.output_dir,
        args.repo,
        args.commit_hash
    )
