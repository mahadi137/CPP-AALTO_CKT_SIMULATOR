#pragma once
#include <complex>

#include "Component.hpp"

class Capacitor : public Component {
 public:
  Capacitor(const std::string& name, double capacitance, int n1, int n2)
      : Component(name, {n1, n2}, capacitance) {}

  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double omega) const override;

 private:
  // Value is stored in base class _value
};