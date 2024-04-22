// SPDX-License-Identifier: MIT */
//
// zig.zig -- part of zig.mod
//
// Copyright (c) 2024 Michael Ortmann

const MODULE_NAME = "zig";

const builtin = @import("builtin");
const c = @cImport({
    @cDefine("STATIC", "1"); // TODO
    @cInclude("src/mod/module.h");
});
const std = @import("std");

const global_funcs = extern struct {
    // 0 - 3
    mod_malloc: *const fn () callconv(.C) void,
    mod_free: *const fn () callconv(.C) void,
    egg_context: *const fn () callconv(.C) void,
    module_rename: *const fn () callconv(.C) void,
    // 4 - 7
    module_register: *const fn ([*]const u8, *const modcall, c_int, c_int) callconv(.C) c_int,
};

const modcall = extern struct {
    start: ?*const fn (*global_funcs) callconv(.C) ?[*]const u8, // MODCALL_START
    close: ?*const fn () callconv(.C) void, // MODCALL_CLOSE
    expmem: ?*const fn () callconv(.C) void, // MODCALL_EXPMEM
    report: ?*const fn () callconv(.C) void, // MODCALL_REPORT
};

export fn zig_report(idx: c_int, details: c_int) void {
    _ = idx;
    _ = details;
}

export fn zig_start(global: *global_funcs) ?[*]const u8 {
    const zig_table = modcall{
        .start = null, //zig_start,
        .close = null,
        .expmem = null,
        .report = null,
    };
    _ = global.module_register(MODULE_NAME.ptr, &zig_table, 0, 1);

    std.io.getStdOut().writer().print("hello from zig.mod zig_start()\n", .{}) catch return null;
    std.io.getStdOut().writer().print("zig version: {s}\n", .{builtin.zig_version_string}) catch return null;
    // return "WIP".ptr;
    return null;
}
