// ACVoltageSource.hpp
#pragma once
#include "Component.hpp"

class ACVoltageSource : public Component {
 public:
  ACVoltageSource(const std::string& name, double acPeak, int n_plus,
                  int n_minus)
      : Component(name, {n_plus, n_minus}, acPeak) {}

  // DC & AC stamps:
  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double /*omega*/) const override;

  void setCurrentIndex(int idx) { _current_idx = idx; }
  int getCurrentIndex() const { return _current_idx; }

  // AC phasor
  // std::polar(_acAmplitude, _phase) → phasor (complex) voltage used in AC
  // steady-state analysis at 50 Hz fixed frequency.
  std::complex<double> Vac = std::polar(getValue(), _phase);

 private:
  double _phase = 0.0;
  double _dcOffset = 0.0;
  int _current_idx = -1;
};