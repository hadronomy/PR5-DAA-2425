const std = @import("std");

const DependencyResources = @This();

steps: []*std.Build.Step,
fmt_lib: *std.Build.Step.Compile,
cli11_include: std.Build.LazyPath,
tabulate_include: std.Build.LazyPath,
fmt_lib_include: std.Build.LazyPath,

pub fn init(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cppflags: []const []const u8,
) !DependencyResources {
    var steps = std.ArrayList(*std.Build.Step).init(b.allocator);
    errdefer steps.deinit();

    // CLI11
    const cli11_src = b.dependency("CLI11", .{
        .target = target,
        .optimize = optimize,
    });
    const cli11_lib = cli11_src.path("./include");

    // FMT
    const fmt_src = b.dependency("fmt", .{
        .target = target,
        .optimize = optimize,
    });
    const fmt_lib_path = fmt_src.path("./include");
    const fmt = b.addStaticLibrary(.{
        .name = "fmt",
        .optimize = optimize,
        .target = target,
    });
    fmt.linkLibCpp();
    fmt.addIncludePath(fmt_lib_path);
    fmt.installHeadersDirectory(
        fmt_lib_path,
        "",
        .{
            .include_extensions = &[_][]const u8{".h"},
        },
    );

    // Find FMT source files
    const fmt_source_files = try findDependencyFiles(
        b,
        fmt_src.path("./src"),
        &[_][]const u8{ ".c", ".cpp", ".cxx", ".c++", ".cc" },
    );

    for (fmt_source_files) |file| {
        fmt.addCSourceFile(.{
            .file = file,
            .flags = cppflags,
        });
    }
    try steps.append(&fmt.step);

    // Tabulate
    const tabulate_src = b.dependency("tabulate", .{
        .target = target,
        .optimize = optimize,
    });
    const tabulate_lib = tabulate_src.path("./include");

    return .{
        .steps = steps.items,
        .fmt_lib = fmt,
        .cli11_include = cli11_lib,
        .tabulate_include = tabulate_lib,
        .fmt_lib_include = fmt_lib_path,
    };
}

pub fn install(self: *const DependencyResources, exe: *std.Build.Step.Compile) void {
    // Add dependencies to the executable
    exe.addIncludePath(self.cli11_include);
    exe.addIncludePath(self.tabulate_include);
    exe.linkLibrary(self.fmt_lib);

    // Add dependency on all steps
    for (self.steps) |step| {
        exe.step.dependOn(step);
    }
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
