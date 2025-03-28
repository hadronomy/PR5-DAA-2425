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

// Include core headers first
// IWYU pragma: begin_exports
#include "algorithm_factory.h"
#include "algorithm_registry.h"
// IWYU pragma: end_exports

// Include algorithm implementations with direct registration
// IWYU pragma: begin_exports
// TODO: Add all other algorithm headers here
// IWYU pragma: end_exports

namespace daa {

// Initialize Factory and register global algorithms
inline void initializeAlgorithms() {
  // This function is called implicitly when the header is included
  // You can add any initialization code here if needed
}

}  // namespace daa

// Restore warning settings
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
