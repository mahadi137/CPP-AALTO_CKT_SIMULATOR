#pragma once
#include <complex>
#include <stdexcept>

#include "Component.hpp"

class VoltageSource : public Component {
 public:
  // Connects from node n_plus to n_minus.
  VoltageSource(const std::string& name, double voltage, int n_plus,
                int n_minus)
      : Component(name, {n_plus, n_minus}, voltage) {}

  void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const override;
  void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
               double omega) const override;

  // The solver will assign an index for the new current variable this
  // source introduces. This method allows the solver to inform the component
  // of its index before stamping.
  void setCurrentIndex(int index) { _current_idx = index; }
  int getCurrentIndex() const { return _current_idx; }

 private:
  int _current_idx = -1;  // Index for this source's current in the 'x' vector
};