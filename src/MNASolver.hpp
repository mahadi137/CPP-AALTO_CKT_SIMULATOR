#pragma once

#include <Eigen/Dense>
#include <complex>

#include "Circuit.hpp"

// The MNASolver class is responsible for taking a Circuit object,
// constructing the MNA matrices, and solving the system of linear equations.
class MNASolver {
 public:
  static Eigen::VectorXd solveDC(Circuit& circuit);
  static Eigen::VectorXcd solveAC(Circuit& circuit, double omega);

 private:
  MNASolver() = default;
};