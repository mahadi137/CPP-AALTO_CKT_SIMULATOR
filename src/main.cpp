#include "circuit.hpp"
#include "currentsource.hpp"
#include "resistor.hpp"
#include "voltagesource.hpp"

int main() {
  Circuit c;
  c.Clear();

  // -----FOR OPEN METHOD------
  // -----DO SAME-----
  // ------NODE POSITIONS ARE THE KEY-------

  c.Add(Resistor::Create("R1", 1, 2, 1));
  c.Add(VoltageSource::VDC("VDC2", 3, 2, 6));
  c.Add(Resistor::Create("R2", 2, 0, 4));
  c.Add(Resistor::Create("R3", 3, 0, 2));
  c.Add(VoltageSource::VDC("VDC1", 1, 0, 4));
  c.Add(CurrentSource::ISRC("ISRC1", 1, 2, 1));

  // -----DC ANALYSIS-----
  c.AnalysisDC();
  c.GetRLCCurrent();

  // -----SAVE TO TXT-----
  c.SaveNetlist();
}