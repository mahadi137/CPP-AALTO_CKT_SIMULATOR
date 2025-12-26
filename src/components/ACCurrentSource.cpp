#include "ACCurrentSource.hpp"

#include <complex>

void ACCurrentSource::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  // Pure AC source: no DC contribution.
  (void)A;
  (void)z;
}

void ACCurrentSource::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                              double /*omega*/) const {
  const int n_minus = _nodes[0];
  const int n_plus = _nodes[1];

  // Same sign convention as DC CurrentSource:
  // - current leaving n_minus (negative injection)
  // - current entering n_plus (positive injection)
  if (n_minus != 0) {
    z(n_minus - 1) -= Iac;
  }
  if (n_plus != 0) {
    z(n_plus - 1) += Iac;
  }

  // Ideal independent current source does not touch the conductance matrix.
  (void)A;
}
