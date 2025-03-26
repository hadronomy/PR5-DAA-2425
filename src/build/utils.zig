const std = @import("std");
const Build = std.Build;

pub fn calculateRelativePath(b: *std.Build, base_path: []const u8, target_path: []const u8) ![]const u8 {
    return std.fs.path.relative(b.allocator, base_path, target_path);
}

pub fn relativePath(b: *std.Build, target_path: []const u8) ![]const u8 {
    return calculateRelativePath(b, b.build_root.path orelse ".", target_path);
}

pub fn findFilesRecursive(b: *std.Build, dir_name: []const u8, exts: []const []const u8) ![][]const u8 {
    var sources = std.ArrayList([]const u8).init(b.allocator);

    var dir = try b.build_root.handle.openDir(dir_name, .{ .iterate = true });
    var walker = try dir.walk(b.allocator);
    defer walker.deinit();
    while (try walker.next()) |entry| {
        const ext = std.fs.path.extension(entry.basename);
        const include_file = for (exts) |e| {
            if (std.mem.eql(u8, ext, e)) {
                break true;
            }
        } else false;
        if (include_file) {
            try sources.append(b.fmt("{s}/{s}", .{ dir_name, entry.path }));
        }
    }

    return sources.items;
}

pub fn findDependencyFiles(b: *Build, dep_path: Build.LazyPath, exts: []const []const u8) ![]const Build.LazyPath {
    var sources = std.ArrayList(Build.LazyPath).init(b.allocator);
    const abs_path = dep_path.getPath(b);

    // Check if the path exists
    var dir = std.fs.cwd().openDir(abs_path, .{ .iterate = true }) catch |err| {
        std.debug.print("Error opening dependency directory '{s}': {any}\n", .{ abs_path, err });
        return error.DependencyPathNotFound;
    };
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;

        const ext = std.fs.path.extension(entry.basename);
        const include_file = for (exts) |e| {
            if (std.mem.eql(u8, ext, e)) {
                break true;
            }
        } else false;

        if (include_file) {
            // The path field already contains the relative path from the root directory
            // Just use it directly to create a LazyPath
            const file_path = dep_path.path(b, entry.path);
            try sources.append(file_path);
        }
    }

    return sources.items;
}
