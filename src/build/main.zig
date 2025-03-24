pub const compile_commands = @import("compile_commands.zig");
pub const config = @import("config.zig");
pub const DependencyResources = @import("DependencyResources.zig");
pub const ParserResources = @import("ParserResources.zig");
pub const utils = @import("utils.zig");
pub const zig = @import("zig.zig");

pub const Step = struct {
    pub const WriteConfigHeader = @import("WriteConfigHeader.zig");
};
