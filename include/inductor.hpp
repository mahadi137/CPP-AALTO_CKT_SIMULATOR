#ifndef INDUCTOR_HPP
#define INDUCTOR_HPP

#include <string>

#include "component.hpp"

/**
 * @file Inductor.hpp
 * @brief Inductor component for MNA
 *
 *  Conductance G = 2.pi.f.L is stamped between its two nodes.
 */

class Inductor : public Component {
 public:
  /**
   * @brief Inductor constructor.
   *
   * @param inductor_name
   * @param a
   * @param b
   * @param inductor_val
   */
  Inductor(std::string inductor_name, int a, int b, double inductor_val);

  /**
   * @brief Create a Inductor calling with explicit unique name or without name
   * used shared to construct and destruct newly created component smoothly.
   *
   * Example:
   *     auto R1 = Inductor::Create(1, 0, 5.0);
   *     auto R1 = Inductor::Create(L1, 1, 0, 5.0);
   *
   * @param a
   * @param b
   * @param inductor_val
   * @return std::shared_ptr<Inductor> Shared pointer to the new Inductor
   */
  static std::shared_ptr<Inductor> Create(const std::string& name, int a, int b,
                                          double inductor_val);

  const std::string GetComponentKind() const override { return "Inductor"; }

  /**
   * @brief add component to mna matrix A
   *
   * picked a very large number to approximate infinity
   * Conductance = 1e12
   *
   * This large enough that the inductor’s voltage drop (computed after the
   * solve) will be effectively 0.
   *
   */
  void StampDC(int, int, MNA_matrix<double>& A, Eigen::VectorXd&,
               int&) const override;

  std::string GetNetlist() const override;

  double GetValue() const override;
  const std::string& GetName() const override;

  /**
   * @brief Static counter for auto-unique-naming L1, L2, L3...
   *
   */
  static int counter;

 private:
  double L_val_ = 0.0;
  const std::string L_name_;
};

#endif  // Inductor_HPP
