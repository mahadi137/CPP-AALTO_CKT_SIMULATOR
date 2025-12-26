#pragma once

#include <complex>

#include "Component.hpp"

class Resistor : public Component {
 public:
  Resistor(const std::string& name, double resistance, int n1, int n2)
      : Component(name, {n1, n2}, resistance) {}

  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double /*omega*/) const override;

 private:
  // Value is stored in base class _value
};