#ifndef CAPACITOR_HPP
#define CAPACITOR_HPP

#include <string>

#include "component.hpp"

/**
 * @file Capacitor.hpp
 * @brief Capacitor component for MNA
 *
 *  Conductance G = 1/2.pi.f.C is stamped between its two nodes.
 */

class Capacitor : public Component {
 public:
  /**
   * @brief Capacitor constructor.
   *
   * @param capacitor_name
   * @param a
   * @param b
   * @param capacitor_val
   */
  Capacitor(std::string capacitor_name, int a, int b, double capacitor_val);

  /**
   * @brief Create a Capacitor calling with explicit unique name or without name
   * used shared to construct and destruct newly created component smoothly.
   *
   * Example:
   *     auto R1 = Capacitor::Create(1, 0, 5.0);
   *     auto R1 = Capacitor::Create(C1, 1, 0, 5.0);
   *
   * @param a
   * @param b
   * @param capacitor_val
   * @return std::shared_ptr<Capacitor> Shared pointer to the new Capacitor
   */
  static std::shared_ptr<Capacitor> Create(const std::string& name, int a,
                                           int b, double capacitor_val);

  const std::string GetComponentKind() const override { return "Capacitor"; }

  void StampDC(int, int, MNA_matrix<double>&, Eigen::VectorXd&,
               int&) const override;

  std::string GetNetlist() const override;

  double GetValue() const override;
  const std::string& GetName() const override;

  /**
   * @brief Static counter for auto-unique-naming C1, C2, C3...
   *
   */
  static int counter;

 private:
  double C_val_ = 0.0;
  const std::string C_name_;
};

#endif  // Capacitor_HPP
