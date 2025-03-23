project_name := "PR5-DAA-2425"
dir_path := `realpath .`
dir_name := `basename $(realpath .)`
bin_name := "tsp"
hash_file := "build/.build_hash"

_default:
    just --list -u

# Build the project (depends on configure)
build *ARGS:
    zig build {{ARGS}}

# Clean build artifacts
clean:
    rm -rf .zig-cache generated

# Create a tarball of the project
tar:
    cd .. && tar cvfz ./{{dir_name}}/{{project_name}}.tar.gz --exclude-from={{dir_name}}/.gitignore {{dir_name}}

# Run the executable, only rebuilding if source has changed
run *ARGS:
    zig build run {{ARGS}}