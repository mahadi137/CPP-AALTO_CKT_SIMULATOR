#include "capacitor.hpp"

#include <sstream>

Capacitor::Capacitor(std::string capacitor_name, int a, int b,
                     double capacitor_val)
    : Component(a, b), C_val_(capacitor_val), C_name_(capacitor_name) {}

// define the static counter
int Capacitor::counter = 0;

std::shared_ptr<Capacitor> Capacitor::Create(const std::string& name, int a,
                                             int b, double capacitor_val) {
  std::string final_name =
      name.empty() ? "C" + std::to_string(++counter) : name;
  return std::make_shared<Capacitor>(final_name, a, b, capacitor_val);
}

void Capacitor::StampDC(int, int, MNA_matrix<double>&, Eigen::VectorXd&,
                        int&) const {
  // Open, c = 0 at DC: no conductance.
  // does nothing
}

std::string Capacitor::GetNetlist() const {
  std::string n1 = std::to_string(node_a);
  std::string n2 = std::to_string(node_b);
  std::string val = std::to_string(C_val_);

  std::ostringstream os;
  os << C_name_ << " " << n1 << " " << n2 << " " << val << std::endl;
  return os.str();
}

double Capacitor::GetValue() const { return C_val_; }

const std::string& Capacitor::GetName() const { return C_name_; }
