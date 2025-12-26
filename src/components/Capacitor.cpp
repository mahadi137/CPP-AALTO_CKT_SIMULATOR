#include "Capacitor.hpp"

void Capacitor::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  (void)A;
  (void)z;
}

void Capacitor::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                        double omega) const {
  const int n1 = _nodes[0];
  const int n2 = _nodes[1];

  // capacitance in farads
  const double C = getValue();
  // Admittance Yc = j * omega * C
  std::complex<double> Yc(0.0, omega * C);

  // Diagonal terms
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += Yc;
  }
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) += Yc;
  }

  // Off-diagonal terms
  if (n1 != 0 && n2 != 0) {
    A(n1 - 1, n2 - 1) -= Yc;
    A(n2 - 1, n1 - 1) -= Yc;
  }

  // No direct contribution to z for an ideal passive capacitor.
  (void)z;
}