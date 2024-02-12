// SPDX-License-Identifier: MIT */
//
// zig.zig -- part of zig.mod
//
// Copyright (c) 2024 Michael Ortmann

const MODULE_NAME: []u8 = "zig";

const c = @cImport({
    @cDefine("STATIC", "1"); // TODO
    @cInclude("src/mod/module.h");
});

const std = @import("std");

export fn zig_start(global_funcs: c.Function) ?[*]const u8 {
    _ = global_funcs;
    // module_register(MODULE_NAME, 0, 0, 1);

    std.io.getStdOut().writer().print("hello from zig.mod zig_start()\n", .{}) catch return null;
    return "WIP".ptr;
}
