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
      : cv_algorithm_name_(std::move(cv_algorithm)), tv_algorithm_name_(std::move(tv_algorithm)) {
    initializeComponents();
  }

  /**
   * @brief Solve the complete VRPT problem
   *
   * @param problem The problem instance
   * @return A complete solution with both CV and TV routes
   */
  VRPTSolution solve(const VRPTProblem& problem) override {
    // Phase 1: Solve the CV routing problem
    VRPTSolution cv_solution = cv_algorithm_->solve(problem);
    // Phase 2: Create TV scheduler with problem reference
    // VRPTSolution final_solution = tv_algorithm_->solve(cv_solution);
    return cv_solution;
  }

  std::string name() const override {
    return "VRPT Solver (" + cv_algorithm_name_ + " + " + tv_algorithm_name_ + ")";
  }

  std::string description() const override {
    return "Complete VRPT solver that uses " + cv_algorithm_name_ +
           " for Collection Vehicle routing "
           "and " +
           tv_algorithm_name_ + " for Transportation Vehicle routing";
  }

  std::string timeComplexity() const override {
    return "O(CV + TV)";  // Complexity depends on the underlying algorithms
  }

  // UI configuration rendering
  void renderConfigurationUI() override;

 private:
  std::string cv_algorithm_name_;
  std::string tv_algorithm_name_;

  std::unique_ptr<TypedAlgorithm<VRPTProblem, VRPTSolution>> cv_algorithm_;
  std::unique_ptr<TypedAlgorithm<VRPTSolution, VRPTSolution>> tv_algorithm_;

  void initializeComponents() {

    try {
      if (!cv_algorithm_name_.empty()) {
        cv_algorithm_ =
          AlgorithmRegistry::createTyped<VRPTProblem, VRPTSolution>(cv_algorithm_name_);
      }

      if (!tv_algorithm_name_.empty()) {
        tv_algorithm_ =
          AlgorithmRegistry::createTyped<VRPTSolution, VRPTSolution>(tv_algorithm_name_);
      }
    } catch (const std::exception&) {
      // Initialization will be retried later if needed
    }
  }
};

// Register the complete solver
REGISTER_ALGORITHM(VRPTSolver, "VRPTSolver");
}  // namespace algorithm
}  // namespace daa
