#include "timedomainGraph.hpp"

#include <cmath>

void TimeDomainGraph::setACsolution(const Eigen::VectorXcd& acSolution) {
  acSolution_ = &acSolution;
}

void TimeDomainGraph::setDCsolution(const Eigen::VectorXd& dcSolution) {
  dcSolution_ = &dcSolution;
}

void TimeDomainGraph::setCircuit(const Circuit& circuit) {
  circuit_ = &circuit;
}

// void TimeDomainGraph::setNumNodes(int num_nodes) { num_nodes_ = num_nodes; }
void TimeDomainGraph::setNumNodes(int num_nodes) {
  num_nodes_ = num_nodes;
  if (nodeToPlot_ > num_nodes_) {
    nodeToPlot_ = num_nodes_;
  }
  if (nodeToPlot_ < 1 && num_nodes_ > 0) {
    nodeToPlot_ = 1;
  }
}

std::pair<std::vector<double>, std::vector<double>>
TimeDomainGraph::generateTimeDomainSamples(const std::complex<double>& Vphasor,
                                           double omega, double t0,
                                           double duration, int samples) {
  std::vector<double> t_vals(samples);
  std::vector<double> v_vals(samples);

  double Vpeak = std::abs(Vphasor);  // peak amplitude
  double phase = std::arg(Vphasor);  // radians

  for (int i = 0; i < samples; ++i) {
    double t = t0 + duration * i / (samples - 1);
    t_vals[i] = t;
    v_vals[i] = Vpeak * std::sin(omega * t + phase);
  }

  return {std::move(t_vals), std::move(v_vals)};
}

// void TimeDomainGraph::renderACGraph() {
//   if (acSolution_->size() == 0 || !circuit_ || num_nodes_ <= 0) {
//     ImGui::Text("Graph data not available");
//     return;
//   }

//   // ===========================
//   // ImPlot time-domain waveform
//   // ===========================
//   ImGui::Text("Circuit node voltages");
//   ImGui::SliderInt("Node index", &nodeToPlot_, 1, num_nodes_);
//   ImGui::SliderInt("Periods", &periods_, 1, 50);
//   ImGui::Text("Frequency: %.2f Hz", freqHz_);

//   // for future control
//   if (freqHz_ <= 0.0f) freqHz_ = 50.0f;
//   if (periods_ <= 0) periods_ = 2;

//   int idx_node = nodeToPlot_ - 1;

//   if (idx_node < 0 || idx_node >= num_nodes_ ||
//       idx_node >= static_cast<int>(acSolution_->size())) {
//     ImGui::Text("Invalid node index or no AC solution data");
//     return;
//   }

//   const double f = static_cast<double>(freqHz_);
//   const double omega = 2.0 * M_PI * f;
//   const double T = 1.0 / f;
//   const double duration = static_cast<double>(periods_) * T;

//   // Complex phasor for selected node (assumed PEAK value)
//   std::complex<double> Vphasor = (*acSolution_)(idx_node);
//   const double Vpeak = std::abs(Vphasor);
//   const double Vrms = Vpeak / std::sqrt(2.0);

//   ImGui::Text("Node V_%d: Vpeak = %.6f V, Vrms = %.6f V, phase = %.3f rad",
//               nodeToPlot_, Vpeak, Vrms, std::arg(Vphasor));

//   // ===============================
//   // time-domain samples
//   // (this will change whenever nodeToPlot_ or periods_ changes)
//   // ===============================
//   // Generate samples
//   auto [t_vals, v_vals] =
//       generateTimeDomainSamples(Vphasor, omega, t0_, duration, samples_);

//   if (ImPlot::BeginPlot("Voltage (peak) vs Time")) {
//     ImPlot::SetupAxes("Time (s)", "Voltage (V)");

//     ImPlot::SetupAxesLimits(static_cast<double>(t0_),
//                             static_cast<double>(t0_) + duration, -1.2 *
//                             Vpeak, +1.2 * Vpeak, ImPlotCond_Always);

//     std::string label = "V_" + std::to_string(nodeToPlot_);
//     ImPlot::PlotLine(label.c_str(), t_vals.data(), v_vals.data(), samples_);

//     ImPlot::EndPlot();
//   }

//   // =====================================
//   // Unknown currents from AC solution
//   // =====================================
//   // voltage source currents
//   std::vector<std::string> current_names;
//   for (const auto& comp : circuit_->getComponents()) {
//     if (dynamic_cast<VoltageSource*>(comp.get()) ||
//         dynamic_cast<ACVoltageSource*>(comp.get())) {
//       current_names.push_back(comp->getName());
//     }
//   }

//   if (current_names.size() <= 0) {
//     ImGui::Text("Graph data not available");
//     return;
//   }

//   ImGui::Separator();
//   ImGui::Text("Voltage source currents");

//   // Choose which unknown current to plot
//   int current_idx_offset = num_nodes_;
//   currentToPlot_ = current_names.size() - 1;
//   int currentplot_idx = currentToPlot_ + 1;

//   ImGui::SliderInt("Current index", &currentplot_idx, 1,
//                    static_cast<int>(current_names.size()));

//   // Index in acSolution_
//   int idx_current = (current_idx_offset + currentToPlot_);

//   // if (idx_current < 0 || idx_current >= total_unknowns) {
//   //   ImGui::Text("Invalid current index");
//   //   return;
//   // }

//   std::complex<double> Iphasor = (*acSolution_)(idx_current);

//   double Ipeak = std::abs(Iphasor);
//   double Irms = Ipeak / std::sqrt(2.0);

//   ImGui::Text(
//       "I_%s: Ipeak = %.6g A, Irms = %.6g A, phase = %.3f "
//       "rad",
//       current_names[currentToPlot_].c_str(), Ipeak, Irms, std::arg(Iphasor));

//   auto [t_vals_I, i_vals] =
//       generateTimeDomainSamples(Iphasor, omega, t0_, duration, samples_);

//   if (ImPlot::BeginPlot("Volt. Src. Current (peak) vs Time")) {
//     ImPlot::SetupAxes("Time (s)", "Current (A)");
//     ImPlot::SetupAxesLimits(t0_, t0_ + duration, -1.2 * Ipeak, +1.2 * Ipeak,
//                             ImPlotCond_Always);

//     std::string label_I = "I_" + current_names[currentToPlot_];
//     ImPlot::PlotLine(label_I.c_str(), t_vals_I.data(), i_vals.data(),
//     samples_); ImPlot::EndPlot();
//   }
// }

// ===========================
// DC graph
// ===========================
void TimeDomainGraph::renderNodeVoltageGraphDC() {
  if (!dcSolution_ || dcSolution_->size() == 0 || !circuit_ ||
      num_nodes_ <= 0) {
    ImGui::Text("Graph data not available");
    return;
  }

  ImGui::Text("Circuit node voltages");
  ImGui::SliderInt("Node index", &nodeToPlot_, 1, num_nodes_);

  const int idx_node = nodeToPlot_ - 1;

  if (idx_node < 0 || idx_node >= num_nodes_ ||
      idx_node >= static_cast<int>(dcSolution_->size())) {
    ImGui::Text("Invalid node index or no DC solution data");
    return;
  }

  const double Vdc = (*dcSolution_)(idx_node);

  ImGui::Text("Node V_%d: Vdc = %.6f V", nodeToPlot_, Vdc);

  // ===============================
  // Generate constant time-domain samples for DC
  // ===============================
  std::vector<double> t_vals(samples_);
  std::vector<double> v_vals(samples_);

  const double dt = duration_ / static_cast<double>(samples_ - 1);

  for (int i = 0; i < samples_; ++i) {
    t_vals[i] = t0_ + i * dt;
    v_vals[i] = Vdc;  // constant DC value
  }

  // ===============================
  // Plot DC voltage vs time
  // ===============================
  if (ImPlot::BeginPlot("Voltage vs Time")) {
    ImPlot::SetupAxes("Time (s)", "Voltage (V)");

    const double margin = std::max(std::abs(Vdc) * 0.2, 0.1);
    ImPlot::SetupAxesLimits(static_cast<double>(t0_),
                            static_cast<double>(t0_) + duration_, Vdc - margin,
                            Vdc + margin, ImPlotCond_Always);

    std::string label = "V_" + std::to_string(nodeToPlot_);
    ImPlot::PlotLine(label.c_str(), t_vals.data(), v_vals.data(), samples_);

    ImPlot::EndPlot();
  }
}

void TimeDomainGraph::renderVoltageSourceCurrentGraphDC() {
  // Basic checks
  if (!dcSolution_ || dcSolution_->size() == 0 || !circuit_ ||
      num_nodes_ <= 0) {
    ImGui::Text("Graph data not available");
    return;
  }

  // =====================================
  // Unknown currents from DC solution
  // =====================================
  // voltage source currents
  std::vector<std::string> current_names;
  for (const auto& comp : circuit_->getComponents()) {
    if (dynamic_cast<VoltageSource*>(comp.get()) ||
        dynamic_cast<ACVoltageSource*>(comp.get())) {
      current_names.push_back(comp->getName());
    }
  }

  if (current_names.empty()) {
    ImGui::Text("Voltage source current graph data not available");
    return;
  }

  ImGui::Text("Voltage source currents");

  // Choose which unknown current to plot
  int current_idx_offset = num_nodes_;
  currentToPlot_ = current_names.size() - 1;
  int currentplot_idx = currentToPlot_ + 1;

  ImGui::SliderInt("Current index", &currentplot_idx, 1,
                   static_cast<int>(current_names.size()));

  int idx_current = (current_idx_offset + currentToPlot_);

  if (idx_current < 0 || idx_current >= static_cast<int>(dcSolution_->size())) {
    ImGui::Text("DC solution does not contain this current index");
    return;
  }

  const double Idc = (*dcSolution_)(idx_current);

  ImGui::Text("I_%s: Idc = %.6g A", current_names[currentToPlot_].c_str(), Idc);

  // ===============================
  // Time window and samples for plot
  // ===============================
  const double dt = duration_ / static_cast<double>(samples_ - 1);

  std::vector<double> t_vals(samples_);
  std::vector<double> i_vals(samples_);

  for (int i = 0; i < samples_; ++i) {
    t_vals[i] = t0_ + i * dt;
    i_vals[i] = Idc;  // constant DC current
  }

  // ===============================
  // Plot DC current vs time
  // ===============================
  if (ImPlot::BeginPlot("Volt. Src. Current vs Time")) {
    ImPlot::SetupAxes("Time (s)", "Current (A)");

    // Y-limits with a little margin
    const double margin = std::max(std::abs(Idc) * 0.2, 0.1);
    ImPlot::SetupAxesLimits(t0_, t0_ + duration_, Idc - margin, Idc + margin,
                            ImPlotCond_Always);

    std::string label_I = "I_" + current_names[currentToPlot_];
    ImPlot::PlotLine(label_I.c_str(), t_vals.data(), i_vals.data(), samples_);

    ImPlot::EndPlot();
  }
}

// ===========================
// AC graph
// ===========================
void TimeDomainGraph::renderNodeVoltageGraphAC() {
  if (acSolution_->size() == 0 || !circuit_ || num_nodes_ <= 0) {
    ImGui::Text("Graph data not available");
    return;
  }

  // ===========================
  // ImPlot time-domain waveform
  // ===========================
  ImGui::Text("Circuit node voltages");
  ImGui::SliderInt("Node index", &nodeToPlot_, 1, num_nodes_);
  ImGui::SliderInt("Periods", &periods_, 1, 50);
  ImGui::Text("Frequency: %.2f Hz", freqHz_);

  // for future control
  if (freqHz_ <= 0.0f) freqHz_ = 50.0f;
  if (periods_ <= 0) periods_ = 2;

  int idx_node = nodeToPlot_ - 1;

  if (idx_node < 0 || idx_node >= num_nodes_ ||
      idx_node >= static_cast<int>(acSolution_->size())) {
    ImGui::Text("Invalid node index or no AC solution data");
    return;
  }

  const double omega = 2.0 * M_PI * freqHz_;
  const double T = 1.0 / freqHz_;
  const double duration = static_cast<double>(periods_) * T;

  // Complex phasor for selected node (assumed PEAK value)
  std::complex<double> Vphasor = (*acSolution_)(idx_node);
  const double Vpeak = std::abs(Vphasor);
  const double Vrms = Vpeak / std::sqrt(2.0);

  ImGui::Text("Node V_%d: Vpeak = %.6f V, Vrms = %.6f V, phase = %.3f rad",
              nodeToPlot_, Vpeak, Vrms, std::arg(Vphasor));

  // ===============================
  // time-domain samples
  // (this will change whenever nodeToPlot_ or periods_ changes)
  // ===============================
  // Generate samples
  auto [t_vals, v_vals] =
      generateTimeDomainSamples(Vphasor, omega, t0_, duration, samples_);

  if (ImPlot::BeginPlot("Voltage (peak) vs Time")) {
    ImPlot::SetupAxes("Time (s)", "Voltage (V)");

    ImPlot::SetupAxesLimits(static_cast<double>(t0_),
                            static_cast<double>(t0_) + duration, -1.2 * Vpeak,
                            +1.2 * Vpeak, ImPlotCond_Always);

    std::string label = "V_" + std::to_string(nodeToPlot_);
    ImPlot::PlotLine(label.c_str(), t_vals.data(), v_vals.data(), samples_);

    ImPlot::EndPlot();
  }
}

void TimeDomainGraph::renderVoltageSourceCurrentGraphAC() {
  if (acSolution_->size() == 0 || !circuit_ || num_nodes_ <= 0) {
    ImGui::Text("Graph data not available");
    return;
  }

  // =====================================
  // Unknown currents from AC solution
  // =====================================
  // voltage source currents
  std::vector<std::string> current_names;
  for (const auto& comp : circuit_->getComponents()) {
    if (dynamic_cast<VoltageSource*>(comp.get()) ||
        dynamic_cast<ACVoltageSource*>(comp.get())) {
      current_names.push_back(comp->getName());
    }
  }

  if (current_names.size() <= 0) {
    ImGui::Text("Voltage source current graph data not available");
    return;
  }

  ImGui::Text("Voltage source currents");

  // Choose which unknown current to plot
  int current_idx_offset = num_nodes_;
  currentToPlot_ = current_names.size() - 1;
  int currentplot_idx = currentToPlot_ + 1;

  ImGui::SliderInt("Current index", &currentplot_idx, 1,
                   static_cast<int>(current_names.size()));

  ImGui::Text("Frequency: %.2f Hz", freqHz_);

  // Index in acSolution_
  int idx_current = (current_idx_offset + currentToPlot_);

  const double omega = 2.0 * M_PI * freqHz_;
  const double T = 1.0 / freqHz_;
  const double duration = static_cast<double>(periods_) * T;

  std::complex<double> Iphasor = (*acSolution_)(idx_current);

  double Ipeak = std::abs(Iphasor);
  double Irms = Ipeak / std::sqrt(2.0);

  ImGui::Text(
      "I_%s: Ipeak = %.6g A, Irms = %.6g A, phase = %.3f "
      "rad",
      current_names[currentToPlot_].c_str(), Ipeak, Irms, std::arg(Iphasor));

  auto [t_vals_I, i_vals] =
      generateTimeDomainSamples(Iphasor, omega, t0_, duration, samples_);

  if (ImPlot::BeginPlot("Volt. Src. Current (peak) vs Time")) {
    ImPlot::SetupAxes("Time (s)", "Current (A)");
    ImPlot::SetupAxesLimits(t0_, t0_ + duration, -1.2 * Ipeak, +1.2 * Ipeak,
                            ImPlotCond_Always);

    std::string label_I = "I_" + current_names[currentToPlot_];
    ImPlot::PlotLine(label_I.c_str(), t_vals_I.data(), i_vals.data(), samples_);
    ImPlot::EndPlot();
  }
}
