#include "DCCurrentSource.hpp"

void DCCurrentSource::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  const int n_minus = _nodes[0];
  const int n_plus = _nodes[1];

  if (n_minus != 0) {
    z(n_minus - 1) -= getValue();
  }
  if (n_plus != 0) {
    z(n_plus - 1) += getValue();
  }
}

void DCCurrentSource::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                              double /*omega*/) const {
  // Constant DC current source has zero AC small-signal value.
  // No contribution to A or z in AC analysis.
  (void)A;
  (void)z;
}