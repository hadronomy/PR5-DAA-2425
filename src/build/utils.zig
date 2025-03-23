const std = @import("std");

pub fn calculateRelativePath(b: *std.Build, base_path: []const u8, target_path: []const u8) ![]const u8 {
    return std.fs.path.relative(b.allocator, base_path, target_path);
}

pub fn relativePath(b: *std.Build, target_path: []const u8) ![]const u8 {
    return calculateRelativePath(b, b.build_root.path orelse ".", target_path);
}
