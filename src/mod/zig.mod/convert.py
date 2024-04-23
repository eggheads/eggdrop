#!/usr/bin/python

# SPDX-License-Identifier: MIT
#
# convert.py -- part of zig.mod
#
# Copyright (c) 2024 Michael Ortmann

found = 0
foo_i = 0

for line in open("src/modules.c").readlines():
    if not found:
        if line == "Function global_table[] = {\n":
            found = 1
    else:
        if line.lstrip().startswith("/"):
            i = 0
            for i, c in enumerate(line):
                if c.isdigit():
                    break
            for j, c in reversed(list(enumerate(line))):
                if c.isdigit():
                    break
            print("    // %s" % line[i:j + 1])
        elif line.startswith("  ("):
            func_name =line.split(",")[0].split(" ")[-1].strip() 
            if (func_name == "0"):
                func_name = "foo%i" % foo_i
                foo_i += 1
            print("    %s: *const fn () callconv(.C) void," % func_name)
        elif line == "};\n":
            break
