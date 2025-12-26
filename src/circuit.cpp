#include "Circuit.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Utils.hpp"

void Circuit::updateNodeCount(int n1, int n2) {
  _node_count = std::max({_node_count, n1, n2});
}

// --- Public Methods for Programmatic Building ---
void Circuit::addResistor(const std::string& name, double r, int n1, int n2) {
  _components.push_back(std::make_unique<Resistor>(name, r, n1, n2));
  updateNodeCount(n1, n2);
}

void Circuit::addCapacitor(const std::string& name, double c, int n1, int n2) {
  _components.push_back(std::make_unique<Capacitor>(name, c, n1, n2));
  updateNodeCount(n1, n2);
}

void Circuit::addInductor(const std::string& name, double l, int n1, int n2) {
  _components.push_back(std::make_unique<Inductor>(name, l, n1, n2));
  updateNodeCount(n1, n2);
  _inductor_count++;
}

void Circuit::addVoltageSource(const std::string& name, double v, int n1,
                               int n2) {
  _components.push_back(std::make_unique<VoltageSource>(name, v, n1, n2));
  updateNodeCount(n1, n2);
  _vsource_count++;
}

void Circuit::addACVoltageSource(const std::string& name, double v, int n1,
                                 int n2) {
  _components.push_back(std::make_unique<ACVoltageSource>(name, v, n1, n2));
  updateNodeCount(n1, n2);
  _vsource_count++;
}

void Circuit::addDCCurrentSource(const std::string& name, double i, int n1,
                                 int n2) {
  _components.push_back(std::make_unique<DCCurrentSource>(name, i, n1, n2));
  updateNodeCount(n1, n2);
}
void Circuit::addACCurrentSource(const std::string& name, double i, int n1,
                                 int n2) {
  _components.push_back(std::make_unique<ACCurrentSource>(name, i, n1, n2));
  updateNodeCount(n1, n2);
}
// --- Public Method for Netlist Parsing ---
void Circuit::parseNetlist(const std::string& netlist_content) {
  std::stringstream ss(netlist_content);
  std::string line;

  // The first line is reserved for title.
  std::getline(ss, line);

  while (std::getline(ss, line)) {
    // Trim leading/trailing whitespace safely
    auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos) {
      continue;  // line is all whitespace
    }
    auto last = line.find_last_not_of(" \t");
    line = line.substr(first, last - first + 1);

    if (line.empty() || line[0] == '*' || line[0] == '.') {
      // Skip comments, control cards, and empty lines
      continue;
    }

    std::stringstream line_ss(line);

    // First token is the component name, e.g. "R1", "V1", "VAC1"
    std::string token;
    line_ss >> token;
    if (token.empty()) {
      continue;
    }

    char type =
        static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));

    std::string n1_str, n2_str, val_str;

    try {
      switch (type) {
        case 'R': {  // Resistor
          std::string name = token;
          line_ss >> n1_str >> n2_str >> val_str;
          addResistor(name, parseValue(val_str), std::stoi(n1_str),
                      std::stoi(n2_str));
          break;
        }
        case 'V': {  // Voltage source (DC or AC, depending on name)
          std::string name = token;
          line_ss >> n1_str >> n2_str >> val_str;

          // Distinguish AC vs DC by name prefix "VAC"
          std::string upperName = name;
          std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                         [](unsigned char c) { return std::toupper(c); });

          if (upperName.rfind("VAC", 0) == 0) {  // starts with "VAC"
            addACVoltageSource(name, parseValue(val_str), std::stoi(n1_str),
                               std::stoi(n2_str));
          } else {
            addVoltageSource(name, parseValue(val_str), std::stoi(n1_str),
                             std::stoi(n2_str));
          }
          break;
        }
        case 'I': {  // Current source
          std::string name = token;
          line_ss >> n1_str >> n2_str >> val_str;
          addDCCurrentSource(name, parseValue(val_str), std::stoi(n1_str),
                             std::stoi(n2_str));
          break;
        }
        case 'C': {  // Capacitor
          std::string name = token;
          line_ss >> n1_str >> n2_str >> val_str;
          addCapacitor(name, parseValue(val_str), std::stoi(n1_str),
                       std::stoi(n2_str));
          break;
        }
        case 'L': {  // Inductor
          std::string name = token;
          line_ss >> n1_str >> n2_str >> val_str;
          addInductor(name, parseValue(val_str), std::stoi(n1_str),
                      std::stoi(n2_str));
          break;
        }
        default:
          std::cerr << "Warning: Skipping unsupported component type: " << type
                    << " (token: " << token << ")\n";
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Could not parse line: '" << line
                << "'. Error: " << e.what() << std::endl;
    }
  }
}
