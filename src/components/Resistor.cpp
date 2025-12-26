#include "Resistor.hpp"

#include <iostream>

void Resistor::stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const {
  const int n1 = _nodes[0];
  const int n2 = _nodes[1];
  const double g = 1.0 / getValue();  // Conductance g

  // A(n1, n1) += g
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += g;
  }
  // A(n2, n2) += g
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) += g;
  }
  // A(n1, n2) -= g and A(n2, n1) -= g
  if (n1 != 0 && n2 != 0) {
    A(n1 - 1, n2 - 1) -= g;
    A(n2 - 1, n1 - 1) -= g;
  }
}

void Resistor::stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                       double /*omega*/) const {
  const int n1 = _nodes[0];
  const int n2 = _nodes[1];
  const double g = 1.0 / getValue();  // Conductance

  // You can let double -> complex<double> convert implicitly,
  // or explicitly make a complex:
  // std::complex<double> gc(g, 0.0);

  // A(n1, n1) += g
  if (n1 != 0) {
    A(n1 - 1, n1 - 1) += g;
  }
  // A(n2, n2) += g
  if (n2 != 0) {
    A(n2 - 1, n2 - 1) += g;
  }
  // A(n1, n2) -= g and A(n2, n1) -= g
  if (n1 != 0 && n2 != 0) {
    A(n1 - 1, n2 - 1) -= g;
    A(n2 - 1, n1 - 1) -= g;
  }
}