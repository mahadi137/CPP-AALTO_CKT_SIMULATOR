#include "VoltageSource.hpp"

void VoltageSource::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  if (_current_idx < 0) {
    throw std::runtime_error(
        "Solver has not assigned a current index to voltage source " + _name);
  }

  const int n_plus = _nodes[0];
  const int n_minus = _nodes[1];
  const int idx = _current_idx;

  if (n_plus != 0) {
    A(n_plus - 1, idx) += 1.0;
  }
  if (n_minus != 0) {
    A(n_minus - 1, idx) -= 1.0;
  }

  // New equation for the voltage source (C matrix part):
  // v_plus - v_minus = V_s
  if (n_plus != 0) {
    A(idx, n_plus - 1) += 1.0;
  }
  if (n_minus != 0) {
    A(idx, n_minus - 1) -= 1.0;
  }

  // Add the known voltage to the source vector 'z'
  z(idx) += getValue();
}

void VoltageSource::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                            double /*omega*/) const {
  // For AC analysis with a purely DC source, usually it behave as a 0 V AC
  // source (i.e. a short in AC/small-signal sense).

  const int n_plus = _nodes[0];
  const int n_minus = _nodes[1];
  const int idx = _current_idx;

  if (n_plus != 0) {
    A(n_plus - 1, idx) += 1.0;
  }
  if (n_minus != 0) {
    A(n_minus - 1, idx) -= 1.0;
  }

  // --- Voltage constraint equation (C matrix part) ---
  // v_plus - v_minus = 0 (AC value of a DC source is 0)
  if (n_plus != 0) {
    A(idx, n_plus - 1) += 1.0;
  }
  if (n_minus != 0) {
    A(idx, n_minus - 1) -= 1.0;
  }

  // DC source has *zero AC amplitude*, so no contribution to z:
  // z(idx) += 0  (do nothing)
  (void)z;  // silence unused warning if needed
}