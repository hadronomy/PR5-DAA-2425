#pragma once

// Temporarily disable unused warnings
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4101)  // Unused parameter and variable warnings
#endif

// IWYU pragma: begin_exports
#include "commands/benchmark.h"
#include "commands/compare.h"
#include "commands/help.h"
#include "commands/list.h"
#include "commands/validate.h"
#include "commands/visualize.h"
// IWYU pragma: end_exports

// Restore warning settings
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
