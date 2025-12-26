#pragma once
#include <complex>
#include <stdexcept>

#include "Component.hpp"

class Inductor : public Component {
 public:
  Inductor(const std::string& name, double inductance, int n1, int n2)
      : Component(name, {n1, n2}, inductance) {}

  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double omega) const override;

  // Like a voltage source, an inductor's current is an unknown variable.
  void setCurrentIndex(int index) { _current_idx = index; }
  int getCurrentIndex() const { return _current_idx; }

 private:
  int _current_idx = -1;
  // Value is stored in base class _value
};