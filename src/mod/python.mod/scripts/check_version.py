# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2025 Michael Ortmann MIT License
# Copyright (C) 2025 Eggheads Development Team
# We use ruff as a code formatter to keep the code style consistent (and to
# avoid off-topic discussions)
import eggdrop
from eggdrop.tcl import putlog
import json
import threading
import urllib.request

current_version = eggdrop.tcl.set("version")
index1 = current_version.find(" ")
index2 = current_version.find("+")
current_version = "v" + current_version[: min(index1, index2)]


def check_version():
    with urllib.request.urlopen(
        "https://api.github.com/repos/eggheads/eggdrop/releases/latest"
    ) as fp:
        latest_version = json.load(fp)["tag_name"]
        if current_version != latest_version:
            putlog(
                f"Version check: eggdrop update found: latest {latest_version} current {current_version}"
            )
    threading.Timer(24 * 60 * 60, check_version).start()  # 24h


check_version()
print("Loaded check_version.py")
