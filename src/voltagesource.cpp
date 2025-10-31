#include "voltagesource.hpp"

// define the static counter
int VoltageSource::counter_dc = 0;
int VoltageSource::counter_ac = 0;

VoltageSource::VoltageSource(int a, int b, double volt_nominal,
                             std::function<double(double)> fn,
                             const std::string& voltagesource_name)
    : Component(a, b),
      Vt(fn),
      VS_val_(volt_nominal),
      VS_name_(voltagesource_name) {}

std::shared_ptr<VoltageSource> VoltageSource::VDC(
    const std::string& volt_src_name, int a, int b, double Vdc) {
  std::string final_name = volt_src_name.empty()
                               ? "VDC" + std::to_string(++counter_dc)
                               : volt_src_name;

  return std::make_shared<VoltageSource>(
      // actual lambda: [Vdc](double /*time*/) { return Vdc; }
      a, b, Vdc, [Vdc](double) { return Vdc; }, final_name);
}

std::shared_ptr<VoltageSource> VoltageSource::VAC(
    const std::string& volt_src_name, int a, int b, double Vrms) {
  std::string final_name = volt_src_name.empty()
                               ? "VAC" + std::to_string(++counter_ac)
                               : volt_src_name;
  // Get peak to get internally the waveform 325.27sin(2π50t)
  const double Vpeak = Vrms * std::sqrt(2.0);
  // M_PI = 3.141592653589793 in cmath header
  const double w = 2.0 * M_PI * 50.0;
  const double phaseDeg = 0.0;

  return std::make_shared<VoltageSource>(
      a, b, Vrms,
      [Vpeak, w, phaseDeg](double time) {
        return Vpeak * std::sin(w * time + phaseDeg);
      },
      final_name);  // internally the waveform is 325.27sin(2π50t)
}

void VoltageSource::StampDC(int, int vsBase, MNA_matrix<double>& A,
                            Eigen::VectorXd& Z, int& nextVS) const {
  const int ia = nodeToIndex(node_a), ib = nodeToIndex(node_b);
  const int k = nextVS++;
  const int ic = vsBase + k;  // row/col index in A for this source voltage

  if (ia >= 0) {
    A.Add(ia, ic, +1.0);
    A.Add(ic, ia, +1.0);
  }
  if (ib >= 0) {
    A.Add(ib, ic, -1.0);
    A.Add(ic, ib, -1.0);
  }

  // known elements
  Z(ic) += VS_val_;
}

std::string VoltageSource::GetNetlist() const {
  std::string n1 = std::to_string(node_a);
  std::string n2 = std::to_string(node_b);
  std::string val = std::to_string(VS_val_);

  std::ostringstream os;
  os << VS_name_ << " " << n1 << " " << n2 << " " << val << std::endl;
  return os.str();
}

double VoltageSource::GetValue() const { return VS_val_; }

const std::string& VoltageSource::GetName() const { return VS_name_; }
