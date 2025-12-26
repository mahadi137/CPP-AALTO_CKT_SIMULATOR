#pragma once

#include <complex>

#include "Component.hpp"

class ACCurrentSource : public Component {
 public:
  ACCurrentSource(const std::string& name, double current, int n_plus,
                  int n_minus)
      : Component(name, {n_plus, n_minus}, current) {}

  // Pure AC source -> no DC contribution.
  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;

  // AC small-signal phasor stamping.
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double omega) const override;
  // AC phasor of the current source:
  std::complex<double> Iac = std::polar(getValue(), _phase);

 private:
  double _phase = 0.0;
};