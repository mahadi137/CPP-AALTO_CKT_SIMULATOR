#include "resistor.hpp"

#include <sstream>

Resistor::Resistor(std::string resistor_name, int a, int b, double resistor_val)
    : Component(a, b), R_val_(resistor_val), R_name_(resistor_name) {}

std::shared_ptr<Resistor> Resistor::Create(const std::string& name, int a,
                                           int b, double resistor_val) {
  std::string final_name =
      name.empty() ? "R" + std::to_string(++counter) : name;
  return std::make_shared<Resistor>(final_name, a, b, resistor_val);
}

void Resistor::StampDC(int, int, MNA_matrix<double>& A, Eigen::VectorXd&,
                       int&) const {
  // get MNA matrix index
  const int ia = nodeToIndex(node_a), ib = nodeToIndex(node_b);

  // value formula for DC analysis
  const double value = 1.0 / R_val_;

  // add to matrix for DC analysis
  if (ia >= 0) {
    A.Add(ia, ia, +value);
  }

  if (ib >= 0) {
    A.Add(ib, ib, +value);
  }

  if (ia >= 0 && ib >= 0) {
    A.Add(ia, ib, -value);
    A.Add(ib, ia, -value);
  }
}

std::string Resistor::GetNetlist() const {
  std::string n1 = std::to_string(node_a);
  std::string n2 = std::to_string(node_b);
  std::string val = std::to_string(R_val_);

  std::ostringstream os;
  os << R_name_ << " " << n1 << " " << n2 << " " << val << std::endl;
  return os.str();
}

double Resistor::GetValue() const { return R_val_; }

const std::string& Resistor::GetName() const { return R_name_; }

// define the static counter
int Resistor::counter = 0;
