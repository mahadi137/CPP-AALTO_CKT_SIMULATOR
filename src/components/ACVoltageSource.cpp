// ACVoltageSource.cpp
#include "ACVoltageSource.hpp"

#include <complex>
#include <stdexcept>

void ACVoltageSource::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  const int n_plus = _nodes[0];
  const int n_minus = _nodes[1];
  const int idx = _current_idx;  // row/col for source current

  // KCL contributions (B matrix)
  if (n_plus != 0) {
    A(n_plus - 1, idx) += 1.0;
  }
  if (n_minus != 0) {
    A(n_minus - 1, idx) -= 1.0;
  }

  // Voltage constraint (C matrix)
  if (n_plus != 0) {
    A(idx, n_plus - 1) += 1.0;
  }
  if (n_minus != 0) {
    A(idx, n_minus - 1) -= 1.0;
  }

  // DC value: v_plus - v_minus = V_DC
  z(idx) += _dcOffset;  // 0.0 by default for a pure AC source
}

void ACVoltageSource::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                              double /*omega*/) const {
  const int n_plus = _nodes[0];
  const int n_minus = _nodes[1];
  const int idx = _current_idx;

  // Same topology as DC, but complex
  if (n_plus != 0) {
    A(n_plus - 1, idx) += 1.0;
  }
  if (n_minus != 0) {
    A(n_minus - 1, idx) -= 1.0;
  }

  if (n_plus != 0) {
    A(idx, n_plus - 1) += 1.0;
  }
  if (n_minus != 0) {
    A(idx, n_minus - 1) -= 1.0;
  }
  // AC phasor voltage source
  z(idx) += Vac;
}
