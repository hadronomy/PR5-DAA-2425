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
  <a href="https://github.com/hadronomy/PR3-DAA-2425/blob/main/LICENSE">
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

See the [docs](/docs/assignment.pdf) pdf for more information.

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
- `help` - Show help message

### Options

- `--iteration=N` - Number of iterations
- `--size=N` - Size(s) of test data (can specify multiple)
- `--file=path` - Input file(s) with graphs to benchmark (can specify multiple)
- `--debug` - Enable debug mode output
- `--time-limit=path` - Time limit per algorithm run (e.g '30s', '1m30', '1h', or milliseconds)

### Examples

```bash
# Benchmark brute force timeout
tsp bench brute_force -N 1,5,10,15

# Benchmark dynamic programming with a file
tsp bench dynamic_programming -f examples/4.txt

# Benchmark simulated annealing 
tsp bench simulated_annealing -N 1,5,10,15 

# Compare nearest neighbor and two_opt
tsp compare nearest_neighbor two_opt -N 1,5,10,15

# Compare all the algorithms and do their benchmarks
tsp compare all -N 1,5,10,15

# List all the available algorithms
tsp list

# Get help information
tsp help

# Enable debug mode
tsp bench nearest_neighbor -N 1,5,10,15 --debug
```

## License

This project is licensed under the MIT License -
see the [LICENSE](/LICENSE) file for details.