// SPDX-License-Identifier: MIT */
//
// zig.zig -- part of zig.mod
//
// Copyright (c) 2024 Michael Ortmann

const std = @import("std");

export fn zig_start() ?[*]const u8 {
  const stdout = std.io.getStdOut().writer();
  stdout.print("Hello, {s}!\n", .{"world"}) catch return null;
  return "WIP".ptr;
}
