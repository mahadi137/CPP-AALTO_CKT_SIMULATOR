#include "inductor.hpp"

#include <sstream>

Inductor::Inductor(std::string inductor_name, int a, int b, double inductor_val)
    : Component(a, b), L_val_(inductor_val), L_name_(inductor_name) {}

std::shared_ptr<Inductor> Inductor::Create(const std::string& name, int a,
                                           int b, double inductor_val) {
  std::string final_name =
      name.empty() ? "L" + std::to_string(++counter) : name;
  return std::make_shared<Inductor>(final_name, a, b, inductor_val);
}

void Inductor::StampDC(int, int, MNA_matrix<double>& A, Eigen::VectorXd&,
                       int&) const {
  // get MNA matrix index
  const int ia = nodeToIndex(node_a), ib = nodeToIndex(node_b);

  // large value for DC analysis to represent short
  const double value = 1e12;

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

std::string Inductor::GetNetlist() const {
  std::string n1 = std::to_string(node_a);
  std::string n2 = std::to_string(node_b);
  std::string val = std::to_string(L_val_);

  std::ostringstream os;
  os << L_name_ << " " << n1 << " " << n2 << " " << val << std::endl;
  return os.str();
}

double Inductor::GetValue() const { return L_val_; }

const std::string& Inductor::GetName() const { return L_name_; }

// define the static counter
int Inductor::counter = 0;
