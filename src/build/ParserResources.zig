const std = @import("std");
const builtin = @import("builtin");

const config = @import("config.zig");
const utils = @import("utils.zig");

const ParserResources = @This();

steps: []*std.Build.Step,
flex_step: ?*std.Build.Step.Run = null,
bison_step: ?*std.Build.Step.Run = null,
binary_dir: []const u8,
step: *std.Build.Step,

pub fn init(
    b: *std.Build,
    flex_file: []const u8,
    bison_file: []const u8,
    cfiles_exts: []const []const u8,
    header_exts: []const []const u8,
    exe: *std.Build.Step.Compile,
) !ParserResources {
    var steps = std.ArrayList(*std.Build.Step).init(b.allocator);
    errdefer steps.deinit();
    const generated_dir = b.path("generated");

    // Set binary directory for generated files
    const binary_dir = try utils.relativePath(b, b.cache_root.path orelse ".");

    // Create directory step
    const dir_step = makeCreateDirStep(b, generated_dir.getPath(b));

    // Create gitignore step
    const gitignore_step = makeGitignoreStep(b, generated_dir.path(b, ".gitignore").getPath(b));
    gitignore_step.dependOn(dir_step);

    // Generate lexer and parser from flex and bison files
    const bison_step = makeBisonStep(b, bison_file, binary_dir);
    bison_step.step.dependOn(gitignore_step);

    const flex_step = makeFlexStep(b, flex_file, binary_dir);
    flex_step.step.dependOn(&bison_step.step);

    // Find, copy files, and add them directly to the executable
    const copy_files_step = makeCopyFilesStep(b, binary_dir, cfiles_exts, header_exts, exe);
    copy_files_step.dependOn(&flex_step.step);
    try steps.append(copy_files_step);

    for (steps.items) |step| {
        exe.step.dependOn(step);
    }

    return .{
        .steps = steps.items,
        .flex_step = flex_step,
        .bison_step = bison_step,
        .binary_dir = binary_dir,
        .step = copy_files_step,
    };
}

fn makeCreateDirStep(b: *std.Build, dir_path: []const u8) *std.Build.Step {
    const CreateDirStep = struct {
        step: std.Build.Step,
        path: []const u8,

        fn make(step_: *std.Build.Step, options: std.Build.Step.MakeOptions) anyerror!void {
            _ = options;
            const self: *@This() = @fieldParentPtr("step", step_);
            std.fs.cwd().makePath(self.path) catch |err| {
                if (err != error.PathAlreadyExists) {
                    std.debug.print("Error creating directory {s}: {any}\n", .{ self.path, err });
                    return err;
                }
            };
        }
    };

    const create_dir_step = b.allocator.create(CreateDirStep) catch @panic("OOM");
    create_dir_step.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = b.fmt("Create directory {s}", .{dir_path}),
            .owner = b,
            .makeFn = CreateDirStep.make,
        }),
        .path = b.allocator.dupe(u8, dir_path) catch @panic("OOM"),
    };

    return &create_dir_step.step;
}

fn makeGitignoreStep(b: *std.Build, gitignore_path: []const u8) *std.Build.Step {
    const GitignoreStep = struct {
        step: std.Build.Step,
        path: []const u8,

        fn make(step_: *std.Build.Step, options: std.Build.Step.MakeOptions) anyerror!void {
            _ = options;
            const self: *@This() = @fieldParentPtr("step", step_);
            const gitignore_content =
                \\# Ignore all files in this directory
                \\*
                \\
            ;
            var gitignore_file = try std.fs.cwd().createFile(self.path, .{});
            defer gitignore_file.close();
            try gitignore_file.writeAll(gitignore_content);
        }
    };

    const gitignore_step = b.allocator.create(GitignoreStep) catch @panic("OOM");
    gitignore_step.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = b.fmt("Create gitignore {s}", .{gitignore_path}),
            .owner = b,
            .makeFn = GitignoreStep.make,
        }),
        .path = b.allocator.dupe(u8, gitignore_path) catch @panic("OOM"),
    };

    return &gitignore_step.step;
}

fn makeFlexStep(
    b: *std.Build,
    input_file: []const u8,
    binary_dir: []const u8,
) *std.Build.Step.Run {
    const binary_exists = binaryExistsInPath(b.allocator, "flex") catch false;
    if (!binary_exists) {
        @panic("Flex not found in PATH");
    }

    const flex_cmd = b.addSystemCommand(&[_][]const u8{
        "flex",
        "--c++",
        "--header-file=lex.yy.h",
    });
    flex_cmd.addFileArg(b.path(input_file));
    flex_cmd.setCwd(.{ .cwd_relative = binary_dir });
    flex_cmd.has_side_effects = true;

    return flex_cmd;
}

fn makeBisonStep(
    b: *std.Build,
    input_file: []const u8,
    binary_dir: []const u8,
) *std.Build.Step.Run {
    const binary_exists = binaryExistsInPath(b.allocator, "bison") catch false;
    if (!binary_exists) {
        @panic("Bison not found in PATH");
    }

    const bison_cmd = b.addSystemCommand(&[_][]const u8{
        "bison",
        "--language=c++",
        "--defines=vrpt_parser.h",
    });
    bison_cmd.addPrefixedFileArg("--defines=", b.path(binary_dir).path(b, "vrpt_parser.h"));
    bison_cmd.addFileArg(b.path(input_file));
    bison_cmd.setCwd(.{ .cwd_relative = binary_dir });
    bison_cmd.has_side_effects = true;

    return bison_cmd;
}

fn makeCopyFilesStep(
    b: *std.Build,
    binary_dir: []const u8,
    cfiles_exts: []const []const u8,
    header_exts: []const []const u8,
    exe: *std.Build.Step.Compile,
) *std.Build.Step {
    const CopyFilesStep = struct {
        step: std.Build.Step,
        build: *std.Build,
        binary_dir: []const u8,
        cfiles_exts: []const []const u8,
        header_exts: []const []const u8,
        exe: *std.Build.Step.Compile,

        fn make(step_: *std.Build.Step, options: std.Build.Step.MakeOptions) anyerror!void {
            _ = options;
            const self: *@This() = @fieldParentPtr("step", step_);

            const binary_sources = try findFilesInDir(self.build, self.binary_dir, self.cfiles_exts);
            const binary_headers = try findFilesInDir(self.build, self.binary_dir, self.header_exts);

            // Track files for proper error handling
            var file_count: usize = 0;
            var error_count: usize = 0;

            // Create generated directory if it doesn't exist
            const generated_dir = self.build.pathFromRoot("generated");
            std.fs.cwd().makePath(generated_dir) catch |err| {
                if (err != error.PathAlreadyExists) {
                    return self.step.addError("Error creating directory {s}: {any}\n", .{ generated_dir, err });
                }
            };

            // Copy source files and add them to executable
            for (binary_sources) |source_file| {
                const src_path = self.build.fmt("{s}/{s}", .{ self.binary_dir, source_file });
                const dest_basename = std.fs.path.basename(source_file);
                const dest_path = self.build.fmt("{s}/{s}", .{ generated_dir, dest_basename });

                // For creating the proper relative path for the build system
                const rel_dest_path = self.build.fmt("generated/{s}", .{dest_basename});

                file_count += 1;

                // Copy the file
                try copyFile(src_path, dest_path);

                // Add to executable using the proper LazyPath syntax with a path relative to build root
                self.exe.addCSourceFile(.{
                    .file = self.build.path(rel_dest_path),
                    .flags = &config.cppflags,
                });
            }

            // Copy header files
            for (binary_headers) |header_file| {
                const src_path = self.build.fmt("{s}/{s}", .{ self.binary_dir, header_file });
                const dest_path = self.build.fmt("{s}/{s}", .{ generated_dir, std.fs.path.basename(header_file) });

                file_count += 1;

                // Copy the file
                copyFile(src_path, dest_path) catch |err| {
                    error_count += 1;
                    std.debug.print("Error copying header file {s}: {any}\n", .{ src_path, err });
                };
            }

            // Make sure we've found at least some files
            if (file_count == 0 or file_count == error_count) {
                std.debug.print("Warning: No files were found or all file copies failed\n", .{});
                if (file_count == 0) {
                    std.debug.print("  No files matched the search criteria\n", .{});
                } else {
                    std.debug.print("  All {d} file operations failed\n", .{file_count});
                }
            }
        }
    };

    const copy_step = b.allocator.create(CopyFilesStep) catch @panic("OOM");
    copy_step.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = "Copy generated files and add to executable",
            .owner = b,
            .makeFn = CopyFilesStep.make,
        }),
        .build = b,
        .binary_dir = b.allocator.dupe(u8, binary_dir) catch @panic("OOM"),
        .cfiles_exts = cfiles_exts,
        .header_exts = header_exts,
        .exe = exe,
    };

    return &copy_step.step;
}

pub fn getGeneratedSourcePaths(self: *const ParserResources, b: *std.Build, allocator: std.mem.Allocator) ![][]const u8 {
    var sources = std.ArrayList([]const u8).init(allocator);

    for (self.generated_sources) |source_name| {
        try sources.append(b.fmt("generated/{s}", .{std.fs.path.basename(source_name)}));
    }

    return sources.items;
}

fn binaryExistsInPath(allocator: std.mem.Allocator, binary_name: []const u8) !bool {
    var env_map = try std.process.getEnvMap(allocator);
    defer env_map.deinit();

    const path_value = env_map.get("PATH") orelse return false;

    // Use builtin.os.tag for OS detection
    const path_sep = if (builtin.os.tag == .windows) ';' else ':';

    // On Windows, you might need to check for .exe extension
    const binary_with_ext = if (builtin.os.tag == .windows and
        !std.mem.endsWith(u8, binary_name, ".exe"))
        try std.fmt.allocPrint(allocator, "{s}.exe", .{binary_name})
    else
        binary_name;
    defer if (binary_with_ext.ptr != binary_name.ptr) allocator.free(binary_with_ext);

    var paths = std.mem.splitScalar(u8, path_value, path_sep);
    while (paths.next()) |path| {
        if (path.len == 0) continue;

        const full_path = try std.fs.path.join(allocator, &[_][]const u8{ path, binary_with_ext });
        defer allocator.free(full_path);

        std.fs.accessAbsolute(full_path, .{ .mode = .read_only }) catch |err| {
            if (err == error.FileNotFound) continue;
            // Just continue on any access error
            continue;
        };

        // If we reach here, the file exists and is accessible
        return true;
    }

    return false;
}

fn findFilesInDir(b: *std.Build, dir_path: []const u8, exts: []const []const u8) ![][]const u8 {
    var sources = std.ArrayList([]const u8).init(b.allocator);

    var abs_dir = std.fs.cwd().openDir(dir_path, .{ .iterate = true }) catch |err| {
        if (err == error.FileNotFound) {
            return sources.items;
        }
        return err;
    };
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

fn readFileContent(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    const file = try std.fs.cwd().openFile(path, .{});
    defer file.close();

    const file_size = try file.getEndPos();
    const content = try allocator.alloc(u8, file_size);
    errdefer allocator.free(content);

    const bytes_read = try file.readAll(content);
    if (bytes_read != file_size) {
        return error.IncompleteRead;
    }

    return content;
}

fn copyFile(src_path: []const u8, dest_path: []const u8) !void {
    const src_file = try std.fs.cwd().openFile(src_path, .{});
    defer src_file.close();

    const dest_file = try std.fs.cwd().createFile(dest_path, .{});
    defer dest_file.close();

    const buffer_size = 8192;
    var buffer: [buffer_size]u8 = undefined;

    while (true) {
        const bytes_read = try src_file.read(&buffer);
        if (bytes_read == 0) break;

        try dest_file.writeAll(buffer[0..bytes_read]);
    }
}
