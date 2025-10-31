#ifndef RESISTOR_HPP
#define RESISTOR_HPP

/**
 * @file Resistor.hpp
 * @brief Resistor component for MNA
 *
 *  Conductance G = 1/R is stamped between its two nodes.
 */

#include <string>

#include "component.hpp"

class Resistor : public Component {
 public:
  /**
   * @brief Resistor constructor.
   * Aim: to use with netlist load() function.
   *
   * @param resistor_name
   * @param a
   * @param b
   * @param resistor_val
   */
  Resistor(std::string resistor_name, int a, int b, double resistor_val);

  /**
   * @brief Create a resistor calling with explicit unique name or without name
   * used shared to construct and destruct newly created component smoothly.
   *
   * Example:
   *     auto R1 = Resistor::Create(1, 0, 5.0);
   *     auto R1 = Resistor::Create(R1, 1, 0, 5.0);
   *
   * @param a
   * @param b
   * @param resistor_val
   * @return std::shared_ptr<Resistor> Shared pointer to the new resistor
   */
  static std::shared_ptr<Resistor> Create(const std::string& name, int a, int b,
                                          double resistor_val);

  const std::string GetComponentKind() const override { return "Resistor"; }

  void StampDC(int, int, MNA_matrix<double>& A, Eigen::VectorXd&,
               int&) const override;

  std::string GetNetlist() const override;
  double GetValue() const override;
  const std::string& GetName() const override;

  /**
   * @brief Static counter for auto-unique-naming R1, R2, R3...
   *
   */
  static int counter;

 private:
  double R_val_ = 0.0;
  const std::string R_name_;
};

#endif  // RESISTOR_HPP
