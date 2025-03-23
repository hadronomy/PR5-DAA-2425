const std = @import("std");
const builtin = @import("builtin");

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

    {
        const flex_file = "include/parser/tsp_lexer.l";
        const bison_file = "include/parser/tsp_parser.y";

        // Set binary directory for generated files
        const binary_dir = b.cache_root.path orelse ".";

        // Generate lexer from flex
        const flex_step = try createFlexStep(b, flex_file);

        // Generate parser from bison
        const bison_step = try createBisonStep(b, bison_file);

        // Make lexer depend on parser (add_flex_bison_dependency equivalent)
        flex_step.step.dependOn(&bison_step.step);

        // Custom step to create the 'generated' directory
        const make_gen_dir = b.addWriteFiles();
        _ = make_gen_dir.add(b.pathFromRoot("generated/.gitignore"),
            \\# Ignore all files in this directory
            \\*
            \\
        ); // Discard the returned value
        make_gen_dir.step.dependOn(&flex_step.step);

        // Find and copy generated source files
        const binary_sources = try findFilesInDir(b, binary_dir, &cfiles_exts);
        const binary_headers = try findFilesInDir(b, binary_dir, &header_exts);

        const copy_files = b.addWriteFiles();
        copy_files.step.dependOn(&make_gen_dir.step);

        // Add file contents to copy_files step
        for (binary_sources) |source_file| {
            const abs_path = b.fmt("{s}/{s}", .{ binary_dir, source_file });
            const dest_file = b.pathFromRoot(b.fmt("generated/{s}", .{std.fs.path.basename(source_file)}));

            if (readFileContent(b.allocator, abs_path)) |content| {
                _ = copy_files.add(dest_file, content);
            } else |err| {
                std.debug.print("Error reading file {s}: {any}\n", .{ abs_path, err });
            }
        }

        // Copy header files
        for (binary_headers) |header_file| {
            const abs_path = b.fmt("{s}/{s}", .{ binary_dir, header_file });
            const dest_file = b.pathFromRoot(b.fmt("generated/{s}", .{std.fs.path.basename(header_file)}));

            if (readFileContent(b.allocator, abs_path)) |content| {
                _ = copy_files.add(dest_file, content);
            } else |err| {
                std.debug.print("Error reading file {s}: {any}\n", .{ abs_path, err });
            }
        }

        // Add source files to executable
        var all_sources = std.ArrayList([]const u8).init(b.allocator);
        try all_sources.appendSlice(sources);

        // Find and add the copied files as sources
        var generated_sources = std.ArrayList([]const u8).init(b.allocator);

        // Create the directory if it doesn't exist
        const build_root_path = b.build_root.path orelse ".";
        const generated_dir = b.fmt("{s}/generated", .{build_root_path});
        std.fs.cwd().makePath(generated_dir) catch |err| {
            // If the directory already exists, that's fine
            if (err != error.PathAlreadyExists) {
                std.debug.print("Error creating generated directory: {any}\n", .{err});
            }
        };

        // Try to find files, but don't error if the directory is empty
        if (findFilesRecursive(b, "generated", &cfiles_exts)) |files| {
            try generated_sources.appendSlice(files);
        } else |err| {
            if (err != error.FileNotFound) {
                std.debug.print("Error finding generated files: {any}\n", .{err});
            }
        }

        try all_sources.appendSlice(generated_sources.items);
        exe.step.dependOn(&copy_files.step);
        exe.addCSourceFiles(.{
            .files = all_sources.items,
            .flags = &cppflags,
        });

        // Add specific steps to run just flex or bison
        const flex_run_step = b.step("flex", "Generate lexer from flex file");
        flex_run_step.dependOn(&flex_step.step);

        const bison_run_step = b.step("bison", "Generate parser from bison file");
        bison_run_step.dependOn(&bison_step.step);
    }

    {
        const cli11_src = b.dependency("CLI11", .{
            .target = target,
            .optimize = optimize,
        });
        const cli11_lib = cli11_src.path("./include");
        exe.addIncludePath(cli11_lib);

        const fmt_src = b.dependency("fmt", .{
            .target = target,
            .optimize = optimize,
        });
        const fmt_lib = fmt_src.path("./include");
        const fmt = b.addStaticLibrary(.{
            .name = "fmt",
            .optimize = optimize,
            .target = target,
        });
        fmt.linkLibCpp();
        fmt.addIncludePath(fmt_lib);
        fmt.installHeadersDirectory(
            fmt_lib,
            "fmt",
            .{
                .include_extensions = &[_][]const u8{".h"},
            },
        );
        const fmt_source_files = try findDependencyFiles(
            b,
            fmt_src.path("./src"),
            &cfiles_exts,
        );
        for (fmt_source_files) |file| {
            fmt.addCSourceFile(.{
                .file = file,
                .flags = &cppflags,
            });
        }

        exe.addIncludePath(fmt_lib);
        exe.linkLibrary(fmt);

        const tabulate_src = b.dependency("tabulate", .{
            .target = target,
            .optimize = optimize,
        });
        const tabulate_lib = tabulate_src.path("./include");
        exe.addIncludePath(tabulate_lib);
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

fn createFlexStep(
    b: *std.Build,
    input_file: []const u8,
) !*std.Build.Step.Run {
    if (!try binaryExistsInPath(b.allocator, "flex")) {
        return error.FlexNotFound;
    }

    const flex_cmd = b.addSystemCommand(&[_][]const u8{
        "flex",
        "--c++",
        "--header-file=lex.yy.h",
        b.path(input_file).getPath(b),
    });

    const binary_dir = b.cache_root.path orelse ".";
    flex_cmd.setCwd(.{ .cwd_relative = binary_dir });

    return flex_cmd;
}

fn createBisonStep(
    b: *std.Build,
    input_file: []const u8,
) !*std.Build.Step.Run {
    if (!try binaryExistsInPath(b.allocator, "bison")) {
        return error.FlexNotFound;
    }

    const bison_cmd = b.addSystemCommand(&[_][]const u8{
        "bison",
        "--language=c++",
        "--defines=parser.tab.h",
        b.path(input_file).getPath(b),
    });

    const binary_dir = b.cache_root.path orelse ".";
    bison_cmd.setCwd(.{ .cwd_relative = binary_dir });

    return bison_cmd;
}

fn readFileContent(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    const file = try std.fs.cwd().openFile(path, .{});
    defer file.close();

    const file_size = try file.getEndPos();
    const content = try allocator.alloc(u8, file_size);

    const bytes_read = try file.readAll(content);
    if (bytes_read != file_size) {
        return error.IncompleteRead;
    }

    return content;
}

fn findFilesInDir(b: *std.Build, dir_path: []const u8, exts: []const []const u8) ![][]const u8 {
    var sources = std.ArrayList([]const u8).init(b.allocator);

    var abs_dir = try std.fs.cwd().openDir(dir_path, .{ .iterate = true });
    var iter = abs_dir.iterate();
    defer abs_dir.close();

    while (try iter.next()) |entry| {
        if (entry.kind == .file) {
            const ext = std.fs.path.extension(entry.name);
            const include_file = for (exts) |e| {
                if (std.mem.eql(u8, ext, e)) {
                    break true;
                }
            } else false;

            if (include_file) {
                try sources.append(b.allocator.dupe(u8, entry.name) catch continue);
            }
        }
    }

    return sources.items;
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

fn findDependencyFiles(b: *std.Build, dep_path: std.Build.LazyPath, exts: []const []const u8) ![]const std.Build.LazyPath {
    var sources = std.ArrayList(std.Build.LazyPath).init(b.allocator);
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

pub fn binaryExistsInPath(allocator: std.mem.Allocator, binary_name: []const u8) !bool {
    const env_map_result = std.process.getEnvMap(allocator);
    if (env_map_result == error.OutOfMemory) {
        return error.OutOfMemory;
    }

    var env_map = try env_map_result;
    defer env_map.deinit();

    const path_value = env_map.get("PATH");
    if (path_value == null) {
        return false;
    }

    const path_env = path_value.?;
    // Use builtin.os.tag for OS detection
    const path_sep = if (builtin.os.tag == .windows) ';' else ':';

    var paths = std.mem.splitScalar(u8, path_env, path_sep);
    while (paths.next()) |path| {
        var full_path_buffer: [1024]u8 = undefined;
        const full_path_slice = try std.fmt.bufPrint(&full_path_buffer, "{s}/{s}", .{ path, binary_name });

        // std.fs.access returns an error, not null, so we need to handle it differently
        std.fs.accessAbsolute(full_path_slice, .{ .mode = .read_only }) catch |err| {
            if (err == error.FileNotFound) {
                continue;
            } else {
                std.debug.print("Error checking {s}: {any}\n", .{ full_path_slice, err });
                continue;
            }
        };

        // If we get here, access was successful
        return true;
    }

    return false;
}
