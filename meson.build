project('PR5-DAA-2425', 'cpp',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
  ]
)

# Initialize Zig as the compiler frontend
zig = find_program('zig', required: true)

# Include directories
inc_dirs = include_directories(
  'include',
  'src',
  'generated'
)

# System dependencies
cpp_args = [
  '-DASIO_HAS_THREADS',
  '-fcolor-diagnostics', 
  # '-Wall',
  # '-Wextra',
  # '-Wpedantic',
  '-Wno-deprecated-declarations',
  '-Wno-unqualified-std-cast-call',
  '-Wno-bitwise-instead-of-logical'
]

# Check if flex/bison are available
flex = find_program('flex', required: true)
bison = find_program('bison', required: true)

# Generate parser using Zig's build system
if flex.found() and bison.found()
  # Run zig build generate which will also export dependencies
  message('Running zig build generate to create parser/lexer and export dependencies')
  run_command(zig, 'build', 'generate', check: true)
endif

# Find all source files in src directory
src_files = []
src_dir_exists = run_command('test', '-d', 'src', check: false).returncode() == 0

if src_dir_exists
  # Use a simpler approach with direct extension matching
  foreach ext : ['c', 'cpp', 'cc', 'cxx', 'c++']
    cmd_result = run_command('find', 'src', '-type', 'f', '-name', '*.' + ext, check: false)
    if cmd_result.returncode() == 0
      output = cmd_result.stdout().strip()
      if output != ''
        found_files = output.split('\n')
        src_files += found_files
      endif
    endif
  endforeach
  
  # Print found source files for debugging
  message('Source files found: ', src_files.length() > 0 ? src_files : 'none')
else
  message('Warning: src directory does not exist')
endif

# Find all generated source files in generated directory
gen_files = []
gen_dir_exists = run_command('test', '-d', 'generated', check: false).returncode() == 0

if gen_dir_exists
  # Use same approach for generated files
  foreach ext : ['c', 'cpp', 'cc', 'cxx', 'c++']
    cmd_result = run_command('find', 'generated', '-type', 'f', '-name', '*.' + ext, check: false)
    if cmd_result.returncode() == 0
      output = cmd_result.stdout().strip()
      if output != ''
        found_files = output.split('\n')
        gen_files += found_files
      endif
    endif
  endforeach
  
  # Print found generated files for debugging
  message('Generated files found: ', gen_files.length() > 0 ? gen_files : 'none')
else
  message('Warning: generated directory does not exist')
endif

# Read dependencies info from JSON if available
dependency_includes = []
deps_json_path = 'generated/dependencies.json'

# Import the filesystem module
fs = import('fs')

deps_json = ''
if fs.exists(deps_json_path)
  deps_json = run_command('cat', deps_json_path, check: false).stdout().strip()
endif

# Dependencies
if deps_json != ''
  # Parse JSON to get dependency paths from Zig
  json = import('json')
  deps_data = json.loads(deps_json)
  
  # CLI11
  cli11_dep = declare_dependency(
    include_directories: include_directories(deps_data.get('cli11').get('include_dir'))
  )
  dependency_includes += include_directories(deps_data.get('cli11').get('include_dir'))
  
  # fmt
  fmt_dep = declare_dependency(
    include_directories: include_directories(deps_data.get('fmt').get('include_dir'))
  )
  dependency_includes += include_directories(deps_data.get('fmt').get('include_dir'))
  
  # tabulate
  tabulate_dep = declare_dependency(
    include_directories: include_directories(deps_data.get('tabulate').get('include_dir'))
  )
  dependency_includes += include_directories(deps_data.get('tabulate').get('include_dir'))
  
  message('Using dependency paths exported from Zig')
else
  # Fallback to traditional dependency resolution
  message('No dependency paths found from Zig, falling back to Meson dependencies')
  
  # CLI11
  cli11_dep = dependency('CLI11', required: false)
  if not cli11_dep.found()
    # Fallback to looking for CLI11 as a subproject
    cli11_proj = subproject('CLI11', required: false)
    if cli11_proj.found()
      cli11_dep = cli11_proj.get_variable('CLI11_dep')
      dependency_includes += include_directories(cli11_proj.get_variable('CLI11_include_dir'))
    endif
  endif

  # fmt
  fmt_dep = dependency('fmt', required: false)
  if not fmt_dep.found()
    # Fallback to looking for fmt as a subproject
    fmt_proj = subproject('fmt', required: false)
    if fmt_proj.found()
      fmt_dep = fmt_proj.get_variable('fmt_dep')
      dependency_includes += include_directories(fmt_proj.get_variable('fmt_include_dir'))
    endif
  endif

  # tabulate
  tabulate_dep = dependency('tabulate', required: false)
  if not tabulate_dep.found()
    # Fallback to looking for tabulate as a subproject
    tabulate_proj = subproject('tabulate', required: false)
    if tabulate_proj.found()
      tabulate_dep = tabulate_proj.get_variable('tabulate_dep')
      dependency_includes += include_directories(tabulate_proj.get_variable('tabulate_include_dir'))
    endif
  endif
endif

# Create the executable
executable('pr5', 
  sources: src_files + gen_files,
  include_directories: [inc_dirs, dependency_includes],
  dependencies: [cli11_dep, fmt_dep, tabulate_dep],
  cpp_args: cpp_args,
  install: true
)