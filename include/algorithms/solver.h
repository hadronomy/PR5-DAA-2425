#pragma once

#include <memory>
#include <string>

#include "algorithm_registry.h"
#include "algorithms/greedy_tv_scheduler.h"
#include "algorithms/vrpt_solution.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {
/**
 * @brief Complete VRPT solver that combines Phase 1 and Phase 2
 *
 * This algorithm ties together the CV routing (Phase 1) and TV routing (Phase 2)
 * to provide a complete solution to the VRPT problem.
 */
class VRPTSolver : public TypedAlgorithm<VRPTProblem, VRPTSolution> {
 public:
  /**
   * @brief Constructor with algorithm selection
   *
   * @param cv_algorithm The algorithm to use for CV routing (Phase 1)
   * @param tv_algorithm The algorithm to use for TV routing (Phase 2)
   */
  VRPTSolver(
    std::string cv_algorithm = "MultiStart",
    std::string tv_algorithm = "GreedyTVScheduler"
  )
      : cv_algorithm_(std::move(cv_algorithm)), tv_algorithm_(std::move(tv_algorithm)) {}

  /**
   * @brief Solve the complete VRPT problem
   *
   * @param problem The problem instance
   * @return A complete solution with both CV and TV routes
   */
  VRPTSolution solve(const VRPTProblem& problem) override {
    // Phase 1: Solve the CV routing problem
    auto cv_solver = AlgorithmRegistry::createTyped<VRPTProblem, VRPTSolution>(cv_algorithm_);
    VRPTSolution cv_solution = cv_solver->solve(problem);

    // Phase 2: Create TV scheduler with problem reference
    GreedyTVScheduler scheduler(problem);
    VRPTSolution final_solution = scheduler.solve(cv_solution);

    return final_solution;
  }

  std::string name() const override {
    return "VRPT Solver (" + cv_algorithm_ + " + " + tv_algorithm_ + ")";
  }

  std::string description() const override {
    return "Complete VRPT solver that uses " + cv_algorithm_ +
           " for Collection Vehicle routing "
           "and " +
           tv_algorithm_ + " for Transportation Vehicle routing";
  }

  std::string timeComplexity() const override {
    return "O(CV + TV)";  // Complexity depends on the underlying algorithms
  }

 private:
  std::string cv_algorithm_;
  std::string tv_algorithm_;
};

// Register the complete solver
REGISTER_ALGORITHM(VRPTSolver, "VRPTSolver");
}  // namespace algorithm
}  // namespace daa
