#include "problem/vrpt_problem.h"
#include "algorithms/vrpt_solution.h"

namespace daa {

double VRPTProblem::evaluateSolution(const algorithm::VRPTSolution& solution) const {
  // Primary objective: minimize number of Collection Vehicles
  double cv_cost = static_cast<double>(solution.getCVCount());

  // Secondary objective: minimize number of Transportation Vehicles
  // This is weighted lower to prioritize CV count
  double tv_cost = solution.isComplete() ? 0.01 * solution.getTVCount() : 0.0;

  return cv_cost + tv_cost;
}

}  // namespace daa