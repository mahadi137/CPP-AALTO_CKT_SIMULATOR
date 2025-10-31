#ifndef SIM_NOCOMPLEX_CIRCUIT_HPP
#define SIM_NOCOMPLEX_CIRCUIT_HPP

/**
 * @file Circuit.hpp
 * @brief Circuit container, DC and transient solvers (MNA).
 *
 * Responsibilities:
 *  - store components and node count
 *  - assemble sparse MNA systems
 *  - solve DC (real)
 *  - simulate transient via Backward Euler
 */

#include <memory>
#include <vector>

#include "component.hpp"

class Circuit {
 public:
  /**
   * @brief
   *
   * @param c container of shared_ptr
   */
  void Add(const std::shared_ptr<Component>& c);
  /**
   * @brief Clear container before storing components
   *
   */
  void Clear();
  /**
   * @brief Start DC analysis
   *
   * @return Eigen::VectorXd matrix x
   */
  Eigen::VectorXd AnalysisDC();
  /**
   * @brief Circuit components netlist generator.
   * This can be useful for save circuit method.
   *
   */
  void SaveNetlist() const;
  /**
   * @brief Calculate each RLC component current
   *
   */
  void GetRLCCurrent() const;

 private:
  // int maxNode_ = 4;
  std::vector<std::shared_ptr<Component> > comps_;
  // Results of unknown nodes and source current storing
  // Must be according to matrix x order.
  // Node voltage first then source current
  std::vector<double> mna_matrix_x_;
};

#endif  // SIM_NOCOMPLEX_CIRCUIT_HPP
