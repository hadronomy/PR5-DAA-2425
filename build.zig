const std = @import("std");
const builtin = @import("builtin");

const DependencyResources = @import("src/build/DependencyResources.zig");
const ParserResources = @import("src/build/ParserResources.zig");

const cfiles_exts = [_][]const u8{ ".c", ".cpp", ".cxx", ".c++", ".cc" };
const header_exts = [_][]const u8{ ".h", ".hpp", ".hxx", ".h++", ".hh" };
const program_name = "main";

const Extension = enum {
    @".c",
    @".cpp",
    @".m",
};

const cppflags = [_][]const u8{
    "-DASIO_HAS_THREADS",
    "-fcolor-diagnostics",
    "-std=c++20",
    // "-Wall",
    // "-Wextra",
    // "-Werror",
    // "-Wpedantic",
    "-Wno-deprecated-declarations",
    "-Wno-unqualified-std-cast-call",
    "-Wno-bitwise-instead-of-logical", //for notcurses
    "-fno-sanitize=undefined",
};

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const triple = try target.result.linuxTriple(b.allocator);
    const sources = try findFilesRecursive(b, "src", &cfiles_exts);

    const exe = b.addExecutable(.{
        .name = b.fmt("{s}-{s}-{s}", .{ program_name, triple, @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });

    if (optimize != .Debug) {
        switch (target.result.os.tag) {
            .windows => exe.subsystem = .Windows,
            else => exe.subsystem = .Posix,
        }
    }

    exe.linkLibCpp();
    exe.linkLibC();
    exe.addSystemIncludePath(.{
        .cwd_relative = "/usr/include",
    });
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("include"));
    exe.addIncludePath(b.path("generated"));
    const config_h = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("include/config.h.in") },
        .include_path = "config.h",
    }, .{
        .tsp_VERSION = "0.1.0",
        .PROJECT_NAME = "tsp", // TODO: get from config
    });
    exe.addConfigHeader(config_h); // TODO: Find a way to write the file

    // Initialize parser resources
    const parser_resources = try ParserResources.init(b, "include/parser/tsp_lexer.l", "include/parser/tsp_parser.y", &cfiles_exts, &header_exts, exe // Pass the executable here
    );
    parser_resources.install(exe);

    // Initialize dependency resources
    const dep_resources = try DependencyResources.init(
        b,
        target,
        optimize,
        &cppflags,
    );
    dep_resources.install(exe);

    // Add source files to the executable
    var all_sources = std.ArrayList([]const u8).init(b.allocator);
    try all_sources.appendSlice(sources);

    // Add source files to executable
    exe.addCSourceFiles(.{
        .files = all_sources.items,
        .flags = &cppflags,
    });

    // Add specific steps to run just flex or bison
    const flex_run_step = b.step("flex", "Generate lexer from flex file");
    if (parser_resources.flex_step) |flex_step| {
        flex_run_step.dependOn(&flex_step.step);
    }

    const bison_run_step = b.step("bison", "Generate parser from bison file");
    if (parser_resources.bison_step) |bison_step| {
        bison_run_step.dependOn(&bison_step.step);
    }

    b.installArtifact(exe);

    {
        const run_cmd = b.addRunArtifact(exe);
        run_cmd.step.dependOn(b.getInstallStep());
        if (b.args) |args| {
            run_cmd.addArgs(args);
        }

        const run_step = b.step("run", "Run the app");
        run_step.dependOn(&run_cmd.step);
    }
}

fn findFilesRecursive(b: *std.Build, dir_name: []const u8, exts: []const []const u8) ![][]const u8 {
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
