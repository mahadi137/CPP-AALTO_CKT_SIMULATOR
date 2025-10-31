#ifndef VOLTAGESOURCE_HPP
#define VOLTAGESOURCE_HPP

/**
 * @file VoltageSource.hpp
 * @brief VoltageSource component for MNA
 *
 *  Voltage source with time function Vt(time).
 */

#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include "component.hpp"

class VoltageSource : public Component {
 public:
  /**
   * @brief Time function Vt(time). The voltage source construction expect
   * a function to return voltage value. Where aim is to use lambda function
   * with time-dependency, so that time as argument can be pass to play
   * around with AC sign wave.
   *
   * Expected simulation should have time-dependent voltage sources.
   * To do that, every voltage source must store a function
   * that can return the voltage for given time as argument.
   *
   * lambda:
   * double Vt(double time) {
   * // ignores time for DC or use time for AC
   * return V;
   * }
   *
   * For DC: time ignored and return DC constant voltage
   * For AC: time use to calculate time-dependent waveform
   * Ex:
   * Vt(time) = 5.0                        // DC
   * Vt(time) = 10.0 * sin(2π * 60 * time)   // sine wave
   *
   * every voltage source object has its own vt — a function
   * that can be called like:
   * vt(t)  // returns voltage at time t
   */
  // actual: std::function<double(double time)> Vt;
  std::function<double(double)> Vt;

  /**
   * @brief Voltage source constructor (NO default args here).
   * Not calling this constructor directly anyway — calling VDC() and VAC().
   * Those already fill with all parameters.
   *
   * @brief VoltageSource constructor with explicit unique name.
   *
   *
   * @param a
   * @param b
   * @param volt_nominal
   * @param fn
   * @param voltagesource_name
   */
  VoltageSource(int a, int b, double volt_nominal,
                std::function<double(double)> fn,
                const std::string& voltagesource_name);

  const std::string GetComponentKind() const override {
    return "CurrentSource";
  }

  void StampDC(int, int vsBase, MNA_matrix<double>& A, Eigen::VectorXd& Z,
               int& nextVS) const override;

  std::string GetNetlist() const override;
  double GetValue() const override;
  const std::string& GetName() const override;

  /**
   * @brief Static counter for auto-unique-naming VAC1, VAC2, VAC3...
   *
   */
  static int counter_ac;
  static int counter_dc;

  /**
   * @brief Create a DC (constant) voltage source.
   *
   * This is a convenience factory that builds a VoltageSource whose
   * voltage does not depend on time (i.e. pure DC).
   *
   * used shared to construct and destruct newly created DC source smoothly.
   *
   * Example:
   *     auto V1 = VoltageSource::DC(1, 0, 5.0) // 5V source from node1 to
   * ground
   *     auto V1 = VoltageSource::DC(VDC1, 1, 0, 5.0)
   *
   * Because of static above way is possible (class level function).
   * No need to create a an instance.
   *
   * @param a    node_a usually positive terminal
   * @param b    node_a usually negative terminal
   * @param Vdc  Constant voltage value in volts
   * @return std::shared_ptr<VoltageSource> Shared pointer to the new
   * VoltageSource
   *
   */
  static std::shared_ptr<VoltageSource> VDC(
      const std::string& volt_src_name = "", int a = 0, int b = 0,
      double Vdc = 0);

  /**
   * @brief Create a AC voltage source. use time in lambda function.
   *
   * \f[
   *     Vt(t) = V_0 + A.sin(2.pi.f.t + phi)
   * \f]
   *
   * Vt = time dependent voltage
   * V_0 = DC offset, ignore in this simulator
   * A = Voltage Amplitude
   * f = frequency
   * t = time
   * pi = PI
   * phi = phase angle = 0 deg = Starts at 0 V, rising positive (default)
   *
   * @param a
   * @param b
   * @param Amp
   * @param freqHz
   * @param phaseDeg
   * @return std::shared_ptr<VoltageSource> Shared pointer to the new
   * VoltageSource
   */
  static std::shared_ptr<VoltageSource> VAC(
      const std::string& volt_src_name = "", int a = 0, int b = 0,
      double Amp = 0);

 private:
  double VS_val_ = 0.0;
  const std::string VS_name_;
};

#endif  // VoltageSource_HPP
