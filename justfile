project_name := "PR3-DAA-2425"
dir_path := `realpath .`
dir_name := `basename $(realpath .)`
bin_name := "tsp"
hash_file := "build/.build_hash"

_default:
    just --list -u

# Configure the project with CMake
configure build_type="RelWithDebInfo":
    cmake -S . -B build -DCMAKE_BUILD_TYPE={{build_type}}

# Generate hash of source and include files, incorporating build type
_generate_hash build_type="RelWithDebInfo":
    @mkdir -p build
    @find src include -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | sort | xargs sha256sum 2>/dev/null | sha256sum | cut -d ' ' -f 1 > "{{hash_file}}.tmp"
    @echo "BUILD_TYPE={{build_type}}" >> "{{hash_file}}.tmp"
    @cat "{{hash_file}}.tmp" | sha256sum | cut -d ' ' -f 1 > "{{hash_file}}.new"
    @rm "{{hash_file}}.tmp"

# Build the project (depends on configure)
build build_type="RelWithDebInfo": (configure build_type)
    @just _generate_hash "{{build_type}}"
    cmake --build build
    @cp "{{hash_file}}.new" "{{hash_file}}"

# Clean build artifacts
clean:
    rm -rf build

# Create a tarball of the project
tar:
    cd .. && tar cvfz ./{{dir_name}}/Team2-{{project_name}}.tar.gz --exclude-from={{dir_name}}/.gitignore {{dir_name}}

# Check if sources have changed
_needs_rebuild build_type="RelWithDebInfo":
    #!/usr/bin/env bash
    if [ ! -f "{{hash_file}}" ] || [ ! -f "build/{{bin_name}}" ]; then
        echo "Hash file or executable missing - rebuild needed" >&2
        echo "true"
        exit 0
    fi
    
    # Generate new hash from current source files and build type
    just _generate_hash "{{build_type}}"
    
    # Debug output
    echo "Previous hash: $(cat {{hash_file}})" >&2
    echo "Current hash:  $(cat {{hash_file}}.new)" >&2
    
    # Compare hashes
    if ! cmp -s "{{hash_file}}" "{{hash_file}}.new"; then
        echo "Source files or build type changed - rebuild needed" >&2
        echo "true"
    else
        echo "No changes detected" >&2
        echo "false"
    fi

# Run the executable, only rebuilding if source has changed
run *ARGS:
    #!/usr/bin/env bash
    build_type="RelWithDebInfo"
    rest_args=""
    
    # Process arguments
    for arg in {{ARGS}}; do
        if [[ "$arg" == +build_type=* ]]; then
            build_type="${arg#+build_type=}"
            echo "Using build type: $build_type"
        else
            rest_args="$rest_args $arg"
        fi
    done
    
    # Trim leading space
    rest_args="${rest_args# }"
    
    # Check if rebuild needed with current build type
    needs_rebuild=$(just _needs_rebuild "$build_type")
    if [ "$needs_rebuild" = "true" ]; then
        echo "Source files or build type changed, rebuilding with build type: $build_type"
        just build "$build_type"
    else
        echo "No changes detected, skipping build"
    fi
    
    # Run with arguments
    if [ -z "$rest_args" ]; then
        echo "No arguments provided, showing help"
        ./build/{{bin_name}} --help
    else
        ./build/{{bin_name}} $rest_args
    fi