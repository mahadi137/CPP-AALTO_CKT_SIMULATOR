/**
 * @file Circuit.cpp
 * @brief MNA assembly and solves (DC and transient, no complex numbers).
 */

#include "circuit.hpp"

#include <Eigen/SparseLU>
#include <cmath>
#include <iomanip>   // for formatting
#include <iostream>  // for std::cout, std::setw
#include <stdexcept>

#include "component.hpp"

void Circuit::Add(const std::shared_ptr<Component>& c) { comps_.push_back(c); }

Eigen::VectorXd Circuit::AnalysisDC() {
  int total_nodes = 3;  // 0 (GND, not counted), 1, 2, 3, 4 unknown
  int total_VS_count = 2;
  // int total_IS_count = 1;
  //  total unknowns = node voltages + source currents
  //  for Z matrix
  const int NS = total_nodes + total_VS_count;

  MNA_matrix<double> A;
  Eigen::VectorXd Z = Eigen::VectorXd::Zero(NS);

  // aim is to have vector with all I1, V1, V2, R1, R2, R3, ...
  // THIS SORT IS VERY IMPORTANT WITH COMP. NUMBERING
  // sorted will keep original vector as it is
  std::vector<std::shared_ptr<Component>> sorted_comps = comps_;
  auto order = [](const std::string& kind) {
    if (kind == "CurrentSource") return 0;  // voltage sources first
    if (kind == "VoltageSource") return 1;  // current sources next
    if (kind == "Resistor") return 2;
    if (kind == "Capacitor") return 3;
    if (kind == "Inductor") return 4;
    // need to return something
    // 5 is presenting else component.
    return 5;
  };

  std::sort(sorted_comps.begin(), sorted_comps.end(),
            [&](const std::shared_ptr<Component>& a,
                const std::shared_ptr<Component>& b) {
              int pa = order(a->GetComponentKind());
              int pb = order(b->GetComponentKind());
              if (pa != pb) {
                return pa < pb;  // lower number first
              }
              // EXPECTATION: components named as VDC1, VDC2
              // name without expectation formate may not work.
              return a->GetName() < b->GetName();
            });

  // No. of VoltageSources (or source currents) connected to nodes
  // initiate 0, this is required to make matrix B, 1/C matrix of matrix A
  int nextVS = 0;
  // here: vsBase = unknown node voltages (v_1, v_2, ..)
  const int vsBase = total_nodes;
  for (const auto& comp : sorted_comps) {
    comp->StampDC(total_nodes, vsBase, A, Z, nextVS);
    std::cout << comp->GetName() << std::endl;
  }

  // ---------COMPUTATION START----------
  Eigen::SparseMatrix<double> Axx(NS, NS);
  Axx.setFromTriplets(A.mna.begin(), A.mna.end());

  // ----- DEBUG PRINT: show A in dense form -----
  std::cout << std::endl
            << "MNA Matrix A (" << NS << "x" << NS << "):" << std::endl;
  Eigen::MatrixXd Adense(Axx);
  std::cout << Adense << std::endl;

  // ----- DEBUG PRINT: show RHS vector Z -----
  std::cout << std::endl
            << "RHS Vector Z:" << std::endl
            << Z.transpose() << std::endl;

  // Solve system
  Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
  // Use the SparseLU factorization computed for matrix A
  solver.compute(Axx);

  if (solver.info() != Eigen::Success)
    throw std::runtime_error("DC factorization failed");

  // solver.solve(Z) uses that factorization to compute x
  Eigen::VectorXd x = solver.solve(Z);

  if (solver.info() != Eigen::Success)
    throw std::runtime_error("DC solve failed");

  // ----- DEBUG PRINT: show solution X -----
  std::cout << std::endl
            << "Solution Vector X (voltages & currents):" << std::endl;

  for (int i = 0; i < total_nodes; i++) {
    double val_x = x(i);
    mna_matrix_x_.push_back(val_x);
    std::cout << "Node_" << (i + 1) << " = " << val_x << std::endl;
  }
  for (int j = 0; j < total_VS_count; j++) {
    int index = total_nodes + j;
    double val_x = x(index);
    mna_matrix_x_.push_back(val_x);
    std::cout << "I_VS_" << (j + 1) << " = " << val_x << std::endl;
  }
  return x;
}

//----------NETLIST---------
void Circuit::SaveNetlist() const {
  std::vector<std::shared_ptr<Component>> sorted_comps = comps_;
  auto order = [](const std::string& kind) {
    if (kind == "CurrentSource") return 0;  // voltage sources first
    if (kind == "VoltageSource") return 1;  // current sources next
    if (kind == "Resistor") return 2;
    if (kind == "Capacitor") return 3;
    if (kind == "Inductor") return 4;
    // need to return something
    // 5 is presenting else component.
    return 5;
  };

  std::sort(sorted_comps.begin(), sorted_comps.end(),
            [&](const std::shared_ptr<Component>& a,
                const std::shared_ptr<Component>& b) {
              int pa = order(a->GetComponentKind());
              int pb = order(b->GetComponentKind());
              if (pa != pb) {
                return pa < pb;  // lower number first
              }
              // EXPECTATION: components named as VDC1, VDC2
              // name without expectation formate may not work.
              return a->GetName() < b->GetName();
            });

  std::cout << std::endl << "Netlists:" << std::endl;
  for (const auto& c : sorted_comps) {
    std::cout << c->GetNetlist() << std::endl;
  }
}

void Circuit::GetRLCCurrent() const {
  std::cout << std::endl;
  std::cout << "RLC components currents:" << std::endl;

  for (const auto& comp : comps_) {
    if (comp->GetComponentKind() != "VoltageSource" &&
        comp->GetComponentKind() != "CurrentSource") {
      // Get node voltages based on component node
      // node voltages stored in mna_matrix_x_
      // node voltages stored sequentially
      double node_a_v = mna_matrix_x_[(comp->node_a) - 1];
      double node_b_v = mna_matrix_x_[(comp->node_b) - 1];

      double comp_val = comp->GetValue();
      double comp_current = (node_a_v - node_b_v) / comp_val;

      std::cout << comp->GetName() << " = " << comp_current << " A"
                << std::endl;
    }
  }
}

void Circuit::Clear() {
  comps_.clear();
  mna_matrix_x_.clear();
}
