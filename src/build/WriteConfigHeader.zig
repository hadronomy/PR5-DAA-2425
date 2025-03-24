const std = @import("std");
const Step = std.Build.Step;
const Allocator = std.mem.Allocator;
const LazyPath = std.Build.LazyPath;

const WriteConfigHeader = @This();

step: Step,
config_header: *std.Build.Step.ConfigHeader,
output_dir: LazyPath,
max_bytes: usize,

pub const base_id: Step.Id = .custom;

pub const Options = struct {
    config_header: *std.Build.Step.ConfigHeader,
    output_dir: LazyPath,
};

pub fn create(owner: *std.Build, options: Options) *WriteConfigHeader {
    const write_config_header = owner.allocator.create(WriteConfigHeader) catch @panic("OOM");

    const include_path = options.config_header.include_path;
    const style = options.config_header.style;

    const name = if (style.getPath()) |_|
        owner.fmt("writing configure {s} header {s} to {s}", .{
            @tagName(style), include_path, options.output_dir.getDisplayName(),
        })
    else
        owner.fmt("writing configure {s} header to {s}", .{ @tagName(style), include_path });

    write_config_header.* = .{
        .step = Step.init(.{
            .id = base_id,
            .name = name,
            .owner = owner,
            .makeFn = make,
        }),
        .config_header = options.config_header,
        .output_dir = options.output_dir,
        .max_bytes = options.config_header.max_bytes,
    };

    write_config_header.step.dependOn(&write_config_header.config_header.step);

    return write_config_header;
}

pub fn make(step: *Step, options: Step.MakeOptions) anyerror!void {
    _ = options;
    const b = step.owner;
    const write_config_header: *WriteConfigHeader = @fieldParentPtr("step", step);

    const gpa = b.allocator;
    const arena = b.allocator;

    const include_path = write_config_header.config_header.include_path;
    const output_dir = write_config_header.output_dir.path(b, include_path).getPath2(b, &write_config_header.step);

    // Use dupeZ to make sure we have a proper owned copy of the path string
    const header_file = try b.allocator.dupe(u8, write_config_header.config_header.output_file.getPath());
    defer b.allocator.free(header_file);

    var output = std.ArrayList(u8).init(gpa);
    defer output.deinit();

    // Read the config header output
    const contents = std.fs.cwd().readFileAlloc(
        arena,
        header_file,
        write_config_header.max_bytes,
    ) catch |err| {
        return step.fail("unable to read config header file at {s}: {s}", .{ header_file, @errorName(err) });
    };
    try output.appendSlice(contents);

    // Write the config header to the output directory
    try std.fs.cwd().writeFile(.{
        .sub_path = output_dir,
        .data = output.items,
    });

    // TODO: Add cache
    // checkout: https://github.com/ziglang/zig/blob/master/lib/std/Build/Step/ConfigHeader.zig
}
