#pragma once

#include <memory>
#include <string>
#include <vector>

#include "components/ACCurrentSource.hpp"
#include "components/ACVoltageSource.hpp"
#include "components/Capacitor.hpp"
#include "components/Component.hpp"
#include "components/DCCurrentSource.hpp"
#include "components/Inductor.hpp"
#include "components/Resistor.hpp"
#include "components/VoltageSource.hpp"

class Circuit {
 public:
  Circuit() = default;

  // --- Programmatic Building ---
  void addResistor(const std::string& name, double resistance, int n1, int n2);
  void addCapacitor(const std::string& name, double cap, int n1, int n2);
  void addInductor(const std::string& name, double ind, int n1, int n2);
  void addVoltageSource(const std::string& name, double voltage, int n1,
                        int n2);
  void addACVoltageSource(const std::string& name, double acAmplitude, int n1,
                          int n2);
  void addDCCurrentSource(const std::string& name, double current, int n1,
                          int n2);
  void addACCurrentSource(const std::string& name, double acMagnitude, int n1,
                          int n2);

  // --- Netlist Parsing ---
  // Parses a multi-line string in ngspice netlist format.
  void parseNetlist(const std::string& netlist_content);

  // --- Getters for Solver ---
  int getNodeCount() const { return _node_count; }
  int getVoltageSourceCount() const { return _vsource_count; }
  int getInductorCount() const { return _inductor_count; }
  const std::vector<std::unique_ptr<Component>>& getComponents() const {
    return _components;
  }

 private:
  void updateNodeCount(int n1, int n2);

  std::vector<std::unique_ptr<Component>> _components;  // Components vector
  int _node_count = 0;
  int _vsource_count = 0;
  int _inductor_count = 0;
};