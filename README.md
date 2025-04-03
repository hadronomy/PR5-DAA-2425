<div align="center">
  <img src="/.github/images/github-header-image.webp" alt="GitHub Header Image" />

  <!-- Badges -->
  <p></p>
  <a href="https://ull.es">
    <img
      alt="License"
      src="https://img.shields.io/badge/ULL-5C068C?style=for-the-badge&logo=gitbook&labelColor=302D41"
    />
  </a>
  <a href="https://github.com/hadronomy/PR5-DAA-2425/blob/main/LICENSE">
    <img
      alt="License"
      src="https://img.shields.io/badge/MIT-EE999F?style=for-the-badge&logo=starship&label=LICENSE&labelColor=302D41"
    />
  </a>
  <p></p>
  <!-- TOC -->
  <a href="#docs">Docs</a> •
  <a href="#requirements">Requirements</a> •
  <a href="#build">Build</a> •
  <a href="#usage">Usage</a> •
  <a href="#license">License</a>
  <hr />
</div>

## Docs

This project implements algorithms for the Vehicle Routing Problem with Transshipments for Solid Waste Collection with Transfer Stations (VRPT-SWTS). See the [docs](/docs/P5_DAA_VRPT_2024_2025.pdf) pdf for more information about the assignment and [summary](/docs/summary.md) for a detailed problem description.

## Requirements

This project uses [mise](https://github.com/jdx/mise) for dependency management. Mise provides a unified interface to manage runtime versions and project-specific tools.

To set up the development environment:

1. Install mise following the [official instructions](https://mise.jdx.dev/getting-started.html)
2. Run `mise install` in the project root to automatically install all dependencies defined in `mise.toml`

The project dependencies will be automatically installed and configured according to the project specifications.

After installing the requirements, you can run `just` without arguments to see a list of available tasks:

```bash
just
```

This will display all available commands defined in the justfile that you can use to build, test, and run the project.

## Build

```bash
just build
```

## Usage

```bash
just run --help
```

### Available Commands

- `bench <algorithm> [options]` - Benchmark a specific algorithm
- `compare <algorithm1> <algorithm2> [options]` - Compare multiple algorithms
- `list` - List available algorithms
- `validate <file>` - Validate a solution file
- `visualize <file>` - Visualize a problem instance

### Options

- `--iteration=N` - Number of iterations
- `--file=path` - Input file(s) with problem instances (can specify multiple)
- `--debug` - Enable debug mode output
- `--time-limit=path` - Time limit per algorithm run (e.g '30s', '1m30', '1h', or milliseconds)

### Examples

```bash
# Benchmark greedy CV generator algorithm
tsp bench greedy_cv_generator -f examples/instance1.txt

# Benchmark GRASP CV generator algorithm
tsp bench grasp_cv_generator -f examples/instance2.txt

# Benchmark multi-start algorithm
tsp bench multi_start -f examples/instance3.txt

# Benchmark GVNS algorithm
tsp bench gvns -f examples/instance4.txt

# Compare greedy CV generator and GRASP CV generator
tsp compare greedy_cv_generator grasp_cv_generator -f examples/instance5.txt

# Compare all the algorithms
tsp compare all -f examples/instance1.txt

# List all the available algorithms
tsp list

# Visualize a problem instance
tsp visualize examples/instance1.txt

# Enable debug mode
tsp bench greedy_cv_generator -f examples/instance1.txt --debug
```

## License

This project is licensed under the MIT License -
see the [LICENSE](/LICENSE) file for details.