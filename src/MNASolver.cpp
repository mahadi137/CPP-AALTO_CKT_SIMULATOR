#include "MNASolver.hpp"

#include <iostream>

Eigen::VectorXd MNASolver::solveDC(Circuit& circuit) {
  const auto& components = circuit.getComponents();

  //-----------------------
  // For non-sequential node numbers, we need to map them to sequential indices.
  //-----------------------

  // --- Step 1: Determine number of node voltage unknowns ---
  // We assume:
  //   - 0 = ground
  //   - all other node numbers are positive ints
  //   - stamping uses (node - 1) as matrix index
  int max_node = 0;
  for (const auto& comp : components) {
    const std::vector<int>& nodes = comp->getNodes();
    for (int n : nodes) {
      if (n > max_node) {
        max_node = n;
      }
    }
  }

  const int num_nodes = max_node;  // rows 0..num_nodes-1 ↔ nodes 1..max_node

  std::cout << "Max node number in circuit: " << max_node << std::endl;
  std::cout << "Node voltage unknowns (excluding ground): " << num_nodes
            << std::endl;

  // --- Step 2: Extra variables (currents of V-sources and inductors) ---
  // const int num_extra_vars = circuit.getVoltageSourceCount() +
  // circuit.getInductorCount();

  const int num_extra_vars = circuit.getVoltageSourceCount();

  const int matrix_size = num_nodes + num_extra_vars;

  std::cout << "Starting DC Analysis..." << std::endl;
  std::cout << "Nodes: " << num_nodes << ", V-Sources: " << num_extra_vars
            << std::endl;
  std::cout << "Matrix size: " << matrix_size << " x " << matrix_size
            << std::endl;

  // --- Step 3: Initialize matrices ---
  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(matrix_size, matrix_size);
  Eigen::VectorXd z = Eigen::VectorXd::Zero(matrix_size);
  std::cout << "Initialized matrices A and z." << std::endl;

  // --- Step 4: Assign indices to voltage sources and inductors ---
  // These go after the node voltage rows/cols: indices [num_nodes ..
  // matrix_size)
  int current_idx = num_nodes;
  for (const auto& comp : components) {
    if (auto* vsource = dynamic_cast<VoltageSource*>(comp.get())) {
      vsource->setCurrentIndex(current_idx++);
      continue;
    }

    if (auto* acvsource = dynamic_cast<ACVoltageSource*>(comp.get())) {
      acvsource->setCurrentIndex(current_idx++);
    }
  }

  // --- Step 5: Stamp all components (Resistor::stampDC still uses n-1
  // indexing) ---
  for (const auto& comp : components) {
    comp->stampDC(A, z);
  }

  // --- Step 6: Solve the linear system A * x = z ---
  std::cout << "Solving system..." << std::endl;
  std::cout << "Matrix A:\n" << A << std::endl;
  std::cout << "Vector z:\n" << z << std::endl;

  Eigen::VectorXd x = A.colPivHouseholderQr().solve(z);

  std::cout << "Solution x:\n" << x << std::endl;

  return x;
}

Eigen::VectorXcd MNASolver::solveAC(Circuit& circuit, double omega) {
  const auto& components = circuit.getComponents();

  //-----------------------
  // For non-sequential node numbers, we use the same scheme as DC:
  // find the maximum node index and size the matrix accordingly.
  //-----------------------

  // --- Step 1: Determine number of node voltage unknowns ---
  // We assume:
  //   - 0 = ground
  //   - all other node numbers are positive ints
  //   - stamping uses (node - 1) as matrix index
  int max_node = 0;
  for (const auto& comp : components) {
    const std::vector<int>& nodes = comp->getNodes();
    for (int n : nodes) {
      if (n > max_node) {
        max_node = n;
      }
    }
  }

  const int num_nodes = max_node;  // rows 0..num_nodes-1 ↔ nodes 1..max_node

  std::cout << "[AC] Max node number in circuit: " << max_node << std::endl;
  std::cout << "[AC] Node voltage unknowns (excluding ground): " << num_nodes
            << std::endl;

  // --- Step 2: Extra variables (currents of V-sources and inductors) ---
  const int num_extra_vars = circuit.getVoltageSourceCount();
  const int matrix_size = num_nodes + num_extra_vars;

  std::cout << "[AC] Starting AC Analysis..." << std::endl;
  std::cout << "[AC] Nodes: " << num_nodes << ", V-Sources: " << num_extra_vars
            << std::endl;
  std::cout << "[AC] Matrix size: " << matrix_size << " x " << matrix_size
            << std::endl;

  // --- Step 3: Initialize matrices (complex) ---
  Eigen::MatrixXcd A = Eigen::MatrixXcd::Zero(matrix_size, matrix_size);
  Eigen::VectorXcd z = Eigen::VectorXcd::Zero(matrix_size);
  std::cout << "[AC] Initialized matrices A and z." << std::endl;

  // --- Step 4: Assign indices to voltage sources and inductors ---
  // Same idea as DC: extra unknown currents start after node voltages.
  int current_idx = num_nodes;
  for (const auto& comp : components) {
    if (auto* vsource = dynamic_cast<VoltageSource*>(comp.get())) {
      vsource->setCurrentIndex(current_idx++);
      continue;
    }
    if (auto* acvsource = dynamic_cast<ACVoltageSource*>(comp.get())) {
      acvsource->setCurrentIndex(current_idx++);
    }
  }

  // --- Step 5: Stamp all components for AC ---
  for (const auto& comp : components) {
    comp->stampAC(A, z, omega);
  }

  // --- Step 6: Solve the linear system A * x = z ---
  std::cout << "[AC] Solving system..." << std::endl;
  std::cout << "[AC] Matrix A:\n" << A << std::endl;
  std::cout << "[AC] Vector z:\n" << z << std::endl;

  Eigen::VectorXcd x = A.colPivHouseholderQr().solve(z);

  std::cout << "[AC] Solution x:\n" << x << std::endl;

  return x;
}
