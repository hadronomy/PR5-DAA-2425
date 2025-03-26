const std = @import("std");
const builtin = @import("builtin");

const buildpkg = @import("src/build/main.zig");
const config = buildpkg.config;
const utils = buildpkg.utils;

// TODO: Improve upon this
// sources:
// - https://ziggit.dev/docs?topic=3531
// - https://github.com/ghostty-org/ghostty/blob/main/build.zig

comptime {
    buildpkg.zig.requireZig("0.14.0");
}

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const triple = try target.result.linuxTriple(b.allocator);

    const run_step = b.step("run", "Run the app");
    const generate_step = b.step("generate", "Generate lexer and parser files");

    const exe = b.addExecutable(.{
        .name = b.fmt("{s}-{s}-{s}", .{ config.program_name, triple, @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibCpp();
    exe.linkLibC();
    exe.addSystemIncludePath(.{
        .cwd_relative = "/usr/include",
    });
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("include"));
    exe.addIncludePath(b.path("generated"));
    const config_h = b.addConfigHeader(
        .{
            .style = .{ .cmake = b.path("include/config.h.in") },
            .include_path = "config.h",
        },
        .{
            .tsp_VERSION = config.version,
            .PROJECT_NAME = config.program_name,
        },
    );
    const write_config_h = buildpkg.Step.WriteConfigHeader.create(b, .{
        .config_header = config_h,
        .output_dir = b.path("include"),
    });
    exe.step.dependOn(&write_config_h.step);

    // Initialize parser resources
    const parser_resources = try buildpkg.ParserResources.init(
        b,
        "include/parser/tsp_lexer.l",
        "include/parser/tsp_parser.y",
        &config.cfiles_exts,
        &config.header_exts,
        exe,
    );

    // Initialize dependency resources
    linkGlfw(b, exe, target);
    _ = try buildpkg.DependencyResources.init(
        b,
        target,
        optimize,
        &config.cppflags,
        exe,
    );

    // Add source files to the executable
    const sources = try utils.findFilesRecursive(b, "src", &config.cfiles_exts);
    var all_sources = std.ArrayList([]const u8).init(b.allocator);
    try all_sources.appendSlice(sources);

    // Add source files to executable
    exe.addCSourceFiles(.{
        .files = all_sources.items,
        .flags = &config.cppflags,
    });
    _ = try exe.step.addDirectoryWatchInput(b.path("src"));

    // Add specific steps to run just flex or bison
    generate_step.dependOn(parser_resources.step);
    generate_step.dependOn(&write_config_h.step);
    buildpkg.compile_commands.createStep(b, "zcc", .{
        .target = exe,
    });

    b.installArtifact(exe);

    {
        const run_cmd = b.addRunArtifact(exe);
        run_cmd.step.dependOn(b.getInstallStep());
        if (b.args) |args| {
            run_cmd.addArgs(args);
        }

        run_step.dependOn(&run_cmd.step);
    }
}

fn linkGlfw(b: *std.Build, exe: *std.Build.Step.Compile, target: std.Build.ResolvedTarget) void {
    _ = b;
    if (target.query.os_tag == .windows) {
        exe.linkSystemLibrary("gdi32");
        exe.linkSystemLibrary("shell32");
        exe.linkSystemLibrary("opengl32");
    } else {
        exe.linkSystemLibrary("gl");
        exe.linkSystemLibrary("glfw");
    }
}
