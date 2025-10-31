#ifndef COMPONENT_HPP
#define COMPONENT_HPP

/**
 * @file Component.hpp
 * @brief base for components.
 *
 * This header defines:
 *  - TripletAcc<T> to accumulate matrix triplets
 *  - nodeToIndex() converting node id -> MNA row/col (0-based, ground=0 -> -1)
 *  - Component interface with DC
 *  - Component netlist save
 *  - Getters for component value and name
 *
 */

#include <Eigen/Sparse>
#include <string>
#include <vector>

/**
 * @brief modified nodal analysis template
 * @brief node number to MNA matrix index converter
 */
template <typename T>
struct MNA_matrix {
  // modified nodal analysis matrix container
  std::vector<Eigen::Triplet<T> > mna;

  // add to the MNA vector
  void Add(int row, int col, const T& val) {
    if (row >= 0 && col >= 0) {
      mna.emplace_back(row, col, val);
    }
  }
};

// Map circuit node number -> MNA index
// Convention: node 0 == ground, which maps to -1 (not part of matrix)
inline int nodeToIndex(int n) { return (n == 0) ? -1 : (n - 1); }

/**
 * @brief components netlist container structure
 *

struct ComponentNetlist {
  const std::string name;
  int node_a, node_b;
  double value;
  ComponentNetlist() = default;
  // Constructor to initialize constant
  ComponentNetlist(const std::string& n) : name(n) {}
} std::vector<ComponentNetlist> netlist_;
 */

// base class for all components
class Component {
 public:
  /**
   * @brief Construct a new Component object
   *
   * @param a node_a usually positive terminal
   * @param b node_b usually negative terminal
   */
  explicit Component(int a, int b);

  /**
   * @brief rule of five
   *
   */
  Component() = default;
  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;
  // also forbid moves; remove these two lines if you want moves
  Component(Component&&) = delete;
  Component& operator=(Component&&) = delete;

  /**
   * @brief Destroy the Base Component object
   *
   */
  virtual ~Component() {}

  /**
   * @brief Get the Component Kind
   *
   * @return const std::string What type is this? ("Resistor", "Capacitor",
   * etc.)
   */
  virtual const std::string GetComponentKind() const = 0;

  /**
   * @brief add component to mna matrix A
   *
   * @param total_nodes total number of nodes used
   * @param vsBase Index in the matrix where voltage-source current unknowns
   * begin. vsBase = total_nodes.
   * @param A MNA matrix A
   * @param Z MNA matrix Z
   * @param nextVS voltagesource to make Z, init = 0
   */
  virtual void StampDC(int total_nodes, int vsBase, MNA_matrix<double>& A,
                       Eigen::VectorXd& Z, int& nextVS) const = 0;

  /**
   * @brief Create netlist string for a component and save
   * and save in a container, where?
   *
   * Serialize (e.g. "R3 1 2 1000")
   */
  virtual std::string GetNetlist() const = 0;

  /**
   * @brief Other Getters
   *
   */
  virtual double GetValue() const = 0;
  virtual const std::string& GetName() const = 0;

  /**
   * @brief variables
   * to store nodes
   * to create MNA matrix
   *
   */
  int node_a, node_b = 0;
};

#endif  // COMPONENT_HPP
