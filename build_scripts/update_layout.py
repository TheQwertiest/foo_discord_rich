#!/usr/bin/env python3

import fileinput
import os
import sys
import urllib.request 
from pathlib import Path

print('Downloading new default layout')

cur_dir = Path(__file__).parent.absolute()
root_dir = cur_dir.parent
layouts_dir = root_dir/"_layouts"
if (not layouts_dir.exists()):
    os.makedirs(layouts_dir)

output_file = layouts_dir/"default.html"

urllib.request.urlretrieve("https://raw.githubusercontent.com/pmarsceill/just-the-docs/master/_layouts/default.html", output_file)

print(f"File downloaded: {output_file}")

for line in fileinput.input(output_file, inplace=True):
    if '</body>' in line:
        sys.stdout.write('\n')
        sys.stdout.write(f'  <!-- This block is added by `{Path(__file__).absolute().relative_to(root_dir)}` -->\n')
        sys.stdout.write('  <script type="text/javascript" src="{{ "/assets/js/lightbox.js" | relative_url }}"></script>\n')
        sys.stdout.write('  <link rel="stylesheet" href="{{ "/assets/css/lightbox.css" | relative_url }}">\n')
        sys.stdout.write('  <script type="text/javascript" src="{{ "/assets/js/slideshow.js" | relative_url }}"></script>\n')
        sys.stdout.write('  <!-- End of custom block -->\n')
    sys.stdout.write(line)

with open(output_file, "a") as f:
    f.write("<!-- build-commit-id: {{ site.github.build_revision }} -->\n")

print(f"File updated: {output_file}")
