## Meeting Notes – Sprint 1
**Date:** 24.10.2025 – 12:00-13:00

**Participants:**
- Aykhan Najafov
- Hassan Sayad
- Nurgül Gündüz
- Rafin Jahan
- Thanh Vo
- **TA:** Toikka Henrik

---

## 1. Summary of Works (Before Meeting)

| Member | Work Done |
| --- | --- |
| Hassan | Created initial GUI sketch inspired by LTSpice and made initial Project Plan based on the team’s needs. |
| Hassan | Attempted setting up ImGui + SFML + CMake, but static linking error persists. Used ChatGPT without success. |
| Vo Thanh | Experimented with MNA solver library integration & partially drafted Netlist parser. |
| Group | Drafted project plan (Word file), but not uploaded to repository yet. No UML class diagram or issue board. |

---

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

---

## 3. Agreed Action Points (Next Steps)

| Member            | Task |
|-------------------|------|
| **Aykhan Najafov** | Prepare Sprint 1 meeting summary and report. |
| **Hassan Sayad**  | Push ImGui + SFML current attempt to new Git branch for TA review. |
| **Vo Thanh**      | Draft initial UML class diagram & share by end of week. |
| **All members** | Finalize and upload project plan into `plan/` folder in repository. |
| **All Members**   | Explore ImGui + SFML basics individually (at least build a blank canvas). |
| **All Members**   | Study simple RLC example with modified nodal analysis. |
| **After GUI Build** | Create GitLab Issue Board (New/Open/Save/Help, toolbar, canvas setup, etc.). |
---

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
