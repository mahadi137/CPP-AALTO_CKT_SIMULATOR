#include <gtest/gtest.h>

#include <cmath>

#include "Circuit.hpp"
#include "MNASolver.hpp"

/**
 * Test: Voltage Divider Circuit
 *
 * Circuit:
 *   V1 (10V) -- R1 (1kΩ) -- Node1 -- R2 (2kΩ) -- GND
 *
 * Expected Results (Ohm's Law):
 *   Total Resistance = 1k + 2k = 3kΩ
 *   Current = V/R = 10V / 3kΩ = 0.00333 A (3.33 mA)
 *   Voltage at Node1 = I * R2 = 0.00333 * 2000 = 6.667 V
 *
 * This tests:
 * - Resistor stamping
 * - Voltage source stamping
 * - DC solver correctness
 */

TEST(CircuitTest, VoltageDivider) {
  // Define netlist
  const std::string netlist = R"(
* Voltage Divider Test Circuit
V1 1 0 10V
R1 1 2 1000
R2 2 0 2000
)";

  // Parse circuit
  Circuit circuit;
  ASSERT_NO_THROW(circuit.parseNetlist(netlist))
      << "Netlist parsing should not throw";

  // Solve circuit
  Eigen::VectorXd solution;
  ASSERT_NO_THROW(solution = MNASolver::solveDC(circuit))
      << "Circuit solver should not throw";

  // Expected values
  const double expected_voltage_node1 = 10.0;   // Voltage source
  const double expected_voltage_node2 = 6.667;  // Voltage divider point
  const double expected_current =
      -0.00333;                   // Current through V1 (negative by convention)
  const double tolerance = 0.01;  // 1% tolerance

  // Get results
  const int num_nodes = circuit.getNodeCount();
  double voltage_node1 = solution(0);       // Node 1
  double voltage_node2 = solution(1);       // Node 2
  double current_v1 = solution(num_nodes);  // Current through V1

  // Verify Node 1 voltage
  EXPECT_NEAR(voltage_node1, expected_voltage_node1, tolerance)
      << "Node 1 voltage should match voltage source";

  // Verify Node 2 voltage
  EXPECT_NEAR(voltage_node2, expected_voltage_node2, tolerance)
      << "Node 2 voltage should match voltage divider calculation";

  // Verify current
  EXPECT_NEAR(current_v1, expected_current,
              tolerance * std::abs(expected_current))
      << "Current through voltage source should match Ohm's law";
}
