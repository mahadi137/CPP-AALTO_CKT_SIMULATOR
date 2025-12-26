#pragma once

#include <Eigen/Dense>
#include <string>
#include <vector>

// Abstract base class for any circuit component.
class Component {
 public:
  // Constructor takes a unique name and the list of nodes it connects to.
  Component(const std::string& name, const std::vector<int>& nodes,
            double value = 0.0)
      : _name(name), _nodes(nodes), _value(value) {}

  // Virtual destructor is crucial for base classes with virtual methods.
  virtual ~Component() = default;

  // Pure virtual function for DC analysis (real matrices).
  virtual void stampDC(Eigen::MatrixXd& A, Eigen::VectorXd& z) const = 0;

  // Pure virtual function for AC analysis (complex matrices).
  virtual void stampAC(Eigen::MatrixXcd& A, Eigen::VectorXcd& z,
                       double omega) const = 0;

  // Getters
  const std::string& getName() const { return _name; }
  const std::vector<int>& getNodes() const { return _nodes; }
  size_t getNodeCount() const { return _nodes.size(); }
  double getValue() const { return _value; }
  void setValue(double value) { _value = value; }
  void setName(const std::string& name) { _name = name; }

  // Setter for updating node connections
  void setNodes(const std::vector<int>& nodes) { _nodes = nodes; }

 protected:
  std::string _name;
  std::vector<int> _nodes;  // Node IDs this component is connected to
  double _value;            // Generic value (resistance, capacitance, etc.)
};