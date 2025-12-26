#include "Inductor.hpp"

void Inductor::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  // In DC analysis, an inductor acts as a short circuit.
  // To find the current through it, we model it as a 0-volt voltage source.
  const int n1 = _nodes[0];
  const int n2 = _nodes[1];
  const int idx = _current_idx;

  // Contribution to KCL (B matrix part)
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += 1.0;
  }
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) -= 1.0;
  }

  // New equation for the inductor's voltage (C matrix part)
  // v1 - v2 = 0
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += 1.0;
  }
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) -= 1.0;
  }
}

void Inductor::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                       double omega) const {
  const int n1 = _nodes[0];
  const int n2 = _nodes[1];

  const double L = getValue();

  // s = j*omega, inductor admittance Y_L = 1 / (s * L) = -j / (omega * L)
  std::complex<double> s(0.0, omega);
  std::complex<double> yL = 1.0 / (s * L);

  // Same pattern as a resistor with conductance = yL
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += yL;
  }
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) += yL;
  }
  if (n1 != 0 && n2 != 0) {
    A(n1 - 1, n2 - 1) -= yL;
    A(n2 - 1, n1 - 1) -= yL;
  }

  // No independent source term
  (void)z;
}