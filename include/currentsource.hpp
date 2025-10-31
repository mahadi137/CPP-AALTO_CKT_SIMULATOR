#ifndef CURRENTSOURCE_HPP
#define CURRENTSOURCE_HPP

/**
 * @file CurrentSource.hpp
 * @brief CurrentSource component for MNA
 *
 *  Current source with time function It(time).
 */

#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include "component.hpp"

class CurrentSource : public Component {
 public:
  /**
   * @brief Time function It(time). The Current source construction expect
   * a function to return Current value. Where aim is to use lambda function
   * with time-dependency, so that time as argument can be pass to play
   * around with AC sign wave.
   *
   * Expected simulation should have time-dependent Current sources.
   * To do that, every Current source must store a function
   * that can return the Current for given time as argument.
   *
   * lambda:
   * double It(double time) {
   * // ignores time for DC or use time for AC
   * return I;
   * }
   *
   * For DC: time ignored and return DC constant Current
   *
   * every Current source object has its own It — a function
   * that can be called like:
   * It(t)  // returns Current at time t
   */
  // actual: std::function<double(double time)> It;
  std::function<double(double)> It;

  /**
   * @brief Voltage source constructor (NO default args here).
   * Not calling this constructor directly anyway — calling VDC() and VAC().
   * Those already fill with all parameters.
   *
   * @brief VoltageSource constructor with explicit unique name.
   *
   * @param a
   * @param b
   * @param volt_nominal
   * @param fn
   * @param currentsource_name
   */
  CurrentSource(int a, int b, double volt_nominal,
                std::function<double(double)> fn,
                const std::string& currentsource_name);

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
  static int counter_dc;

  /**
   * @brief Create a (constant) Current source.
   *
   * This is a convenience factory that builds a CurrentSource whose
   * Current does not depend on time (i.e. pure DC).
   *
   * used shared to construct and destruct newly created DC source smoothly.
   *
   * Example:
   *     auto ISRC1 = CurrentSource::DC("", 1, 0, 5.0)
   *     auto ISRC1 = CurrentSource::DC(ISRC1, 1, 0, 5.0)
   *
   * Because of static above way is possible (class level function).
   * No need to create an instance.
   *
   * @param a    node_a usually positive terminal
   * @param b    node_a usually negative terminal
   * @param ISRC  Constant Current value in volts
   * @return std::shared_ptr<CurrentSource> Shared pointer to the new
   * CurrentSource
   *
   */
  static std::shared_ptr<CurrentSource> ISRC(
      const std::string& curr_src_name = "", int a = 0, int b = 0,
      double Isrc = 0);

 private:
  double IS_val_ = 0.0;
  const std::string IS_name_;
};

#endif  // CurrentSource_HPP
