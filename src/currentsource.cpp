#include "currentsource.hpp"

// define the static counter
int CurrentSource::counter_dc = 0;

CurrentSource::CurrentSource(int a, int b, double curr_nominal,
                             std::function<double(double)> fn,
                             const std::string& CurrentSource_name)
    : Component(a, b),
      It(fn),
      IS_val_(curr_nominal),
      IS_name_(CurrentSource_name) {}

std::shared_ptr<CurrentSource> CurrentSource::ISRC(
    const std::string& curr_src_name, int a, int b, double Isrc) {
  std::string final_name = curr_src_name.empty()
                               ? "ISRC" + std::to_string(++counter_dc)
                               : curr_src_name;

  return std::make_shared<CurrentSource>(
      // actual lambda: [Vdc](double /*time*/) { return Vdc; }
      a, b, Isrc, [Isrc](double) { return Isrc; }, final_name);
}

void CurrentSource::StampDC(int, int, MNA_matrix<double>&, Eigen::VectorXd& Z,
                            int&) const {
  const int ia = nodeToIndex(node_a), ib = nodeToIndex(node_b);

  // Positive IS_val_ means current from a -> b.
  // IS_val_ is known elements, this will fill node voltage row/col
  if (ia >= 0) Z(ia) -= IS_val_;
  if (ib >= 0) Z(ib) += IS_val_;
}

std::string CurrentSource::GetNetlist() const {
  std::string n1 = std::to_string(node_a);
  std::string n2 = std::to_string(node_b);
  std::string val = std::to_string(IS_val_);

  std::ostringstream os;
  os << IS_name_ << " " << n1 << " " << n2 << " " << val << std::endl;
  return os.str();
}

double CurrentSource::GetValue() const { return IS_val_; }

const std::string& CurrentSource::GetName() const { return IS_name_; }
