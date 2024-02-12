// SPDX-License-Identifier: MIT */
//
// zig.zig -- part of zig.mod
//
// Copyright (c) 2024 Michael Ortmann

const MODULE_NAME = "zig";

const c = @cImport({
    @cDefine("STATIC", "1"); // TODO
    @cInclude("src/mod/module.h");
});
const std = @import("std");

const global_funcs = extern struct {
    // 0 - 3
    mod_malloc: *const fn () c_int,
    mod_free: *const fn () c_int,
    egg_context: *const fn () c_int,
    module_rename: *const fn () c_int,
    // 4 - 7
    module_register: *const fn ([]const u8, c_int, c_int, c_int) c_int,
};

export fn zig_start(global: *global_funcs) ?[*]const u8 {
    _ = global.module_register(MODULE_NAME, 5, 7, 11);

    std.io.getStdOut().writer().print("hello from zig.mod zig_start()\n", .{}) catch return null;
    // return "WIP".ptr;
    return null;
}
