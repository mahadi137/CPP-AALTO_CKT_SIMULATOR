# Test files

This folder contains unit and integration tests for the circuit simulator using
GoogleTest framework.

## Running Tests

### Build and run all tests:

```bash
cd build
cmake ..
make
ctest --output-on-failure
```

### Run tests directly with GoogleTest:

```bash
cd build
./circuit_tests                      # Run all tests
./circuit_tests --gtest_filter=*     # Run all tests
./circuit_tests --gtest_filter=CircuitTest.VoltageDivider  # Run specific test
```

### GoogleTest Features:

- **EXPECT_NEAR**: For floating-point comparisons with tolerance
- **ASSERT_NO_THROW**: Ensures parsing/solving doesn't crash
- **Test Discovery**: CTest automatically discovers all GoogleTest tests
- **Detailed Output**: Clear test names and failure messages

# Unit Tests

## Test 1: Voltage Divider

**Involved Classes:** Circuit, MNASolver, Resistor, VoltageSource

**Test File:** `test_voltage_divider.cpp`

**Circuit:** 10V source with 1kΩ and 2kΩ resistors in series

**Expected Results:**

- Node 1: 10V (at voltage source)
- Node 2: 6.667V (voltage divider point)
- Current: 3.33 mA

**Results:** ✅ **PASSED** - All voltages and currents match analytical
calculations within 1% tolerance

---

## Test 2: Series Resistors

**Involved Classes:** Circuit, MNASolver, Resistor, VoltageSource

**Test File:** `test_series_resistors.cpp`

**Circuit:** 12V source with 100Ω, 200Ω, and 300Ω resistors in series

**Expected Results:**

- Node 1: 12V (at voltage source)
- Node 2: 10V (after 100Ω)
- Node 3: 6V (after 200Ω)
- Current: 20 mA

**Results:** ✅ **PASSED** - Verifies Ohm's law for series resistance networks
