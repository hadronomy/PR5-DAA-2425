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

// Solution representation must be included first to resolve forward declarations
#include "algorithms/vrpt_solution.h"

// Base algorithm components
#include "algorithm_factory.h"
#include "algorithm_registry.h"
#include "meta_heuristic.h"
#include "meta_heuristic_components.h"
#include "meta_heuristic_factory.h"

// Include algorithm implementations with direct registration
#include "algorithms/cv_local_search.h"
#include "algorithms/grasp_cv_generator.h"
#include "algorithms/greedy_cv_generator.h"
#include "algorithms/greedy_tv_scheduler.h"
#include "algorithms/gvns.h"
#include "algorithms/multi_start.h"

// Initialize Factory and register global algorithms
inline void initializeAlgorithms() {
  // No additional initialization required as algorithms
  // are registered automatically through REGISTER_* macros
}

// Restore warning settings
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
