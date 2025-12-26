#pragma once

#include <complex>

#include "Component.hpp"

class DCCurrentSource : public Component {
 public:
  // Connects from node n_plus to n_minus.
  DCCurrentSource(const std::string& name, double current, int n_plus,
                  int n_minus)
      : Component(name, {n_plus, n_minus}, current) {}

  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double omega) const override;

 private:
  // double _current;
};