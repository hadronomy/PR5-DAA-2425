pub const cfiles_exts = [_][]const u8{ ".c", ".cpp", ".cxx", ".c++", ".cc" };
pub const header_exts = [_][]const u8{ ".h", ".hpp", ".hxx", ".h++", ".hh" };
pub const program_name = "main";

pub const Extension = enum {
    @".c",
    @".cpp",
    @".m",
};

pub const cppflags = [_][]const u8{
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
