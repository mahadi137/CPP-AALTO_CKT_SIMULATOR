## Meeting Notes – Sprint 1
**Date:** 24.10.2025 – 12:00-13:00

**Participants:**
- Aykhan Najafov
- Hassan Sayad
- Nurgül Gündüz
- Rafin Jahan
- Thanh Vo
- **TA:** Toikka Henrik


## 1. Summary of Works (Before Meeting)

| Member | Work Done |
| --- | --- |
| Hassan | Created initial GUI sketch inspired by LTSpice and made initial Project Plan based on the team’s needs. |
| Hassan | Attempted setting up ImGui + SFML + CMake, but static linking error persists. Used ChatGPT without success. |
| Vo Thanh | Experimented with MNA solver library integration & partially drafted Netlist parser. |
| Group | Drafted project plan (Word file), but not uploaded to repository yet. No UML class diagram or issue board. |


## 2. Challenges & Discussions

### GUI & CMake Setup Issues
- ImGui + SFML build gives static linking error.
- Team unsure about proper CMake setup & library integration.
- TA suggested pushing code to a branch so he can test/debug.

### Lack of Class Architecture
- No UML diagram or defined architecture yet.
- TA emphasized importance of deciding classes & responsibilities before coding.

### Understanding MNA & ODEint
- Difficulty understanding how to generate matrices for modified nodal analysis.
- Unsure how Boost ODEint integrates with circuit differential equations.
- TA pointed to resources and encouraged starting with a basic RLC circuit.

### GUI Implementation Concept Still Unclear
- How snapping, wires, nodes, and drag/drop logic will work is unclear.
- Team decided first goal is a blank ImGui/SFML canvas build.

### Sprint Delay
- Sprint 1 tasks must be completed by Sunday.
- Repository currently has no working code or documentation.


## 3. Agreed Action Points (Next Steps)

| Member            | Task |
|-------------------|------|
| **Aykhan Najafov** | Prepare Sprint 1 meeting summary and report. |
| **Hassan Sayad**  | Push ImGui + SFML current attempt to new Git branch for TA review. |
| **Vo Thanh**      | Draft initial UML class diagram & share by end of week + initial backend DC implementations. |
| **All members** | Finalize and upload project plan into `plan/` folder in repository. |
| **All Members**   | Explore ImGui + SFML basics individually (at least build a blank canvas). |
| **All Members**   | Study simple RLC example with modified nodal analysis. |
| **After GUI Build** | Create GitLab Issue Board (New/Open/Save/Help, toolbar, canvas setup, etc.). |

## 4. Project Status (After meeting)
- Project plan near finalization.
- Have not uploaded libraries into repository yet.
- CMake + GUI framework not building successfully yet.
- Close to no class templates or code committed.
- Sprint 1 is behind but expected progress by Sunday.
- TA approved starting simple (DC RLC simulation first).

## 5. TO-DOs summary
- Create a UML class diagram before dividing them among
members. (important and urgent)
- Understand Modified Nodal Analysis for circuit simulation.
- Resolve ImGui and SFML build issues. Everyone should look
into these libraries
- Make final changes to the Project Plan (till Sunday)
- Divide work among members based on their preference and
expertise.

**Next TA&group meeting:** 31.10.2025 - 12:00 (Sprint 2 review).

---

## Meeting Notes - Sprint 2

**Date:** 7.11.2025 – 12:00–12:45  
**Participants:**
- Hassan Sayad  
- Nurgül Gündüz  
- Rafin Jahan  
- Thanh Vo  
**TA:** Toikka Henrik  


## 1. Summary of Works (Before Meeting)

| Member | Work Done |
|---------|------------|
| **Aykhan Najafov** | Reviewed GUI class responsibilities, and updated Sprint 1 report. |
| **Hassan Sayad** | Continued GUI implementation; worked on canvas, component placement, and explored grid snapping methods. |
| **Nurgül Gündüz** | Worked on GUI components assigned from the UML diagram (component rendering). |
| **Rafin Jahan** | Focused on backend – further developed MNA solver and tested simple RLC circuit simulation. |
| **Thanh Vo** | Continued work on netlist parser and class integration with the backend solver. |
| **Group** | Discussed integration plan between GUI and backend modules; reviewed division of GUI classes from previous sprint. |


## 2. Challenges & Discussions

### Wiring System Design
- Main topic of the meeting was how to handle **connections (wires) between components** in the schematic editor.  
- Several approaches were discussed, such as:  
  - Node-based connection representation  
  - Manual linking via wire objects  
  - Auto-snapping between nearby pins  
- Team agreed to **experiment with multiple methods** and compare usability before finalizing.

### GUI Grid Implementation
- Need for a **snap-to-grid system** to align components and wires accurately.  
- Grid design should be flexible and visually subtle to maintain a clean canvas.

### Project Timeline & Integration
- TA asked whether the project could be completed on time.  
- Team confirmed the timeline is **realistic**, provided that wiring logic and GUI–backend integration are completed soon.  
- Agreed to start integration tests immediately after the wiring prototype is working properly.


## 3. Agreed Action Points (Next Steps)

| Member | Task |
|---------|------|
| **Hassan Sayad** | Implement Main SFML application class which is connected to Circuit Controller and Schematic Editor |
| **Nurgül Gündüz** | Implement Schematic Editor class |
| **Rafin Jahan** | Implement Main SFML application class which is connected to Circuit Controller and Schematic Editor |
| **Thanh Vo** | Work on the AC Analysis and the Toolbar in frontend. |
| **Aykhan Najafov** | Work on Schematic Editor. Sprint 2 summary. |


## 4. Project Status (After Meeting)

- GUI framework successfully builds; base canvas functional.  
- Component rendering progressing well.  
- Wiring design under development – prototype planned before next sprint.  
- Backend solver (MNA) can simulate simple DC circuits.  
- Integration phase expected to begin after wiring prototype is stable.  


## 5. TO-DOs Summary

- Finalize **wiring implementation approach** before next sprint.  
- Implement **grid snapping system** in GUI. 
- Begin **initial GUI–backend integration tests** once wiring prototype is ready.  


**Next TA & Group Meeting:** 21.11.2025 – 12:00 (Sprint 3 Planning)
## 📝 Meeting Notes – Sprint 3

**Date:** 21.11.2025 – 12:00–12:45  
**Participants:**  
- Hassan Sayad  
- Nurgül Gündüz  
- Rafin Jahan  
- Thanh Vo  
- Aykhan Najafov
**TA:** Toikka Henrik  

---

## 1. Summary of Works (Before Meeting)

| Member | Work Done |
|---------|------------|
| **Aykhan Najafov** | Reviewed GUI class responsibilities, created SchematicEditor.cpp and finalized Sprint 2 summary. |
| **Hassan Sayad** | Continued GUI implementation; developed schematic editor canvas, measurement area, and component toolbar using GitHub Copilot. Added resistor, inductor, capacitor, wire, ground, current, and voltage source components. |
| **Nurgül Gündüz** | Worked on GUI components and interaction logic within schematic editor. |
| **Rafin Jahan** | Tested backend MNA solver with different circuit node connections and observed non-sequential node numbering issues. |
| **Thanh Vo** | Worked on AC Analysis implementation and frontend toolbar development. |
| **Group** | Discussed GUI-backend integration, node numbering issues, and essential features (save/open files, editing components, current measurement). |

---

## 2. Challenges & Discussions

### Node Numbering Issue
- Current system assigns random node IDs (e.g., 1, 3, 5 instead of 1, 2, 3).  
- Needs to be sequential and consistently updated after wiring.  
- Ground connection updates netlist dynamically, but numbering logic must ensure proper reference node (0 V).

### Component Editing & Deletion
- Enable **double-click editing** of component names and values.  
- Automatic naming to prevent duplicate IDs.  
- Deleting components must also remove corresponding node numbers.

### File Save & Open Functionality
- **Essential feature** missing from current simulator.  
- Required for basic-level completion according to A-Plus project guidelines.  
- Will allow loading and re-opening of user circuits and netlists.

### Measurement Enhancements
- Plan to automatically display **currents through each component** along with node voltages after simulation.  
- Requires integration between backend MNA results and GUI measurement view.

### Implementation Scope & Priorities
- Complete **basic-level features first** (editing, file handling, sequential nodes) before intermediate/advanced tasks (AC graphs, frequency domain, etc.).  

---

## 3. Agreed Action Points (Next Steps)

| Member | Task |
|---------|------|
| **Hassan Sayad** | Maintain main SFML application integrating `CircuitController` and `SchematicEditor`. |
| **Nurgül Gündüz** | Implement **component editing** and ensure node A/B logic and deletion updates node numbering. |
| **Rafin Jahan** | Implement **file save and open** functionalities. |
| **Aykhan Najafov** | Fix **node numbering logic** (sequential numbering) and prepare Sprint 3 report. |
| **Thanh Vo** | Continue **AC Analysis** and toolbar improvements. |

---

## 4. Project Status (After Meeting)

- GUI and canvas functional with all main components (R, L, C, wire, source, ground).  
- Node numbering works but not sequential — fix required.  
- Ground connection updates netlist successfully.  
- Component editing and deletion under development.  
- File save/open and measurement enhancements pending.  
- Backend MNA solver stable for DC analysis; AC analysis ongoing.  

---

## 5. TO-DOs Summary

- Fix **node numbering** logic (sequential order).  
- Implement **file save/open**.  
- Add **component editing** with proper node A/B handling and deletion behavior.  
- Integrate **current display** in measurements.  
- Complete **basic-level features** before starting intermediate ones.  

---

## 6. Tasks Assignment

| **Task** | **Description** | **Assigned To** |
|-----------|----------------|----------------|
| **1** | Node numbering and editing the component parameters (Node A and B logic; deletion updates nodes) | **Nurgül Gündüz** |
| **2** | Saving and opening file functionality | **Rafin Jahan** |
| **3** | Node number fixing (sequential instead of random) | **Aykhan Najafov** |

---

**Next TA & Group Meeting:** 5.12.2025 – 12:00 (Sprint 4 Planning)

## 📝 Meeting Notes – Sprint 4

**Date:** 08.12.2025
**Duration:** 18:00-19:00
**Participants:**  
- Aykhan Najafov
- Vo Thanh
- Nurgul Gunduz
- Hassan Sayad
**TA:** Toikka Henrik  

---

## 1. Summary of Works (Before Meeting)

### General Progress
- The repository now includes initial unit tests (`series_resistors` and `voltage_divider`).  
- GUI and backend development continued during the week, but **no major updates** were provided this meeting due to limited availability of some members.
- Hassan informed the team that he is currently **outside Finland** and will not be able to contribute this week or attend the upcoming demo.

### Individual Updates
| Member | Summary |
|--------|---------|
| **Hassan Sayad** | No progress this week; will not be available for the demo. |
| **Team (general)** | Using the GUI for testing and verifying integration; planning remaining tasks before demo. |

---

## 2. Challenges & Discussions

### 2.1 Unit Testing
- TA noted that only two tests are currently present.
- Recommended **expanding the test suite** to cover:  
  - more component combinations  
  - edge cases and invalid circuits  
  - backend calculations  
- Tests must be included **inside the repository** and be clearly documented.

### 2.2 Demo Requirements
- A demo will take place **later this week** (Thursday or Friday).  
- TA will send a set of possible time slots, and the team should choose one that works for most members.
- Full team attendance is **not required**, but at least one or two members must present.
- Demo content may be:  
  - a **live demonstration** of the current simulator, or  
  - a **short slide deck** summarizing progress.
- A+ documentation mentions **SFML 2.5**, but TA confirmed the team **does not need to downgrade**.  
- Whatever version is being used now is acceptable as long as:  
  **→ It is clearly documented in the repository.**

### 2.4 Documentation Expectations
- TA reminded the team that all project documentation must be placed inside the repository under the `doc/` folder.
- This includes:  
  - dependency versions  
  - build instructions  
  - testing description  
  - sprint summaries  
  - demo materials (if applicable)

---

## 3. Agreed Action Points (Next Steps)

| Member | Task |
|--------|------|
| **Remaining team members** | Prepare the **demo** and select a suitable time slot once TA sends options. |
| **Team (general)** | Add **more unit tests** covering broader functionality. |
| **Team (general)** | Document **all dependency versions** (SFML, compilers, libraries) used in the project. |
| **Team (general)** | Ensure all documentation is located inside the `doc/` directory. |
| **Hassan** | Resume contributions next week once back in Finland. |

---

## 4. Project Status (After Meeting)

- Project is progressing but still requires:  
  - more robust testing  
  - documentation improvements  
  - demo preparation  
- Core GUI and backend functionality continue to evolve, but polishing and integration work will continue in Sprint 5.
- Team availability is partially reduced this week, but remaining members will prepare the demo.

---

## 5. Immediate To-Do Checklist

- [ ] Select demo time when TA sends the scheduling options  
- [ ] Prepare 10–15 min demo (live app or slides)  
- [ ] Add several new unit tests to increase coverage  
- [ ] Document SFML and other version dependencies  
- [ ] Clean up and organize documentation under `doc/`  
