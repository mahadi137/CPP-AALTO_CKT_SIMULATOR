#pragma once

#include <complex>
#include <string>
#include <vector>

#include "Circuit.hpp"
#include "imgui.h"
#include "implot.h"

/**
 * TimeDomainGraph - Encapsulates time-domain and frequency-domain graph
 * rendering
 *
 * Responsible for:
 * - Time-domain waveform plotting (AC analysis)
 * - Graph UI controls (node selection, time window, samples)
 * - Phase and amplitude calculations from AC solution
 */
class TimeDomainGraph {
 public:
  /**
   * Constructor
   * @param acSolution AC frequency domain solution (complex phasors)
   * @param circuit Reference to the circuit for component information
   * @param num_nodes Total number of nodes in the circuit
   */
  TimeDomainGraph()
      : acSolution_(nullptr),
        dcSolution_(nullptr),
        circuit_(nullptr),
        num_nodes_(0) {};

  /**
   * Renders the DC time-domain graph visualization
   * Includes controls for node selection
   * Creates ImPlot graph showing voltage vs time
   */
  void renderNodeVoltageGraphDC();

  /**
   * Renders the DC time-domain graph visualization
   * Includes controls for voltage source current selection
   * Creates ImPlot graph showing current vs time
   */
  void renderVoltageSourceCurrentGraphDC();

  /**
   * Renders the AC time-domain graph visualization
   * Includes controls for node selection
   * Creates ImPlot graph showing voltage vs time
   */
  void renderNodeVoltageGraphAC();

  /**
   * Renders the AC time-domain graph visualization
   * Includes controls for voltage source current selection
   * Creates ImPlot graph showing current vs time
   */
  void renderVoltageSourceCurrentGraphAC();

  /**
   * Set the DC solution to render
   * Should be called after each AC analysis run
   */
  void setDCsolution(const Eigen::VectorXd& dcSolution);

  /**
   * Set the AC solution to render
   * Should be called after each AC analysis run
   */
  void setACsolution(const Eigen::VectorXcd& acSolution);

  /**
   * Set circuit reference
   */
  void setCircuit(const Circuit& circuit);

  /**
   * Set number of nodes
   */
  void setNumNodes(int num_nodes);

 private:
  const Eigen::VectorXcd* acSolution_;
  const Eigen::VectorXd* dcSolution_;
  const Circuit* circuit_;
  int num_nodes_;

  // UI state variables
  int nodeToPlot_ = 1;
  int currentToPlot_ = 0;
  float freqHz_ = 50.0f;    // frequency in Hz
  double t0_ = 0.0f;        // start time (s)
  int periods_ = 2;         // number of periods to show
  double duration_ = 0.1f;  // duration of the time window (s)
  int samples_ = 400;       // number of samples

  /**
   * Generates time-domain samples from phasor representation
   * @param Vnode Complex voltage phasor
   * @param omega Angular frequency (rad/s)
   * @param t0 Start time
   * @param duration Total time duration
   * @param samples Number of samples to generate
   * @return Pair of vectors: (time_values, voltage_values)
   */
  std::pair<std::vector<double>, std::vector<double>> generateTimeDomainSamples(
      const std::complex<double>& Vnode, double omega, double t0,
      double duration, int samples);
};
