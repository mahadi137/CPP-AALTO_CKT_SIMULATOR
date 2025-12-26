#include <gtest/gtest.h>

#include <cmath>

#include "Circuit.hpp"
#include "MNASolver.hpp"

/**
 * Test: Series Resistor Circuit
 *
 * Circuit:
 *   V1 (12V) -- R1 (100Ω) -- Node1 -- R2 (200Ω) -- Node2 -- R3 (300Ω) -- GND
 *
 * Expected Results:
 *   Total Resistance = 100 + 200 + 300 = 600Ω
 *   Current = V/R = 12V / 600Ω = 0.02 A (20 mA)
 *   Voltage at Node1 = 12 - (I * R1) = 12 - (0.02 * 100) = 10V
 *   Voltage at Node2 = 12 - (I * (R1+R2)) = 12 - (0.02 * 300) = 6V
 */

TEST(CircuitTest, SeriesResistors) {
  const std::string netlist = R"(
* Series Resistor Test Circuit
V1 1 0 12V
R1 1 2 100
R2 2 3 200
R3 3 0 300
)";

  Circuit circuit;
  ASSERT_NO_THROW(circuit.parseNetlist(netlist))
      << "Netlist parsing should not throw";

  Eigen::VectorXd solution;
  ASSERT_NO_THROW(solution = MNASolver::solveDC(circuit))
      << "Circuit solver should not throw";

  const double expected_v1 = 12.0;
  const double expected_v2 = 10.0;
  const double expected_v3 = 6.0;
  const double tolerance = 0.01;

  double v1 = solution(0);
  double v2 = solution(1);
  double v3 = solution(2);

  // Verify all node voltages
  EXPECT_NEAR(v1, expected_v1, tolerance)
      << "Node 1 voltage should match voltage source";

  EXPECT_NEAR(v2, expected_v2, tolerance)
      << "Node 2 voltage should match series calculation";

  EXPECT_NEAR(v3, expected_v3, tolerance)
      << "Node 3 voltage should match series calculation";
}
