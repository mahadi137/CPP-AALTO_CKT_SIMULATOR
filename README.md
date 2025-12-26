# Circuit Simulator 3

A basic circuit simulator gives us RMS (root mean square) values for voltage and
current. For time or frequency domain values, solving of differential equations
is needed, either using complex number circuit analysis or ODE solvers.

This project includes both a command-line backend solver and a graphical user
interface (GUI) for interactive circuit design and simulation.

# Group

- Aykhan Najafov
- Nurgül Gündüz
- Rafin Jahan
- Sayad Hassan
- Thanh Vo

# Building the Project

## Prerequisites

- CMake 3.11 or higher
- C++17 compatible compiler
- SFML 2.5 or higher
- Eigen library (included in libs/)
- GoogleTest (included in libs/)

## Build Instructions

```bash
cd build
cmake ..
make
```

This will create two executables:

- `circuit-simulator` - Backend CLI for circuit simulation
- `circuit_gui` - GUI application for interactive circuit design

## Running the GUI Application

```bash
cd build
./circuit_gui
```

### GUI Features (In Progress)

- **Application Framework**: Main window with event handling (✅ Implemented)
- **Terminal Class**: Connection points for components (✅ Implemented)
- **Keyboard Shortcuts**:
  - `Ctrl+N` - New circuit
  - `Ctrl+S` - Save circuit
  - `F5` - Run DC simulation
- **Schematic Editor**: Visual circuit design (🚧 To be implemented)
- **Components**: Resistors, capacitors, voltage sources (🚧 To be implemented)
- **Circuit Controller**: Integration with backend solver (🚧 To be implemented)

# Repository organization

Your project implementation should follow the organization in this repository.
See readme.md files in each folder.

# Project Implementation

You must use git repository for the work on the project, making frequent enough
commits so that the project group (and course staff) can follow the progress.

The course staff should be able to easily compile the project work using
makefile and related instructions provided in the git repository. The final
output should be in the **master branch** of the git repository.

# Source code documentation

It is recommended to use Doxygen to document your source code. Please go over
the _Project Guidelines_ for details.
