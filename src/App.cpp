
#include <dirent.h>    // For directory operations
#include <pwd.h>       // For getpwuid()
#include <sys/stat.h>  // For stat()
#include <sys/wait.h>  // For waitpid()
#include <unistd.h>    // For fork() and execlp()

#include <SFML/Graphics.hpp>
#include <algorithm>  // For std::sort
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "Utils.hpp"

#ifdef _WIN32
#include <windows.h>  // For CreateProcessA()
#endif

#include "Circuit.hpp"
#include "MNASolver.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "theme.hpp"
#include "timedomainGraph.hpp"

/**
 * Main Application Class for Circuit Simulator
 *
 * This class manages the entire circuit simulation application including:
 * - SFML window and graphics rendering
 * - ImGui user interface system
 * - Circuit component management and visualization
 * - Interactive wire drawing and component placement
 * - Grid system for precise component alignment
 * - Measurement and analysis display
 *
 * Architecture:
 * - Uses SFML for low-level graphics and window management
 * - Uses ImGui for immediate-mode GUI elements
 * - Integrates with Circuit/MNASolver for electrical simulation
 * - Maintains separate areas: menu, toolbar, expanded canvas, measurements
 */
class App {
 public:
  /**
   * Constructor - Initializes the application
   * Sets up SFML window, ImGui integration, and canvas grid system
   */
  App()
      : isFullScreen(false),
        measureRMS(false),
        measureMean(false),
        measureMax(false),
        measureMin(false),
        showGraphForNodeVolt(false),
        showGraphForVoltageSourceCurrent(false),
        wireMode(false),
        nodeVoltagesFound(false),
        isDrawingWire(false) {
    window.create(sf::VideoMode(1280, 720), "Circuit Simulator");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
      window.close();
    }

    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    arialFont = io.FontDefault;

    Theme::setProfessionalTheme();

    windowSize = ImVec2(window.getSize().x, window.getSize().y);

    initCanvasGrids();

    loadComponentImages();

    // Initialize graph renderer
    graph_ = new TimeDomainGraph();
  }

  ~App() {
    if (graph_) delete graph_;
    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();
  }

  void run() {
    while (window.isOpen()) {
      handleEvents();
      update();
      render();
    }
  }

 private:
  std::map<Component*, ImVec2> componentPositions;
  Eigen::VectorXd solution_;
  Eigen::VectorXcd acSolution_;
  TimeDomainGraph* graph_ = nullptr;  // Graph renderer for AC analysis
  sf::RenderWindow window;
  sf::Clock deltaClock;
  bool isFullScreen;
  ImVec2 windowSize;
  bool nodeVoltagesFound;
  bool measureRMS;
  bool measureMean;
  bool measureMax;
  bool isDCMode = false;  // Flag for DC analysis mode
  bool isACMode = false;  // Flag for AC analysis mode
  bool measureMin;
  bool showGraphForNodeVolt;
  bool showGraphForVoltageSourceCurrent;
  bool wireMode;
  bool isDrawingWire;
  bool openEditPopupFromCanvas =
      false;  // Flag to open ResistorPopup from canvas context
  bool isEditPopupCurrentlyOpen = false;  // Track if edit popup is open
  ImVec2 wireStart;
  struct WireEndpoint {
    Component* comp = nullptr;
    int leg = 0;
  };
  struct Wire {
    WireEndpoint a;
    WireEndpoint b;
  };
  std::vector<Wire> completedWires;

  struct VisualWire {
    std::vector<ImVec2> points;
    int startComponentIndex = -1;
    int startNodeIndex = -1;
    int endComponentIndex = -1;
    int endNodeIndex = -1;
    int carriedNodeNumber = -1;  // Node number carried by this wire
    ImU32 color = IM_COL32(0, 51, 153, 255);
    float thickness = 2.0f;
  };
  std::vector<VisualWire> visualWires;

  VisualWire currentWire;

  bool isWireSnapped = false;
  ImVec2 wireCurrentEnd;
  int snappedComponentIndex = -1;
  int snappedNodeIndex = -1;
  ImVec2 snappedNodePosition;

  int wireStartComponentIndex = -1;
  int wireStartNodeIndex = -1;

  static constexpr float A4_WIDTH = 1587.0f;   // A4 Landscape width (11.69")
  static constexpr float A4_HEIGHT = 1123.0f;  // A4 Landscape height (8.27")

  static const int gridSpacing =
      15;  // Fixed spacing for component node alignment (pixels)
  bool gridInitialized = false;
  std::vector<sf::RectangleShape> vLines;
  std::vector<sf::RectangleShape> hLines;
  sf::Color gridColor = sf::Color(180, 180, 180, 100);
  sf::Color canvasBackColor = sf::Color(250, 250, 250);
  sf::Color appBackColor = sf::Color(37, 37, 38);
  std::map<std::string, sf::Texture> componentTextures;
  std::map<std::string, sf::Sprite> componentSprites;
  bool imagesLoaded = false;
  struct VisualComponent {
    std::string name;
    std::string type;
    ImVec2 position;
    ImVec2 size;
    float rotation = 0.0f;
    float value = 0.0f;
    int nodeA = 0, nodeB = 0;
    ImU32 color = IM_COL32(70, 130, 180, 255);
    bool isSelected = false;

    ImVec2 labelOffset = ImVec2(0, -25);
    bool isLabelDragging = false;
    bool hasCustomLabelPos = false;

    VisualComponent(const std::string& n, const std::string& t, ImVec2 pos)
        : name(n), type(t), position(pos), size(60, 20) {}
  };

  std::vector<VisualComponent> visualComponents;
  int selectedComponentIndex = -1;
  bool isDraggingComponent = false;
  ImVec2 dragOffset;

  int selectedLabelIndex = -1;
  int draggingLabelIndex = -1;
  ImVec2 labelDragOffset;

  ImFont* arialFont = nullptr;
  bool isSelecting = false;
  ImVec2 selectionStart;
  ImVec2 selectionEnd;
  std::vector<int> selectedComponents;
  std::vector<int> selectedWires;

  // File operations
  std::string currentFilename = "";
  char saveFilenameBuffer[256] = "circuit.cir";
  char loadFilenameBuffer[256] = "circuit.cir";
  char exportNetlistBuffer[256] = "circuit.net";
  char importNetlistBuffer[256] = "circuit.net";
  bool showSaveDialog = false;
  bool showLoadDialog = false;
  bool showExportNetlistDialog = false;
  bool showImportNetlistDialog = false;

  // File browser state for Save/Load dialogs
  std::string currentBrowsePath = "";
  std::vector<std::string> browseDirectories;
  std::vector<std::string> browseFiles;
  std::string selectedFileName = "circuit.cir";

  // Node connection system for automatic circuit analysis
  struct NodeConnection {
    int nodeNumber;  // Unique node identifier
    std::set<std::pair<int, int>>
        connectedNodes;               // {componentIndex, nodeIndex (0=A, 1=B)}
    bool isGroundConnection = false;  // True if this connection includes GND

    NodeConnection(int number) : nodeNumber(number) {}
  };

  std::vector<NodeConnection> nodeConnections;  // All active node connections
  int nextNodeNumber = 1;  // Next available node number (0 reserved for GND)
  std::map<std::pair<int, int>, int>
      componentNodeToNumber;  // Fast lookup: {compIndex, nodeIndex} ->
                              // nodeNumber
  std::map<std::pair<int, int>, int>
      positionToNodeNumber;  // Reverse lookup: (x, y) position -> node number
                             // for inheritance
  std::map<std::pair<int, int>, int>
      positionToWireCarryNodeNumber;  // Track wire-carried node numbers at
                                      // positions

 public:
  Circuit circuit;

 private:
  /**
   * Returns the default label offset for a given component type and rotation.
   * For resistor, inductor, capacitor:
   * - Horizontal (0°, 180°): labels on top
   * - Vertical (90°, 270°): labels on right side
   * Voltage source, current source, ground: always on left
   */
  ImVec2 getDefaultLabelOffset(const std::string& componentType,
                               float rotation = 0.0f) {
    if (componentType == "resistor" || componentType == "inductor" ||
        componentType == "capacitor") {
      float normalizedRotation = fmod(rotation, 360.0f);
      if (normalizedRotation < 0) normalizedRotation += 360.0f;

      if (abs(normalizedRotation - 90.0f) < 45.0f ||
          abs(normalizedRotation - 270.0f) < 45.0f) {
        return ImVec2(55, -10);
      } else {
        return ImVec2(-25, -45);
      }
    } else if (componentType == "voltage_source" ||
               componentType == "ac_voltage_source" ||
               componentType == "dc_current_source" ||
               componentType == "ac_current_source" ||
               componentType == "ground") {
      return ImVec2(-80, -10);
    }
    return ImVec2(0, -25);
  }

  /**
   * Converts an ImVec2 position to a pair<int, int> for use as map key.
   * Rounds coordinates to nearest integer.
   */
  std::pair<int, int> positionToKey(ImVec2 pos) {
    return std::make_pair((int)std::round(pos.x), (int)std::round(pos.y));
  }

  /**
   * Stores a node number at a specific position for later lookup.
   * Updates positionToNodeNumber map: (x, y) position -> node number
   * IMPORTANT: This does NOT update positionToWireCarryNodeNumber to avoid
   * overwriting wire node numbers. Wire positions are managed separately
   * by storeWireCarryNodeAtPosition().
   */
  void storeNodeAtPosition(int nodeNum, ImVec2 position) {
    auto key = positionToKey(position);
    positionToNodeNumber[key] = nodeNum;

    // auto it = positionToWireCarryNodeNumber.find(key);
    // if (it != positionToWireCarryNodeNumber.end()) {
    //   it->second = nodeNum;
    //   printf("\nUpdated existing node at position (%d, %d) from %d to %d\n",
    //          key.first, key.second, it->second, nodeNum);
    // } else {
    //   positionToWireCarryNodeNumber[key] = nodeNum;
    // }
    printf("\nStored node %d at position (%.1f, %.1f) -> key (%d, %d)\n",
           nodeNum, position.x, position.y, key.first, key.second);

    // DO NOT update positionToWireCarryNodeNumber here - it should only
    // be updated via storeWireCarryNodeAtPosition() to keep wires' carried
    // nodes separate from component positions.
  }

  /**
   * Gets node number at a position from positionToNodeNumber map.
   * Uses tolerant lookup: first tries exact match, then searches nearby within
   * SNAP_DISTANCE. Checks both component nodes and wire-carried nodes.
   * Returns the node number if found, -1 otherwise.
   */
  int getNodeNumberAtPosition(ImVec2 position, float SNAP_DISTANCE = 20.0f) {
    auto key = positionToKey(position);

    // FIRST: Check if starting on an existing wire (any point - start, middle,
    // or end) New wires starting from existing wires should adopt that wire's
    // node number
    for (const auto& wire : visualWires) {
      if (wire.carriedNodeNumber < 0) continue;

      // Check ALL wire points (not just start and end)
      for (size_t i = 0; i < wire.points.size(); ++i) {
        ImVec2 wirePoint = wire.points[i];
        float dx = wirePoint.x - position.x;
        float dy = wirePoint.y - position.y;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist <= SNAP_DISTANCE) {
          if (i == 0) {
            printf(
                "Found existing wire at START position (%.1f, %.1f) carrying "
                "node %d (dist %.1f)\n",
                wirePoint.x, wirePoint.y, wire.carriedNodeNumber, dist);
          } else if (i == wire.points.size() - 1) {
            printf(
                "Found existing wire at END position (%.1f, %.1f) carrying "
                "node %d (dist %.1f)\n",
                wirePoint.x, wirePoint.y, wire.carriedNodeNumber, dist);
          } else {
            printf(
                "Found existing wire at MIDDLE position (%.1f, %.1f) carrying "
                "node %d (dist %.1f)\n",
                wirePoint.x, wirePoint.y, wire.carriedNodeNumber, dist);
          }
          return wire.carriedNodeNumber;
        }
      }
    }

    // Second, try exact key lookup in positionToNodeNumber map (component
    // nodes)
    auto it = positionToNodeNumber.find(key);
    if (it != positionToNodeNumber.end()) {
      printf("Found node %d at position (%.1f, %.1f) via exact match\n",
             it->second, position.x, position.y);
      return it->second;
    }

    // Third, search nearby positions within SNAP_DISTANCE in
    // positionToNodeNumber map
    for (auto& entry : positionToNodeNumber) {
      float ex = (float)entry.first.first;
      float ey = (float)entry.first.second;
      float dx = position.x - ex;
      float dy = position.y - ey;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist <= SNAP_DISTANCE) {
        printf(
            "Found node %d at position (%.1f, %.1f) via nearby search (dist "
            "%.1f)\n",
            entry.second, position.x, position.y, dist);
        return entry.second;
      }
    }

    // Finally, search in positionToWireCarryNodeNumber map for wire-carried
    // nodes
    for (auto& entry : positionToWireCarryNodeNumber) {
      float ex = (float)entry.first.first;
      float ey = (float)entry.first.second;
      float dx = position.x - ex;
      float dy = position.y - ey;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist <= SNAP_DISTANCE) {
        printf(
            "Found wire-carried node %d at position (%.1f, %.1f) via nearby "
            "search (dist %.1f)\n",
            entry.second, position.x, position.y, dist);
        return entry.second;
      }
    }

    printf("No node found at position (%.1f, %.1f)\n", position.x, position.y);
    return -1;
  }

  /**
   * Stores a wire-carried node number at a position.
   * Used when a wire carries a node through its path to endpoint.
   */
  void storeWireCarryNodeAtPosition(int nodeNum, ImVec2 position) {
    auto key = positionToKey(position);

    // Check if this position already exists in the map
    if (positionToWireCarryNodeNumber.find(key) !=
        positionToWireCarryNodeNumber.end()) {
      int existingNodeNum = positionToWireCarryNodeNumber[key];
      if (existingNodeNum != nodeNum) {
        printf(
            "Position (%.1f, %.1f) already has wire-carry node %d, "
            "UPDATING to latest node %d\n",
            position.x, position.y, existingNodeNum, nodeNum);
        // Update to the latest (new) node number
        positionToWireCarryNodeNumber[key] = nodeNum;

        printf("  All positionToWireCarryNodeNumber entries: ");
        for (auto& entry : positionToWireCarryNodeNumber) {
          printf("[%d,%d]=%d ", entry.first.first, entry.first.second,
                 entry.second);
        }
        printf("\n");
      }
      return;
    }

    // Position doesn't exist yet - store the new node number
    positionToWireCarryNodeNumber[key] = nodeNum;
    printf(
        "Stored wire-carry node %d at position (%.1f, %.1f) -> key (%d, %d)\n",
        nodeNum, position.x, position.y, key.first, key.second);

    // Ensure any visual wire that contains this position reflects the same
    // carried node number. This keeps visualWires[].carriedNodeNumber in
    // sync with the position map so lookups using getWireNodeNumber() will
    // return the correct carried node.
    for (auto& wire : visualWires) {
      for (const auto& pt : wire.points) {
        auto ptKey = positionToKey(pt);
        if (ptKey == key) {
          if (wire.carriedNodeNumber != nodeNum) {
            wire.carriedNodeNumber = nodeNum;
            ImVec2 ws = wire.points.front();
            ImVec2 we = wire.points.back();
            printf(
                "Synced visualWire (start: %.0f, %.0f end: %.0f, %.0f) to "
                "carried node %d\n",
                ws.x, ws.y, we.x, we.y, nodeNum);
          }
          break;  // move to next wire after matching this point
        }
      }
    }
  }

  /**
   * Helper: Check if a position has an existing wire-carried node number.
   * Returns the node number if found, -1 otherwise.
   * Used when creating new components to inherit node numbers from wires.
   */
  /**
   * Gets wire-carried node number at a position.
   * Checks exact match first, then nearby positions within SNAP_DISTANCE.
   * Returns the node number if found, -1 otherwise.
   * This is the PRIMARY function for checking wire node at a position.
   */
  int getWireCarryNodeAtPosition(ImVec2 position, float SNAP_DISTANCE = 20.0f) {
    auto key = positionToKey(position);

    // Exact match first
    auto it = positionToWireCarryNodeNumber.find(key);
    if (it != positionToWireCarryNodeNumber.end()) {
      printf("Found wire-carry node %d at position (%.1f, %.1f)\n", it->second,
             position.x, position.y);
      return it->second;
    }

    // Nearby search within SNAP_DISTANCE
    for (const auto& entry : positionToWireCarryNodeNumber) {
      ImVec2 entryPos =
          ImVec2((float)entry.first.first, (float)entry.first.second);
      float dx = entryPos.x - position.x;
      float dy = entryPos.y - position.y;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist <= SNAP_DISTANCE) {
        printf(
            "Found wire-carried node %d at position (%.1f, %.1f) via nearby "
            "search (dist %.1f)\n",
            entry.second, entryPos.x, entryPos.y, dist);
        return entry.second;
      }
    }
    return -1;
  }

  /**
   * Always-check: Synchronize visual component node numbers from actual wire
   * positions. Checks each component's node positions against all wire
   * endpoints. Updates component node numbers ONLY if position matches a wire
   * endpoint exactly. If no match found, keeps the component's original node
   * number.
   */
  void syncComponentNodesFromWireCarry() {
    const float SNAP_DISTANCE = 25.0f;  // Snap distance for position matching

    for (int i = 0; i < (int)visualComponents.size(); ++i) {
      VisualComponent& comp = visualComponents[i];
      auto [nodeAPos, nodeBPos] = getComponentNodePositions(comp);

      // Check nodeA: FIRST check positionToNodeNumber map for existing stored
      // node
      auto nodeAPosKey = positionToKey(nodeAPos);
      int storedNodeA = -1;

      // if (positionToNodeNumber.find(nodeAPosKey) !=
      //     positionToNodeNumber.end()) {
      //   storedNodeA = positionToNodeNumber[nodeAPosKey];
      //   printf(
      //       "Always-Check: Component %s nodeA at (%.0f, %.0f) - found stored
      //       " "node %d in positionToNodeNumber\n ", comp.name.c_str(),
      //       nodeAPos.x, nodeAPos.y, storedNodeA);
      // }

      // If no stored node found, check positionToWireCarryNodeNumber map
      if (storedNodeA == -1 &&
          positionToWireCarryNodeNumber.find(nodeAPosKey) !=
              positionToWireCarryNodeNumber.end()) {
        storedNodeA = positionToWireCarryNodeNumber[nodeAPosKey];
        printf(
            "Always-Check: Component %s nodeA at (%.0f, %.0f) - found wire "
            "node %d in positionToWireCarryNodeNumber\n",
            comp.name.c_str(), nodeAPos.x, nodeAPos.y, storedNodeA);
      }

      // If we found a stored node at this position, use it
      if (storedNodeA != -1) {
        if (storedNodeA != comp.nodeA) {
          printf(
              "Sync: Component %s nodeA updated: %d -> %d (stored position "
              "node at %.0f, %.0f)\n",
              comp.name.c_str(), comp.nodeA, storedNodeA, nodeAPos.x,
              nodeAPos.y);
          comp.nodeA = storedNodeA;
          componentNodeToNumber[{i, 0}] = storedNodeA;
        }
      } else {
        // No stored node found - check wire endpoints as fallback
        int wireNodeA = -1;
        for (const auto& wire : visualWires) {
          if (wire.carriedNodeNumber < 0) continue;

          // Check wire start position
          if (!wire.points.empty()) {
            ImVec2 wireStart = wire.points.front();
            float dx = wireStart.x - nodeAPos.x;
            float dy = wireStart.y - nodeAPos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= SNAP_DISTANCE) {
              wireNodeA = wire.carriedNodeNumber;
              printf(
                  "Always-Check: Component %s nodeA (%.0f, %.0f) matches wire "
                  "start (dist %.1f) -> node %d\n",
                  comp.name.c_str(), nodeAPos.x, nodeAPos.y, dist, wireNodeA);
              break;
            }
          }

          // Check wire end position
          if (wire.points.size() > 1) {
            ImVec2 wireEnd = wire.points.back();
            float dx = wireEnd.x - nodeAPos.x;
            float dy = wireEnd.y - nodeAPos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= SNAP_DISTANCE) {
              wireNodeA = wire.carriedNodeNumber;
              printf(
                  "Always-Check: Component %s nodeA (%.0f, %.0f) matches wire "
                  "end (dist %.1f) -> node %d\n",
                  comp.name.c_str(), nodeAPos.x, nodeAPos.y, dist, wireNodeA);
              break;
            }
          }
        }

        // Update nodeA if wire node found and different from current
        if (wireNodeA != -1 && wireNodeA != comp.nodeA) {
          printf(
              "Sync: Component %s nodeA updated: %d -> %d (wire endpoint match "
              "at %.0f, %.0f)\n",
              comp.name.c_str(), comp.nodeA, wireNodeA, nodeAPos.x, nodeAPos.y);
          comp.nodeA = wireNodeA;
          componentNodeToNumber[{i, 0}] = wireNodeA;
          storeNodeAtPosition(wireNodeA, nodeAPos);
        } else if (wireNodeA == -1) {
          printf(
              "Always-Check: Component %s nodeA at (%.0f, %.0f) - no wire "
              "match, keeping node %d\n",
              comp.name.c_str(), nodeAPos.x, nodeAPos.y, comp.nodeA);
        }
      }

      // Check nodeB: FIRST check positionToNodeNumber map for existing stored
      // node
      auto nodeBPosKey = positionToKey(nodeBPos);
      int storedNodeB = -1;
      // if (positionToNodeNumber.find(nodeBPosKey) !=
      //     positionToNodeNumber.end()) {
      //   storedNodeB = positionToNodeNumber[nodeBPosKey];
      //   printf(
      //       "Always-Check: Component %s nodeB at (%.0f, %.0f) - found stored
      //       " "node %d in positionToNodeNumber\n ", comp.name.c_str(),
      //       nodeBPos.x, nodeBPos.y, storedNodeB);
      // }

      // If no stored node found, check positionToWireCarryNodeNumber map
      if (storedNodeB == -1 &&
          positionToWireCarryNodeNumber.find(nodeBPosKey) !=
              positionToWireCarryNodeNumber.end()) {
        storedNodeB = positionToWireCarryNodeNumber[nodeBPosKey];
        printf(
            "Always-Check: Component %s nodeB at (%.0f, %.0f) - found wire "
            "node %d in positionToWireCarryNodeNumber\n",
            comp.name.c_str(), nodeBPos.x, nodeBPos.y, storedNodeB);
      }

      // If we found a stored node at this position, use it
      if (storedNodeB != -1) {
        if (storedNodeB != comp.nodeB) {
          printf(
              "Sync: Component %s nodeB updated: %d -> %d (stored position "
              "node at %.0f, %.0f)\n",
              comp.name.c_str(), comp.nodeB, storedNodeB, nodeBPos.x,
              nodeBPos.y);
          comp.nodeB = storedNodeB;
          componentNodeToNumber[{i, 1}] = storedNodeB;
        }
      } else {
        // No stored node found - check wire endpoints as fallback
        int wireNodeB = -1;
        for (const auto& wire : visualWires) {
          if (wire.carriedNodeNumber < 0) continue;

          // Check wire start position
          if (!wire.points.empty()) {
            ImVec2 wireStart = wire.points.front();
            float dx = wireStart.x - nodeBPos.x;
            float dy = wireStart.y - nodeBPos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= SNAP_DISTANCE) {
              wireNodeB = wire.carriedNodeNumber;
              printf(
                  "Always-Check: Component %s nodeB (%.0f, %.0f) matches wire "
                  "start (dist %.1f) -> node %d\n",
                  comp.name.c_str(), nodeBPos.x, nodeBPos.y, dist, wireNodeB);
              break;
            }
          }

          // Check wire end position
          if (wire.points.size() > 1) {
            ImVec2 wireEnd = wire.points.back();
            float dx = wireEnd.x - nodeBPos.x;
            float dy = wireEnd.y - nodeBPos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= SNAP_DISTANCE) {
              wireNodeB = wire.carriedNodeNumber;
              printf(
                  "Always-Check: Component %s nodeB (%.0f, %.0f) matches wire "
                  "end (dist %.1f) -> node %d\n",
                  comp.name.c_str(), nodeBPos.x, nodeBPos.y, dist, wireNodeB);
              break;
            }
          }
        }

        // Update nodeB if wire node found and different from current
        if (wireNodeB != -1 && wireNodeB != comp.nodeB) {
          printf(
              "Sync: Component %s nodeB updated: %d -> %d (wire endpoint match "
              "at %.0f, %.0f)\n",
              comp.name.c_str(), comp.nodeB, wireNodeB, nodeBPos.x, nodeBPos.y);
          comp.nodeB = wireNodeB;
          componentNodeToNumber[{i, 1}] = wireNodeB;
          storeNodeAtPosition(wireNodeB, nodeBPos);
        } else if (wireNodeB == -1) {
          printf(
              "Always-Check: Component %s nodeB at (%.0f, %.0f) - no wire "
              "match, keeping node %d\n",
              comp.name.c_str(), nodeBPos.x, nodeBPos.y, comp.nodeB);
        }
      }
    }
  }

  /**
   * Removes all wire-carry nodes at a component's node positions.
   * This prevents sync from finding stale wire-carry nodes after a component
   * has been properly connected to wires.
   */
  void clearComponentWireCarryNodes(int componentIndex) {
    if (componentIndex < 0 || componentIndex >= visualComponents.size()) return;

    VisualComponent& comp = visualComponents[componentIndex];
    auto [nodeAPos, nodeBPos] = getComponentNodePositions(comp);

    auto nodeAPosKey = positionToKey(nodeAPos);
    if (positionToWireCarryNodeNumber.find(nodeAPosKey) !=
        positionToWireCarryNodeNumber.end()) {
      printf(
          "Clearing wire-carry node at component %s nodeA position (%.0f, "
          "%.0f)\n",
          comp.name.c_str(), nodeAPos.x, nodeAPos.y);
      positionToWireCarryNodeNumber.erase(nodeAPosKey);
    }

    auto nodeBPosKey = positionToKey(nodeBPos);
    if (positionToWireCarryNodeNumber.find(nodeBPosKey) !=
        positionToWireCarryNodeNumber.end()) {
      printf(
          "Clearing wire-carry node at component %s nodeB position (%.0f, "
          "%.0f)\n",
          comp.name.c_str(), nodeBPos.x, nodeBPos.y);
      positionToWireCarryNodeNumber.erase(nodeBPosKey);
    }
  }

  /**
   * Ensures all wires maintain a consistent single node number from start to
   * end. For each wire, verifies that the node number is preserved and not
   * changed. Wires NEVER change their node number - they keep whatever node
   * they were assigned. This is called after any operation that might affect
   * wires to log and verify consistency.
   */
  void ensureWireConsistency() {
    printf("=== Ensuring wire consistency ===\n");
    for (size_t i = 0; i < visualWires.size(); ++i) {
      auto& wire = visualWires[i];
      if (wire.carriedNodeNumber < 0) {
        printf("Wire %zu: No node assigned (carriedNodeNumber = -1)\n", i);
        continue;
      }

      // Log wire information
      printf("Wire %zu: Carries node %d, ", i, wire.carriedNodeNumber);

      if (!wire.points.empty()) {
        ImVec2 wireStart = wire.points.front();
        ImVec2 wireEnd = wire.points.back();
        printf("Start: (%.0f, %.0f), End: (%.0f, %.0f), Points: %zu",
               wireStart.x, wireStart.y, wireEnd.x, wireEnd.y,
               wire.points.size());

        // Verify wire endpoints are properly set
        if (wire.startComponentIndex >= 0) {
          printf(", StartComp: %d", wire.startComponentIndex);
        }
        if (wire.endComponentIndex >= 0) {
          printf(", EndComp: %d", wire.endComponentIndex);
        }
      }
      printf("\n");
    }
    printf("=== Wire consistency check complete ===\n");
  }

  /**
   * Handles SFML window events (close, resize, etc.)
   * Also passes events to ImGui for UI interaction.
   * Window resize triggers canvas grid regeneration.
   */
  void handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(window, event);  // Let ImGui handle UI events

      if (event.type == sf::Event::Closed) window.close();

      if (event.type == sf::Event::Resized) {
        window.setView(
            sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
        windowSize = ImVec2(event.size.width, event.size.height);
        // A4 canvas is fixed size - no need to regenerate grid on resize
      }
    }
  }

  /**
   * Renders the main menu bar at the top of the application.
   * Contains: File, Edit, Window, and Help menus with standard actions.
   */
  void renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New")) {
          // Launch a new instance of the circuit simulator application
          // (non-blocking)
#ifdef _WIN32
          // Use CreateProcessA on Windows for non-blocking execution
          STARTUPINFOA si = {};
          PROCESS_INFORMATION pi = {};
          CreateProcessA(nullptr, (LPSTR) "build/circuit-simulator.exe",
                         nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr,
                         nullptr, &si, &pi);
          if (pi.hProcess) CloseHandle(pi.hProcess);
          if (pi.hThread) CloseHandle(pi.hThread);
#elif __APPLE__
          // Use fork/exec on macOS for non-blocking execution
          pid_t pid = fork();
          if (pid == 0) {
            // Child process
            execlp("./build/circuit-simulator", "./build/circuit-simulator",
                   nullptr);
            _exit(1);
          }
          // Parent continues without waiting
#else
          // Linux: use fork/exec for non-blocking execution
          pid_t pid = fork();
          if (pid == 0) {
            // Child process
            execlp("./build/circuit-simulator", "./build/circuit-simulator",
                   nullptr);
            _exit(1);
          }
          // Parent continues without waiting
#endif
        }
        if (ImGui::MenuItem("Open")) {
          showLoadDialog = true;
        }
        if (ImGui::MenuItem("Save",
                            currentFilename.empty() ? nullptr : "Ctrl+S")) {
          if (!currentFilename.empty()) {
            saveCircuitToFile(currentFilename);
          } else {
            showSaveDialog = true;
          }
        }
        if (ImGui::MenuItem("Save As")) {
          showSaveDialog = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Close Window")) {
          window.close();
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo")) {
          // ...
        }
        if (ImGui::MenuItem("Redo")) {
          // ...
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Window")) {
        if (ImGui::MenuItem(isFullScreen ? "Exit Full Screen"
                                         : "Full Screen")) {
          isFullScreen = !isFullScreen;
          // toggle
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Documentation")) {
          std::string url =
              "https://version.aalto.fi/gitlab/elec-a7151-2023/"
              "circuit-simulator-3/-/blob/main/README.md";
#ifdef _WIN32
          system(("start " + url).c_str());
#elif __APPLE__
          system(("open " + url).c_str());
#else
          system(("xdg-open " + url).c_str());
#endif
        }

        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  /**
   * Renders the main toolbar below the menu bar.
   * Contains: Settings popup, Play button, Zoom controls, and component
   * buttons. Settings popup includes measurement options and grid spacing
   * control.
   */
  void renderSimpleToolbar() {
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, 70));
    ImGui::Begin("Toolbar", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoTitleBar);

    // Set uniform button dimensions
    float buttonHeight = 50.0f;  // Full toolbar height
    float buttonWidth = 80.0f;   // Fixed width for all buttons

    // Add spacing between buttons and remove button rounding
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f);  // No button radius

    // Light blue color for all toolbar buttons
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.4f, 0.7f, 1.0f, 1.0f));  // Light blue
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.5f, 0.8f, 1.0f, 1.0f));  // Lighter blue on hover
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.3f, 0.6f, 0.9f, 1.0f));  // Darker blue when clicked

    if (ImGui::Button("Graph", ImVec2(buttonWidth, buttonHeight))) {
      ImGui::OpenPopup("GraphPopup");
    }
    if (ImGui::BeginPopup("GraphPopup")) {
      ImGui::MenuItem("Time-domain: Node Voltages", nullptr,
                      &showGraphForNodeVolt);
      ImGui::MenuItem("Time-domain: Voltage Source Currents", nullptr,
                      &showGraphForVoltageSourceCurrent);
      ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Test", ImVec2(buttonWidth, buttonHeight))) {
      // Load a simple test circuit for quick testing
      const std::string testNetlist = R"(
      * Simple Test Circuit - Voltage Divider
      V1 1 0 12
      R1 1 2 1k
      R2 2 0 2k
      R3 2 0 2k
      )";
      circuit.parseNetlist(testNetlist);
      printf("Test circuit loaded: V1=10V, R1=1kΩ, R2=2kΩ\n");
    }

    ImGui::SameLine();
    if (ImGui::Button("Run DC", ImVec2(buttonWidth, buttonHeight))) {
      // Check if circuit has components before running
      if (circuit.getComponents().empty()) {
        printf(
            "ERROR: No components in circuit! Use 'Load Test' or add "
            "components via GUI.\n");
        return;
      }

      isDCMode = true;   // Set DC flag
      isACMode = false;  // Clear AC flag

      solution_ = MNASolver::solveDC(circuit);

      /**
       * Display calculated currents for all passive components (resistors,
       * inductors, capacitors) immediately after running the simulation. This
       * provides instant feedback in the toolbar area. Skips voltage and
       * current sources, computes current using Ohm's law, and shows the
       * result.
       */
      for (const auto& comp : circuit.getComponents()) {
        std::string name = comp->getName();
        if (!name.empty() && name[0] != 'V' && name[0] != 'I') {
          int comNodeA = comp->getNodes()[0];
          int comNodeB = comp->getNodes()[1];
          double vA = 0.0, vB = 0.0;
          if (comNodeA > 0 && comNodeA <= solution_.size())
            vA = solution_(comNodeA - 1);
          if (comNodeB > 0 && comNodeB <= solution_.size())
            vB = solution_(comNodeB - 1);
          double compCurrent = 0.0;
          if (comp->getValue() != 0.0) {
            compCurrent = (vA - vB) / comp->getValue();
          }
          ImGui::Text("%s_I_rms: %.4f A", name.c_str(), compCurrent);
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Run AC", ImVec2(buttonWidth, buttonHeight))) {
      // Check if circuit has components before running
      if (circuit.getComponents().empty()) {
        printf(
            "ERROR: No components in circuit! Use 'Load Test' or add "
            "components via GUI.\n");
        return;
      }

      isDCMode = false;  // Clear DC flag
      isACMode = true;   // Set AC flag

      // CONSTANT FREQUENCY FOR AC ANALYSIS
      double f = 50.0;
      // M_PI = 3.141592653589793 in cmath header
      double omega = 2.0 * M_PI * f;

      acSolution_ = MNASolver::solveAC(circuit, omega);

      /**
       * Display calculated currents for all passive components (resistors,
       * inductors, capacitors) immediately after running the simulation.
       * This provides instant feedback in the toolbar area. Skips voltage
       * and current sources, computes current using Ohm's law, and shows
       * the result.
       */
      for (const auto& comp : circuit.getComponents()) {
        // Only print for non-voltage/current sources
        std::string name = comp->getName();
        if (!name.empty() && name[0] != 'V' && name[0] != 'I') {
          int comNodeA = comp->getNodes()[0];
          int comNodeB = comp->getNodes()[1];

          // Bounds check before accessing acSolution_
          std::complex<double> solu_A = 0.0;
          std::complex<double> solu_B = 0.0;
          if (comNodeA > 0 && comNodeA <= acSolution_.size())
            solu_A = acSolution_(comNodeA - 1);

          if (comNodeB > 0 && comNodeB <= acSolution_.size())
            solu_B = acSolution_(comNodeB - 1);

          // RMS magnitude
          double magA = std::abs(solu_A) / std::sqrt(2.0);
          double magB = std::abs(solu_B) / std::sqrt(2.0);

          double vA = (comNodeA > 0) ? magA : 0.0;
          double vB = (comNodeB > 0) ? magB : 0.0;

          double compCurrent = 0.0;
          if (comp->getValue() != 0.0) {
            compCurrent = (vA - vB) / comp->getValue();
          }
          ImGui::Text("%s_I_rms: %.4f A", name.c_str(), compCurrent);
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(buttonWidth, buttonHeight))) {
      // Clear all visual components
      visualComponents.clear();
      visualWires.clear();
      selectedComponents.clear();
      selectedWires.clear();
      selectedComponentIndex = -1;

      // Clear all node connections and mappings
      nodeConnections.clear();
      componentNodeToNumber.clear();
      positionToNodeNumber.clear();
      positionToWireCarryNodeNumber.clear();
      nextNodeNumber = 1;

      // Clear the circuit
      circuit = Circuit();

      // Clear sprites
      componentSprites.clear();

      // Clear file reference
      currentFilename = "";

      printf("Canvas cleared - fresh start\n");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    renderComponentButtons();

    // // Show current selection count
    // if (!selectedComponents.empty() || !selectedWires.empty()) {
    //   ImGui::Separator();
    //   ImGui::Text("Selected: %d components, %d wires",
    //               (int)selectedComponents.size(), (int)selectedWires.size());
    // }

    // Restore original colors and spacing
    ImGui::PopStyleColor(3);  // Pop button colors
    ImGui::PopStyleVar(2);    // Pop spacing and rounding

    ImGui::End();
  }

  /**
   * Renders component creation buttons and their popup dialogs.
   * Available components: Resistor, Inductor, Capacitor, Wire, Voltage Source,
   * Ground. Each button opens a popup for entering component parameters (name,
   * nodes, value).
   */
  void renderComponentButtons() {
    // Use same button dimensions as toolbar
    float buttonHeight = 50.0f;
    float buttonWidth = 80.0f;  // Fixed width for all buttons

    struct ComponentButton {
      std::string name;
      std::string label;
      bool useImage;
      std::string popupName;
    };

    std::vector<ComponentButton> components = {
        {"resistor", "Resistor", true, "ResistorPopup"},
        {"inductor", "Inductor", true, "InductorPopup"},
        {"capacitor", "Capacitor", true, "CapacitorPopup"},
        {"voltage_source", "Voltage\nSource", true, "VoltagePopup"},
        {"ac_voltage_source", "AC\nVoltage\nSource", true, "ACVoltagePopup"},
        {"dc_current_source", "DC\nCurrent\nSource", true, "DCCurrentPopup"},
        {"ac_current_source", "AC\nCurrent\nSource", true, "ACCurrentPopup"},
        {"ground", "GND", false, "GNDPopup"},
        {"wire", "W", false, ""}};  // Wire uses text button only};

    for (size_t i = 0; i < components.size(); ++i) {
      const auto& c = components[i];
      bool clicked = false;

      if (c.useImage &&
          componentTextures.find(c.name) != componentTextures.end()) {
        ImGui::PushID(c.name.c_str());
        clicked = ImGui::ImageButton(
            componentTextures[c.name],
            sf::Vector2f(buttonWidth - 10, buttonHeight - 8));
        ImGui::PopID();
      } else {
        clicked =
            ImGui::Button(c.label.c_str(), ImVec2(buttonWidth, buttonHeight));
      }

      if (clicked) {
        if (c.name == "wire") {
          wireMode = true;
          isDrawingWire = false;
        } else {
          wireMode = false;
          if (!c.popupName.empty()) ImGui::OpenPopup(c.popupName.c_str());
        }
      }

      if (i < components.size() - 1) ImGui::SameLine();
    }

    // Component creation popups

    if (ImGui::BeginPopup("ResistorPopup")) {
      ImGui::Text("Add Resistor");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "1k";  // Default 1k ohm
      static int lastResistorCount = -1;

      // Dynamically generate next resistor name if needed
      int resistorCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "resistor") {
          // comp.name.c_str() + 1 moves the pointer one character
          // forward, skipping the first character ('R').
          // So, if comp.name is "R2", comp.name.c_str() + 1 points to "2".
          int num = atoi(comp.name.c_str() + 1);
          if (num > resistorCount) resistorCount = num;
        }
      }
      if (lastResistorCount != resistorCount) {
        snprintf(name, sizeof(name), "R%d", resistorCount + 1);
        lastResistorCount = resistorCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      // Force name to uppercase
      // For loop will go through each character in the name string
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Value (ohm)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual resistor %s with value %s\n", name, valueStr);

        // Add visual component to A4 canvas center
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        // Snap initial position to grid
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent resistor(name, "resistor", gridCenter);
        resistor.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(resistor);

        // Check if nodeA position has a wire-carried node number
        auto nodeAPosKey = positionToKey(nodeAPos);
        int nodeAFromWire = -1;
        if (positionToWireCarryNodeNumber.find(nodeAPosKey) !=
            positionToWireCarryNodeNumber.end()) {
          nodeAFromWire = positionToWireCarryNodeNumber[nodeAPosKey];
          printf(
              "Resistor %s nodeA at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }

        // Check if nodeB position has a wire-carried node number
        auto nodeBPosKey = positionToKey(nodeBPos);
        int nodeBFromWire = -1;
        if (positionToWireCarryNodeNumber.find(nodeBPosKey) !=
            positionToWireCarryNodeNumber.end()) {
          nodeBFromWire = positionToWireCarryNodeNumber[nodeBPosKey];
          printf(
              "Resistor %s nodeB at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        // Assign node numbers: use wire nodes if found, otherwise create new
        resistor.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        resistor.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "Resistor %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, resistor.nodeA, resistor.nodeB, nodeAFromWire, nodeBFromWire);

        resistor.color = IM_COL32(101, 67, 33, 255);
        resistor.size = ImVec2(90, 50);
        resistor.labelOffset =
            getDefaultLabelOffset("resistor", resistor.rotation);
        visualComponents.push_back(resistor);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("InductorPopup")) {
      ImGui::Text("Add Inductor");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "1m";
      static int lastInductorCount = -1;

      int inductorCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "inductor") {
          int num = atoi(comp.name.c_str() + 1);
          if (num > inductorCount) inductorCount = num;
        }
      }
      if (lastInductorCount != inductorCount) {
        snprintf(name, sizeof(name), "L%d", inductorCount + 1);
        lastInductorCount = inductorCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Value (H)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual inductor %s with value %s\n", name, valueStr);

        // Add visual component to A4 canvas center, snapped to grid
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent inductor(name, "inductor", gridCenter);
        inductor.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(inductor);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "Inductor %s nodeA at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "Inductor %s nodeB at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        inductor.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        inductor.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "Inductor %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, inductor.nodeA, inductor.nodeB, nodeAFromWire, nodeBFromWire);

        inductor.color = IM_COL32(30, 144, 255, 255);
        inductor.size = ImVec2(90, 50);
        inductor.labelOffset =
            getDefaultLabelOffset("inductor", inductor.rotation);
        visualComponents.push_back(inductor);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("CapacitorPopup")) {
      ImGui::Text("Add Capacitor");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "1u";
      static int lastCapacitorCount = -1;

      int capacitorCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "capacitor") {
          int num = atoi(comp.name.c_str() + 1);
          if (num > capacitorCount) capacitorCount = num;
        }
      }
      if (lastCapacitorCount != capacitorCount) {
        snprintf(name, sizeof(name), "C%d", capacitorCount + 1);
        lastCapacitorCount = capacitorCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Value (F)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual capacitor %s with value %s\n", name, valueStr);
        // Add visual component to canvas, snapped to grid
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent capacitor(name, "capacitor", gridCenter);
        capacitor.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(capacitor);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "Capacitor %s nodeA at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "Capacitor %s nodeB at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        capacitor.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        capacitor.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "Capacitor %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, capacitor.nodeA, capacitor.nodeB, nodeAFromWire,
            nodeBFromWire);

        capacitor.color = IM_COL32(0, 0, 0, 255);
        capacitor.size =
            ImVec2(90, 50);  // 90x50 size - nodes align with 30px grid
        capacitor.labelOffset =
            getDefaultLabelOffset("capacitor", capacitor.rotation);
        visualComponents.push_back(capacitor);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("VoltagePopup")) {
      ImGui::Text("Add Voltage Source");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "5";
      static int lastVoltageCount = -1;

      int voltageCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "voltage_source") {
          int num = atoi(comp.name.c_str() + 1);
          if (num > voltageCount) voltageCount = num;
        }
      }
      if (lastVoltageCount != voltageCount) {
        snprintf(name, sizeof(name), "VDC%d", voltageCount + 1);
        lastVoltageCount = voltageCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Value (V)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual voltage source %s with value %s\n", name,
               valueStr);

        // Add visual component to canvas, snapped to grid
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent voltageSource(name, "voltage_source", gridCenter);
        voltageSource.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(voltageSource);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "VoltageSource %s nodeA at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "VoltageSource %s nodeB at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        voltageSource.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        voltageSource.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "VoltageSource %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, voltageSource.nodeA, voltageSource.nodeB, nodeAFromWire,
            nodeBFromWire);

        voltageSource.color = IM_COL32(220, 20, 60, 255);
        voltageSource.size =
            ImVec2(90, 60);  // 90x60 size - nodes align with 30px grid
        voltageSource.rotation = 90.0f;
        voltageSource.labelOffset =
            getDefaultLabelOffset("voltage_source", voltageSource.rotation);
        visualComponents.push_back(voltageSource);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("ACVoltagePopup")) {
      ImGui::Text("Add AC Voltage Source");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "230";
      static int lastACVoltageCount = -1;

      int acVoltageCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "ac_voltage_source") {
          int num = atoi(comp.name.c_str() + 2);  // Skip "VAC" prefix
          if (num > acVoltageCount) acVoltageCount = num;
        }
      }
      if (lastACVoltageCount != acVoltageCount) {
        snprintf(name, sizeof(name), "VAC%d", acVoltageCount + 1);
        lastACVoltageCount = acVoltageCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Peak (V)", valueStr, 32);

      ImGui::Text("Default:");
      ImGui::Text("DC offset = 0.0V");
      ImGui::Text("Frequency = 50Hz");

      if (ImGui::Button("Add")) {
        printf("Created visual AC voltage source %s with amplitude %s\n", name,
               valueStr);

        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent acVoltageSource(name, "ac_voltage_source", gridCenter);
        acVoltageSource.value = parseValue(valueStr);

        auto [nodeAPos, nodeBPos] = getComponentNodePositions(acVoltageSource);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "ACVoltageSource %s nodeA at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "ACVoltageSource %s nodeB at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        acVoltageSource.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        acVoltageSource.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "ACVoltageSource %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, acVoltageSource.nodeA, acVoltageSource.nodeB, nodeAFromWire,
            nodeBFromWire);

        acVoltageSource.color = IM_COL32(220, 100, 60, 255);
        acVoltageSource.size = ImVec2(90, 60);
        acVoltageSource.rotation = 90.0f;
        acVoltageSource.labelOffset = getDefaultLabelOffset(
            "ac_voltage_source", acVoltageSource.rotation);
        visualComponents.push_back(acVoltageSource);

        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("DCCurrentPopup")) {
      ImGui::Text("Add DC Current Source");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "10";
      static int lastCurrentCount = -1;

      int currentCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "dc_current_source") {
          int num = atoi(comp.name.c_str() + 1);
          if (num > currentCount) currentCount = num;
        }
      }
      if (lastCurrentCount != currentCount) {
        snprintf(name, sizeof(name), "IDC%d", currentCount + 1);
        lastCurrentCount = currentCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }

      ImGui::InputText("Value (A)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual dc current source %s with value %s\n", name,
               valueStr);

        // Add visual component to canvas, snapped to grid
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent dcCurrentSource(name, "dc_current_source", gridCenter);
        dcCurrentSource.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(dcCurrentSource);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "DC CurrentSource %s nodeA at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "DC CurrentSource %s nodeB at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        dcCurrentSource.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        dcCurrentSource.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "DC CurrentSource %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, dcCurrentSource.nodeA, dcCurrentSource.nodeB, nodeAFromWire,
            nodeBFromWire);

        dcCurrentSource.color = IM_COL32(34, 139, 34, 255);
        dcCurrentSource.size =
            ImVec2(90, 60);  // 90x60 size - nodes align with 30px grid
        dcCurrentSource.rotation = 90.0f;
        dcCurrentSource.labelOffset = getDefaultLabelOffset(
            "dc_current_source", dcCurrentSource.rotation);
        visualComponents.push_back(dcCurrentSource);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("ACCurrentPopup")) {
      ImGui::Text("Add AC Current Source");
      static char name[32] = "";
      static int nodeA = 0, nodeB = 0;
      static char valueStr[32] = "10";
      static int lastCurrentCount = -1;

      int currentCount = 0;
      for (const auto& comp : visualComponents) {
        if (comp.type == "ac_current_source") {
          int num = atoi(comp.name.c_str() + 1);
          if (num > currentCount) currentCount = num;
        }
      }
      if (lastCurrentCount != currentCount) {
        snprintf(name, sizeof(name), "IAC%d", currentCount + 1);
        lastCurrentCount = currentCount;
      }

      ImGui::InputText("Name", name, 32);
      // Force name to uppercase
      for (int i = 0; name[i] != '\0'; ++i) {
        name[i] = toupper(name[i]);
      }
      ImGui::InputText("Value (A)", valueStr, 32);

      if (ImGui::Button("Add")) {
        // Circuit will be built when connections are made
        printf("Created visual ac current source %s with value %s\n", name,
               valueStr);

        // Add visual component to canvas, snapped to grid
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent acCurrentSource(name, "ac_current_source", gridCenter);
        acCurrentSource.value = parseValue(valueStr);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(acCurrentSource);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);
        int nodeBFromWire = getWireCarryNodeAtPosition(nodeBPos);

        if (nodeAFromWire != -1) {
          printf(
              "acCurrentSource %s nodeA at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }
        if (nodeBFromWire != -1) {
          printf(
              "acCurrentSource %s nodeB at (%.0f, %.0f) - found existing wire "
              "node %d\n",
              name, nodeBPos.x, nodeBPos.y, nodeBFromWire);
        }

        acCurrentSource.nodeA =
            (nodeAFromWire != -1) ? nodeAFromWire : nextNodeNumber++;
        acCurrentSource.nodeB =
            (nodeBFromWire != -1) ? nodeBFromWire : nextNodeNumber++;

        printf(
            "acCurrentSource %s assigned nodeA=%d, nodeB=%d (from wire: %d, "
            "%d)\n",
            name, acCurrentSource.nodeA, acCurrentSource.nodeB, nodeAFromWire,
            nodeBFromWire);

        acCurrentSource.color = IM_COL32(34, 139, 34, 255);
        acCurrentSource.size =
            ImVec2(90, 60);  // 90x60 size - nodes align with 30px grid
        acCurrentSource.rotation = 90.0f;
        acCurrentSource.labelOffset = getDefaultLabelOffset(
            "ac_current_source", acCurrentSource.rotation);
        visualComponents.push_back(acCurrentSource);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node positions in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);
        storeNodeAtPosition(comp.nodeB, nodeBPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("GNDPopup")) {
      ImGui::Text("Add Ground");
      static char name[32] = "GND";
      ImGui::InputText("Name", name, 32);

      if (ImGui::Button("Add")) {
        ImVec2 canvasCenter = ImVec2(A4_WIDTH / 2, A4_HEIGHT / 2);
        int snapX = ((int)(canvasCenter.x + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        int snapY = ((int)(canvasCenter.y + gridSpacing / 2) / gridSpacing) *
                    gridSpacing;
        ImVec2 gridCenter = ImVec2(snapX, snapY);
        VisualComponent ground(name, "ground", gridCenter);

        // Get node positions BEFORE creating component to check for
        // positionToWireCarryNodeNumber
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(ground);

        int nodeAFromWire = getWireCarryNodeAtPosition(nodeAPos);

        if (nodeAFromWire != -1) {
          printf(
              "Ground %s nodeA at (%.0f, %.0f) - found existing wire node "
              "%d\n",
              name, nodeAPos.x, nodeAPos.y, nodeAFromWire);
        }

        // Ground always has node 0 (ground reference) - don't override with
        // wire node
        ground.nodeA = 0;
        ground.nodeB = 0;  // Ground only has one connection

        printf("Ground %s assigned nodeA=0, nodeB=0 (ground reference)\n",
               name);

        ground.color = IM_COL32(64, 64, 64, 255);
        ground.size = ImVec2(90, 60);
        ground.labelOffset = getDefaultLabelOffset("ground", ground.rotation);
        visualComponents.push_back(ground);

        // Get the newly added component
        int compIdx = visualComponents.size() - 1;
        VisualComponent& comp = visualComponents[compIdx];

        // Store node position in positionToNodeNumber map
        storeNodeAtPosition(comp.nodeA, nodeAPos);

        initializeComponentNodes(compIdx);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  /**
   * Renders the component edit popup when right-clicking on a component
   * This must be called from the main update() function, not from
   * renderSimpleToolbar()
   * Uses BeginPopup (non-modal) instead of BeginPopupModal so it doesn't block
   * menu
   */
  void renderComponentEditPopup() {
    static char compNewName[64] = "";
    static char compNewValue[64] = "";
    // Check if we should open the popup from canvas right-click
    if (openEditPopupFromCanvas) {
      ImGui::OpenPopup("Component Edit");
      openEditPopupFromCanvas = false;
      isEditPopupCurrentlyOpen = true;

      // Initialize the edit fields with current component values
      if (selectedComponentIndex >= 0 &&
          selectedComponentIndex < visualComponents.size()) {
        VisualComponent& comp = visualComponents[selectedComponentIndex];
        strncpy(compNewName, comp.name.c_str(), sizeof(compNewName) - 1);
        snprintf(compNewValue, sizeof(compNewValue), "%f", comp.value);
      }
    }

    if (ImGui::BeginPopup("Component Edit",
                          ImGuiWindowFlags_AlwaysAutoResize)) {
      isEditPopupCurrentlyOpen = true;  // Popup is open this frame

      if (selectedComponentIndex >= 0 &&
          selectedComponentIndex < visualComponents.size()) {
        VisualComponent& comp = visualComponents[selectedComponentIndex];

        ImGui::Text("Edit Component: %s", comp.name.c_str());
        ImGui::Separator();

        // Component type (read-only)
        ImGui::Text("Type: %s", comp.type.c_str());

        // Component name - updates live in the buffer
        ImGui::InputText("Name##comp", compNewName, sizeof(compNewName));

        // Component value - updates live in the buffer
        ImGui::InputText("Value##comp", compNewValue, sizeof(compNewValue));

        ImGui::Separator();

        // Buttons - properly scoped
        if (ImGui::Button("Update", ImVec2(120, 0))) {
          // Update component properties
          comp.name = compNewName;
          comp.value = parseValue(compNewValue);

          const auto& components = circuit.getComponents();
          for (const auto& c : components) {
            if (c->getName() == comp.name) {
              c->setName(compNewName);
              c->setValue(parseValue(compNewValue));
              break;
            }
          }
          updateCircuitComponentNodes();
          ImGui::CloseCurrentPopup();
          isEditPopupCurrentlyOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
          isEditPopupCurrentlyOpen = false;
        }
        ImGui::EndPopup();
      } else {
        isEditPopupCurrentlyOpen = false;  // Popup is not open
      }
    }
  }
  /**
   * Renders the main canvas area as a fixed A4-sized sheet with scrolling.
   * Features:
   * - Fixed A4 size (1587x1123 pixels) that doesn't change with window resize
   * - Grid background for component alignment
   * - Scrollbars when window is smaller than A4 canvas
   * - Draggable components with rotation support (Ctrl+R)
   * - Interactive wire drawing system with anchor snapping
   * - Real-time wire preview with axis locking
   * - Component leg visualization and connection points
   */
  void renderCanvas() {
    // Set canvas container window position and size (no component bar, starts
    // from left edge)
    ImVec2 canvasWindowPos = ImVec2(0, ImGui::GetFrameHeight() + 65);
    ImVec2 canvasWindowSize =
        ImVec2(windowSize.x - 200, windowSize.y - ImGui::GetFrameHeight() - 65);

    ImGui::SetNextWindowPos(canvasWindowPos);
    ImGui::SetNextWindowSize(canvasWindowSize);
    ImGui::Begin("##CanvasContainer", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoTitleBar);

    ImGui::BeginChild("##A4Canvas", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Set the content size for both horizontal and vertical scrolling
    float extendedWidth = A4_WIDTH * 3.0f;
    float extendedHeight = A4_HEIGHT * 2.0f;
    ImGui::InvisibleButton("##canvasArea",
                           ImVec2(extendedWidth, extendedHeight));

    // Reset cursor position after setting content size
    ImGui::SetCursorScreenPos(canvasPos);

    // Draw canvas background and grid
    drawCanvasBackground();

    // Draw and handle visual components
    drawAndHandleVisualComponents(canvasPos, drawList);

    // Draw all completed wires
    drawVisualWires(drawList, canvasPos);

    // Wire mode with snapping and drawing functionality
    if (wireMode) {
      ImVec2 mousePos = ImGui::GetMousePos();
      // Extended canvas area for mouse interaction
      float extendedWidth = A4_WIDTH * 3.0f;
      float extendedHeight = A4_HEIGHT * 2.0f;
      bool mouseInCanvas = (mousePos.x >= canvasPos.x &&
                            mousePos.x <= canvasPos.x + extendedWidth &&
                            mousePos.y >= canvasPos.y &&
                            mousePos.y <= canvasPos.y + extendedHeight);

      if (mouseInCanvas) {
        // Hide system cursor and show custom crosshair
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        // Find closest component node for snapping
        auto [componentIndex, nodeIndex] =
            findClosestComponentNode(mousePos, canvasPos, 15.0f);
        ImVec2 cursorPos = mousePos;

        // Handle node snapping
        if (componentIndex != -1) {
          auto [nodeA, nodeB] =
              getComponentNodePositions(visualComponents[componentIndex]);
          nodeA = ImVec2(canvasPos.x + nodeA.x, canvasPos.y + nodeA.y);
          nodeB = ImVec2(canvasPos.x + nodeB.x, canvasPos.y + nodeB.y);
          cursorPos = (nodeIndex == 0) ? nodeA : nodeB;

          // Draw highlighted node
          drawList->AddCircleFilled(cursorPos, 8.0f,
                                    IM_COL32(255, 255, 0, 200));
          drawList->AddCircle(cursorPos, 8.0f, IM_COL32(255, 0, 0, 255), 0,
                              2.0f);
        }

        // Draw crosshair cursor
        drawList->AddLine(ImVec2(cursorPos.x - 12, cursorPos.y),
                          ImVec2(cursorPos.x + 12, cursorPos.y),
                          IM_COL32(255, 0, 0, 255), 2.5f);
        drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y - 12),
                          ImVec2(cursorPos.x, cursorPos.y + 12),
                          IM_COL32(255, 0, 0, 255), 2.5f);

        // Start wire on first click
        // But only if mouse is not over the graph window
        bool isOverGraphWindow = false;
        ImGuiWindow* graphWindow = ImGui::FindWindowByName("AC Analysis Graph");
        if (graphWindow != nullptr) {
          isOverGraphWindow =
              ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !isOverGraphWindow) {
          if (!isDrawingWire) {
            // Start new wire
            currentWire.points.clear();
            currentWire.carriedNodeNumber = -1;

            ImVec2 startPoint;

            if (componentIndex != -1) {
              // Wire starting on a component node - use exact component node
              // position
              auto [nodeA, nodeB] =
                  getComponentNodePositions(visualComponents[componentIndex]);
              // Get the actual absolute position (in canvas/world
              // coordinates)
              startPoint = (nodeIndex == 0) ? nodeA : nodeB;
              currentWire.startComponentIndex = componentIndex;
              currentWire.startNodeIndex = nodeIndex;
              printf(
                  "\nWire started on component %d, node %d at position "
                  "(%.1f, "
                  "%.1f)\n",
                  componentIndex, nodeIndex, startPoint.x, startPoint.y);
            } else {
              // Convert to canvas-relative coordinates for grid snapping
              ImVec2 canvasRelativePos =
                  ImVec2(cursorPos.x - canvasPos.x, cursorPos.y - canvasPos.y);
              ImVec2 snappedPos = snapToGrid(canvasRelativePos);
              // Keep startPoint in CANVAS/WORLD coordinates (not screen)
              // This matches how component node positions are stored
              startPoint = snappedPos;
              currentWire.startComponentIndex = -1;
              currentWire.startNodeIndex = -1;
            }

            currentWire.points.push_back(startPoint);
            wireCurrentEnd = startPoint;
            isDrawingWire = true;

            // Search for node number at wire start position
            int nodeAtStart = getNodeNumberAtPosition(startPoint);
            if (nodeAtStart != -1) {
              // Found a node at start position - wire will carry this node
              currentWire.carriedNodeNumber = nodeAtStart;
              printf(
                  "Wire started at position (%.1f, %.1f) - carrying node "
                  "%d\n",
                  startPoint.x, startPoint.y, nodeAtStart);
            } else {
              printf(
                  "Wire started at position (%.1f, %.1f) - no node found at "
                  "start\n",
                  startPoint.x, startPoint.y);
            }
          } else {
            // Add new point to existing wire using constrained position
            currentWire.points.push_back(wireCurrentEnd);
          }
        }

        // Draw current wire while drawing
        if (isDrawingWire) {
          // Constrain wire to 90-degree angles (horizontal or vertical only)
          // and snap to grid
          ImVec2 constrainedEnd = cursorPos;

          // If not snapped to a component, snap cursor to grid first
          if (componentIndex == -1) {
            ImVec2 canvasRelativePos =
                ImVec2(cursorPos.x - canvasPos.x, cursorPos.y - canvasPos.y);
            ImVec2 snappedPos = snapToGrid(canvasRelativePos);
            // Keep in CANVAS/WORLD coordinates (not screen)
            constrainedEnd = snappedPos;
          } else {
            // When snapped to component, convert to canvas coordinates
            constrainedEnd =
                ImVec2(cursorPos.x - canvasPos.x, cursorPos.y - canvasPos.y);
          }

          if (!currentWire.points.empty()) {
            ImVec2 lastPoint = currentWire.points.back();
            float deltaX = abs(constrainedEnd.x - lastPoint.x);
            float deltaY = abs(constrainedEnd.y - lastPoint.y);

            // Choose direction with larger movement
            if (deltaX > deltaY) {
              // Horizontal movement - lock Y to last point position
              constrainedEnd.y = lastPoint.y;
            } else {
              // Vertical movement - lock X to last point position
              constrainedEnd.x = lastPoint.x;
            }

            // Snap the constrained end to grid (unless it's a component
            // connection)
            if (componentIndex == -1) {
              ImVec2 canvasRelativeEnd =
                  ImVec2(constrainedEnd.x, constrainedEnd.y);
              ImVec2 snappedEnd = snapToGrid(canvasRelativeEnd);
              // Keep in CANVAS/WORLD coordinates
              constrainedEnd = snappedEnd;
            }
          }
          wireCurrentEnd = constrainedEnd;

          // Draw existing wire segments in SCREEN coordinates
          for (size_t i = 0; i < currentWire.points.size() - 1; i++) {
            ImVec2 p1 = ImVec2(currentWire.points[i].x + canvasPos.x,
                               currentWire.points[i].y + canvasPos.y);
            ImVec2 p2 = ImVec2(currentWire.points[i + 1].x + canvasPos.x,
                               currentWire.points[i + 1].y + canvasPos.y);
            drawList->AddLine(p1, p2, IM_COL32(255, 0, 0, 255), 3.0f);
          }

          // Draw line from last point to constrained cursor position in
          // SCREEN coordinates
          if (!currentWire.points.empty()) {
            ImVec2 lastPointScreen =
                ImVec2(currentWire.points.back().x + canvasPos.x,
                       currentWire.points.back().y + canvasPos.y);
            ImVec2 currentEndScreen = ImVec2(wireCurrentEnd.x + canvasPos.x,
                                             wireCurrentEnd.y + canvasPos.y);
            drawList->AddLine(lastPointScreen, currentEndScreen,
                              IM_COL32(255, 0, 0, 255), 3.0f);
          }

          // Draw points in SCREEN coordinates
          for (const ImVec2& point : currentWire.points) {
            ImVec2 screenPoint =
                ImVec2(point.x + canvasPos.x, point.y + canvasPos.y);
            drawList->AddCircleFilled(screenPoint, 4.0f,
                                      IM_COL32(0, 255, 0, 255));
          }
        }

        // End wire on ESC
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && isDrawingWire) {
          // auto-connection
          currentWire.endComponentIndex = -1;
          currentWire.endNodeIndex = -1;

          if (currentWire.points.size() >= 2) {
            ImVec2 wireEndPoint = currentWire.points.back();
            auto [endComponentIndex, endNodeIndex] = findClosestComponentNode(
                ImVec2(wireEndPoint.x + canvasPos.x,
                       wireEndPoint.y + canvasPos.y),
                canvasPos, 15.0f);  // 15px snap radius, convert to screen
                                    // coords for search

            if (endComponentIndex != -1) {
              // Wire endpoint is close to a component node - auto-connect
              currentWire.endComponentIndex = endComponentIndex;
              currentWire.endNodeIndex = endNodeIndex;

              // Update the wire endpoint to exact component node position (in
              // canvas coords)
              auto [nodeA, nodeB] = getComponentNodePositions(
                  visualComponents[endComponentIndex]);
              ImVec2 exactNodePos = (endNodeIndex == 0) ? nodeA : nodeB;
              currentWire.points.back() = exactNodePos;

              printf("Wire endpoint auto-connected to component %d, node %d\n",
                     endComponentIndex, endNodeIndex);
            }
          }

          // Add completed wire to collection if it has at least 2 points
          if (currentWire.points.size() >= 2) {
            // Debug: Show connection info before saving
            printf(
                "Wire completed with ESC - %zu points, start: (%.1f, %.1f) "
                "end: (%.1f, %.1f), startComp: %d, startNode: %d, endComp: "
                "%d, "
                "endNode: %d, carriedNode: %d\n",
                currentWire.points.size(), currentWire.points[0].x,
                currentWire.points[0].y, currentWire.points.back().x,
                currentWire.points.back().y, currentWire.startComponentIndex,
                currentWire.startNodeIndex, currentWire.endComponentIndex,
                currentWire.endNodeIndex, currentWire.carriedNodeNumber);

            // DEBUG: Print map contents at wire start position
            ImVec2 wireStartPoint = currentWire.points.front();
            auto startKey = positionToKey(wireStartPoint);
            printf("DEBUG MAPS at wire start (%.0f, %.0f):\n", wireStartPoint.x,
                   wireStartPoint.y);
            if (positionToNodeNumber.find(startKey) !=
                positionToNodeNumber.end()) {
              printf("  positionToNodeNumber[%d,%d] = %d\n", startKey.first,
                     startKey.second, positionToNodeNumber[startKey]);
            }
            if (positionToWireCarryNodeNumber.find(startKey) !=
                positionToWireCarryNodeNumber.end()) {
              printf("  positionToWireCarryNodeNumber[%d,%d] = %d\n",
                     startKey.first, startKey.second,
                     positionToWireCarryNodeNumber[startKey]);
            }
            printf("  All positionToWireCarryNodeNumber entries: ");
            for (auto& entry : positionToWireCarryNodeNumber) {
              printf("[%d,%d]=%d ", entry.first.first, entry.first.second,
                     entry.second);
            }
            printf("\n");  // Handle wire-carried node transfer if wire is
                           // carrying a node
            if (currentWire.carriedNodeNumber != -1) {
              ImVec2 wireStartPoint = currentWire.points.front();
              ImVec2 wireEndPoint = currentWire.points.back();

              // Store wire start position with carried node number
              storeWireCarryNodeAtPosition(currentWire.carriedNodeNumber,
                                           wireStartPoint);
              printf(
                  "Stored wire start position (%.1f, %.1f) with carried node "
                  "%d in positionToWireCarryNodeNumber\n",
                  wireStartPoint.x, wireStartPoint.y,
                  currentWire.carriedNodeNumber);

              // IMPORTANT: Keep the wire's carried node number constant
              // throughout Don't check what's at the end - just store the
              // same node there too This ensures wire continuity - it carries
              // the SAME node from start to end
              storeWireCarryNodeAtPosition(currentWire.carriedNodeNumber,
                                           wireEndPoint);
              printf(
                  "Stored wire end position (%.1f, %.1f) with carried node "
                  "%d in positionToWireCarryNodeNumber\n",
                  wireEndPoint.x, wireEndPoint.y,
                  currentWire.carriedNodeNumber);
            }

            visualWires.push_back(currentWire);

            // Store wire-carried node at ALL positions along the wire path
            if (currentWire.carriedNodeNumber != -1 &&
                !currentWire.points.empty()) {
              // Store at every point in the wire path
              for (const auto& point : currentWire.points) {
                storeWireCarryNodeAtPosition(currentWire.carriedNodeNumber,
                                             point);
              }

              ImVec2 wireStartPoint = currentWire.points.front();
              ImVec2 wireEndPoint = currentWire.points.back();
              printf(
                  "Stored wire path (start: %.0f, %.0f, end: %.0f, %.0f) "
                  "with "
                  "carried node %d (%d points total)\n",
                  wireStartPoint.x, wireStartPoint.y, wireEndPoint.x,
                  wireEndPoint.y, currentWire.carriedNodeNumber,
                  (int)currentWire.points.size());
            }

            // Handle different wire connection scenarios
            if (currentWire.startComponentIndex != -1 &&
                currentWire.endComponentIndex != -1) {
              // Both ends connected to components - create
              // component-to-component connection
              printf("Creating node connection for completed wire\n");
              createNodeConnection(
                  currentWire.startComponentIndex, currentWire.startNodeIndex,
                  currentWire.endComponentIndex, currentWire.endNodeIndex);

              // After node merge, update wire's carried node number to the
              // merged node
              int mergedNodeNumber = getNodeNumber(
                  currentWire.startComponentIndex, currentWire.startNodeIndex);
              if (mergedNodeNumber != -1 &&
                  mergedNodeNumber != currentWire.carriedNodeNumber) {
                printf("Updating wire carried node from %d to merged node %d\n",
                       currentWire.carriedNodeNumber, mergedNodeNumber);

                // Update the wire in visualWires with the new node number
                visualWires.back().carriedNodeNumber = mergedNodeNumber;

                // Update positionToWireCarryNodeNumber entries for the WHOLE
                // wire: remove old entries and set the merged node number for
                // every point along the wire so the wire consistently carries
                // the same node everywhere (start, middle, end).
                auto& savedWire = visualWires.back();
                for (const ImVec2& pt : savedWire.points) {
                  auto key = positionToKey(pt);
                  // Remove any old entry at this exact position
                  if (positionToWireCarryNodeNumber.find(key) !=
                      positionToWireCarryNodeNumber.end()) {
                    positionToWireCarryNodeNumber.erase(key);
                  }
                  // Store the merged node number at this wire point
                  storeWireCarryNodeAtPosition(mergedNodeNumber, pt);
                }
                // Log updated start/end for convenience
                ImVec2 wireStartPoint = savedWire.points.front();
                ImVec2 wireEndPoint = savedWire.points.back();
                printf(
                    "Updated entire wire (start: %.0f, %.0f end: %.0f, %.0f) "
                    "with merged node %d\n",
                    wireStartPoint.x, wireStartPoint.y, wireEndPoint.x,
                    wireEndPoint.y, mergedNodeNumber);
                // Clear stale wire-carry nodes from both components
                clearComponentWireCarryNodes(currentWire.startComponentIndex);
                clearComponentWireCarryNodes(currentWire.endComponentIndex);
                // After merging node networks, sync components and rebuild
                // circuit
                syncComponentNodesFromWireCarry();
                updateCircuitComponentNodes();
                ensureWireConsistency();
              }
            } else if (currentWire.startComponentIndex != -1 &&
                       currentWire.endComponentIndex == -1) {
              // Wire starts from component but doesn't end on component
              // Check if it ends on an existing wire
              ImVec2 wireEndPoint = currentWire.points.back();
              int existingWireIndex = findWireAtPoint(wireEndPoint, 15.0f);

              if (existingWireIndex != -1) {
                int existingWireNodeNumber =
                    getWireNodeNumber(existingWireIndex);
                if (existingWireNodeNumber > 0) {
                  printf("Wire ending on existing wire %d with node %d\n",
                         existingWireIndex, existingWireNodeNumber);

                  // IMPORTANT: Update the NEW wire to carry the EXISTING
                  // wire's node
                  visualWires.back().carriedNodeNumber = existingWireNodeNumber;

                  // Store the existing wire's node at the new wire's start
                  // and end positions
                  ImVec2 newWireStart = currentWire.points.front();
                  ImVec2 newWireEnd = currentWire.points.back();
                  storeWireCarryNodeAtPosition(existingWireNodeNumber,
                                               newWireStart);
                  storeWireCarryNodeAtPosition(existingWireNodeNumber,
                                               newWireEnd);

                  // Store at all points along the new wire
                  for (const auto& pt : visualWires.back().points) {
                    storeWireCarryNodeAtPosition(existingWireNodeNumber, pt);
                  }

                  printf(
                      "Updated new wire to carry node %d (same as wire %d it "
                      "ends on)\n",
                      existingWireNodeNumber, existingWireIndex);

                  connectComponentToWireNode(currentWire.startComponentIndex,
                                             currentWire.startNodeIndex,
                                             existingWireNodeNumber);
                  //-------
                  // storeWireCarryNodeAtPosition(existingWireNodeNumber,
                  //                              wireEndPoint);
                  // storeWireCarryNodeAtPosition(
                  //     existingWireNodeNumber,
                  //     currentWire.points.front());  // also at start point

                  // Clear stale wire-carry nodes from the component
                  clearComponentWireCarryNodes(currentWire.startComponentIndex);
                  // Update all components and circuit
                  updateAllComponentNodeValues();
                  updateCircuitComponentNodes();
                } else {
                  printf("Wire ends on wire %d but it has no node number\n",
                         existingWireIndex);
                }
              } else {
                printf(
                    "Wire from component %d doesn't end on component or "
                    "existing wire\n",
                    currentWire.startComponentIndex);
              }
            } else if (currentWire.startComponentIndex == -1 &&
                       currentWire.endComponentIndex == -1) {
              // Wire doesn't start or end on a component - check if both ends
              // on wires
              ImVec2 wireStartPoint = currentWire.points.front();
              ImVec2 wireEndPoint = currentWire.points.back();

              int startWireIndex = findWireAtPoint(wireStartPoint, 15.0f);
              int endWireIndex = findWireAtPoint(wireEndPoint, 15.0f);

              if (startWireIndex != -1 && endWireIndex != -1 &&
                  startWireIndex != endWireIndex) {
                // Wire connects two different wires - merge them
                int startWireNode = getWireNodeNumber(startWireIndex);
                int endWireNode = getWireNodeNumber(endWireIndex);

                printf(
                    "Wire connects two wires: wire %d (node %d) to wire %d "
                    "(node %d)\n",
                    startWireIndex, startWireNode, endWireIndex, endWireNode);

                // Merge the wires - update the end wire to carry the start
                // wire's node
                if (startWireNode != -1) {
                  // Update the end wire and all its points to carry the start
                  // wire's node
                  visualWires[endWireIndex].carriedNodeNumber = startWireNode;

                  // Update all points on the end wire with the start wire's
                  // node
                  for (const ImVec2& pt : visualWires[endWireIndex].points) {
                    auto key = positionToKey(pt);
                    if (positionToWireCarryNodeNumber.find(key) !=
                        positionToWireCarryNodeNumber.end()) {
                      positionToWireCarryNodeNumber.erase(key);
                    }
                    storeWireCarryNodeAtPosition(startWireNode, pt);
                  }

                  printf(
                      "Updated wire %d to carry node %d (merged with wire "
                      "%d)\n",
                      endWireIndex, startWireNode, startWireIndex);
                }
              } else if (startWireIndex != -1) {
                // Wire ends on existing wire
                int startWireNode = getWireNodeNumber(startWireIndex);
                printf("Wire ends on existing wire %d with node %d\n",
                       startWireIndex, startWireNode);

                // Update current wire to carry the same node
                if (startWireNode != -1) {
                  currentWire.carriedNodeNumber = startWireNode;

                  // Update all points on current wire
                  visualWires.back().carriedNodeNumber = startWireNode;
                  auto& savedWire = visualWires.back();
                  for (const ImVec2& pt : savedWire.points) {
                    auto key = positionToKey(pt);
                    if (positionToWireCarryNodeNumber.find(key) !=
                        positionToWireCarryNodeNumber.end()) {
                      positionToWireCarryNodeNumber.erase(key);
                    }
                    storeWireCarryNodeAtPosition(startWireNode, pt);
                  }
                }
              }
            } else {
              printf(
                  "Wire completed but not both ends connected to components "
                  "(start: %d, end: %d)\n",
                  currentWire.startComponentIndex,
                  currentWire.endComponentIndex);
            }
          } else if (currentWire.points.size() == 1) {
            // Single point wire - just abandon it since no endpoint was
            // clicked
            printf("Single point wire abandoned on ESC - startComp was: %d\n",
                   currentWire.startComponentIndex);
          }

          // Reset wire drawing state robustly
          currentWire.points.clear();
          currentWire.startComponentIndex = -1;
          currentWire.startNodeIndex = -1;
          currentWire.endComponentIndex = -1;
          currentWire.endNodeIndex = -1;
          isDrawingWire = false;
          wireMode = false;
        }
      } else {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
      }
    } else {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

      // Preserve any in-progress wire when exiting wire mode
      if (isDrawingWire && currentWire.points.size() >= 2) {
        visualWires.push_back(currentWire);

        // Store wire-carried node at ALL positions along the wire path
        if (currentWire.carriedNodeNumber != -1) {
          // Store at every point in the wire path
          for (const auto& point : currentWire.points) {
            storeWireCarryNodeAtPosition(currentWire.carriedNodeNumber, point);
          }

          ImVec2 wireStartPoint = currentWire.points.front();
          ImVec2 wireEndPoint = currentWire.points.back();
          printf(
              "Stored wire path (start: %.0f, %.0f, end: %.0f, %.0f) with "
              "carried node %d (%d points total) - mode exit\n",
              wireStartPoint.x, wireStartPoint.y, wireEndPoint.x,
              wireEndPoint.y, currentWire.carriedNodeNumber,
              (int)currentWire.points.size());
        }

        // Handle different wire connection scenarios
        if (currentWire.startComponentIndex != -1 &&
            currentWire.endComponentIndex != -1) {
          // Both ends connected to components
          createNodeConnection(
              currentWire.startComponentIndex, currentWire.startNodeIndex,
              currentWire.endComponentIndex, currentWire.endNodeIndex);

          // After node merge, update wire's carried node number to the merged
          // node
          int mergedNodeNumber = getNodeNumber(currentWire.startComponentIndex,
                                               currentWire.startNodeIndex);
          if (mergedNodeNumber != -1 &&
              mergedNodeNumber != currentWire.carriedNodeNumber) {
            printf(
                "Updating wire carried node from %d to merged node %d "
                "(mode exit)\n",
                currentWire.carriedNodeNumber, mergedNodeNumber);

            // Update the wire in visualWires with the new node number
            visualWires.back().carriedNodeNumber = mergedNodeNumber;

            // Update positionToWireCarryNodeNumber entries with the new node
            // number
            // Update positionToWireCarryNodeNumber for every point on the
            // saved wire so the entire wire consistently carries the merged
            // node number (mode exit)
            auto& savedWire = visualWires.back();
            for (const ImVec2& pt : savedWire.points) {
              auto key = positionToKey(pt);
              if (positionToWireCarryNodeNumber.find(key) !=
                  positionToWireCarryNodeNumber.end()) {
                positionToWireCarryNodeNumber.erase(key);
              }
              storeWireCarryNodeAtPosition(mergedNodeNumber, pt);
            }
            ImVec2 wireStartPoint = savedWire.points.front();
            ImVec2 wireEndPoint = savedWire.points.back();
            printf(
                "Updated entire wire (start: %.0f, %.0f end: %.0f, %.0f) "
                "with merged node %d (mode exit)\n",
                wireStartPoint.x, wireStartPoint.y, wireEndPoint.x,
                wireEndPoint.y, mergedNodeNumber);
            // Clear stale wire-carry nodes from both components
            clearComponentWireCarryNodes(currentWire.startComponentIndex);
            clearComponentWireCarryNodes(currentWire.endComponentIndex);
            // After merging node networks on mode exit, sync components and
            // rebuild
            syncComponentNodesFromWireCarry();
            updateCircuitComponentNodes();
            ensureWireConsistency();
          }
        } else if (currentWire.startComponentIndex != -1 &&
                   currentWire.endComponentIndex == -1) {
          // Wire starts from component but doesn't end on component
          // Check if it ends on an existing wire
          ImVec2 wireEndPoint = currentWire.points.back();
          int existingWireIndex = findWireAtPoint(wireEndPoint, 15.0f);

          if (existingWireIndex != -1) {
            int existingWireNodeNumber = getWireNodeNumber(existingWireIndex);
            if (existingWireNodeNumber > 0) {
              printf(
                  "Wire ending on existing wire %d with node %d (mode "
                  "exit)\n",
                  existingWireIndex, existingWireNodeNumber);
              connectComponentToWireNode(currentWire.startComponentIndex,
                                         currentWire.startNodeIndex,
                                         existingWireNodeNumber);
              // Clear stale wire-carry nodes from the component
              clearComponentWireCarryNodes(currentWire.startComponentIndex);
              // Update all components and circuit
              updateAllComponentNodeValues();
              updateCircuitComponentNodes();
            }
          }
        } else if (currentWire.startComponentIndex == -1 &&
                   currentWire.endComponentIndex == -1) {
          // Wire doesn't start or end on a component - check if both ends on
          // wires
          ImVec2 wireStartPoint = currentWire.points.front();
          ImVec2 wireEndPoint = currentWire.points.back();

          int startWireIndex = findWireAtPoint(wireStartPoint, 15.0f);
          int endWireIndex = findWireAtPoint(wireEndPoint, 15.0f);

          if (startWireIndex != -1 && endWireIndex != -1 &&
              startWireIndex != endWireIndex) {
            // Wire connects two different wires - merge them
            int startWireNode = getWireNodeNumber(startWireIndex);
            int endWireNode = getWireNodeNumber(endWireIndex);

            printf(
                "Wire connects two wires: wire %d (node %d) to wire %d (node "
                "%d) (mode exit)\n",
                startWireIndex, startWireNode, endWireIndex, endWireNode);

            // Merge the wires - update the end wire to carry the start wire's
            // node
            if (startWireNode != -1) {
              // Update the end wire and all its points to carry the start
              // wire's node
              visualWires[endWireIndex].carriedNodeNumber = startWireNode;

              // Update all points on the end wire with the start wire's node
              for (const ImVec2& pt : visualWires[endWireIndex].points) {
                auto key = positionToKey(pt);
                if (positionToWireCarryNodeNumber.find(key) !=
                    positionToWireCarryNodeNumber.end()) {
                  positionToWireCarryNodeNumber.erase(key);
                }
                storeWireCarryNodeAtPosition(startWireNode, pt);
              }

              printf(
                  "Updated wire %d to carry node %d (merged with wire %d) "
                  "(mode exit)\n",
                  endWireIndex, startWireNode, startWireIndex);
            }
          } else if (startWireIndex != -1) {
            // Wire ends on existing wire
            int startWireNode = getWireNodeNumber(startWireIndex);
            printf("Wire ends on existing wire %d with node %d (mode exit)\n",
                   startWireIndex, startWireNode);

            // Update current wire to carry the same node
            if (startWireNode != -1) {
              currentWire.carriedNodeNumber = startWireNode;

              // Update all points on current wire
              visualWires.back().carriedNodeNumber = startWireNode;
              auto& savedWire = visualWires.back();
              for (const ImVec2& pt : savedWire.points) {
                auto key = positionToKey(pt);
                if (positionToWireCarryNodeNumber.find(key) !=
                    positionToWireCarryNodeNumber.end()) {
                  positionToWireCarryNodeNumber.erase(key);
                }
                storeWireCarryNodeAtPosition(startWireNode, pt);
              }
            }
          }
        }
      }
      currentWire.points.clear();
      isDrawingWire = false;
    }

    ImGui::EndChild();  // End A4Canvas child
    ImGui::End();       // End CanvasContainer
  }

  /**
   * Displays nodes voltages, voltage sources currents graphs.
   * Content depends on options selected in settings.
   */
  void renderNodeVoltageGraphWindow() {
    if (!showGraphForNodeVolt || !graph_) {
      return;
    }

    int num_nodes = circuit.getNodeCount();

    if (isDCMode && solution_.size() >= num_nodes) {
      // Create a separate, moveable, resizable window for the graph
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(300, 150), ImGuiCond_FirstUseEver);

      if (ImGui::Begin("Node Voltage Graph", &showGraphForNodeVolt,
                       ImGuiWindowFlags_NoCollapse)) {
        // Update graph with current DC solution
        graph_->setDCsolution(solution_);
        graph_->setCircuit(circuit);
        graph_->setNumNodes(num_nodes);
        graph_->renderNodeVoltageGraphDC();
      }
      ImGui::End();
    }

    if (isACMode && acSolution_.size() >= num_nodes) {
      // Create a separate, moveable, resizable window for the graph
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(300, 150), ImGuiCond_FirstUseEver);

      if (ImGui::Begin("Node Voltage Graph", &showGraphForNodeVolt,
                       ImGuiWindowFlags_NoCollapse)) {
        // Update graph with current AC solution
        graph_->setACsolution(acSolution_);
        graph_->setCircuit(circuit);
        graph_->setNumNodes(num_nodes);
        graph_->renderNodeVoltageGraphAC();
      }
      ImGui::End();
    }
  }

  void renderVoltageSourceCurrentGraphWindow() {
    if (!showGraphForVoltageSourceCurrent || !graph_) {
      return;
    }

    int num_nodes = circuit.getNodeCount();

    if (isDCMode && solution_.size() >= num_nodes) {
      // Create a separate, moveable, resizable window for the graph
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(300, 150), ImGuiCond_FirstUseEver);

      if (ImGui::Begin("Voltage Source Current Graph",
                       &showGraphForVoltageSourceCurrent,
                       ImGuiWindowFlags_NoCollapse)) {
        // Update graph with current DC solution
        graph_->setDCsolution(solution_);
        graph_->setCircuit(circuit);
        graph_->setNumNodes(num_nodes);
        graph_->renderVoltageSourceCurrentGraphDC();
      }

      ImGui::End();
    }

    if (isACMode && acSolution_.size() >= num_nodes) {
      // Create a separate, moveable, resizable window for the graph
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(300, 150), ImGuiCond_FirstUseEver);

      if (ImGui::Begin("Voltage Source Current Graph",
                       &showGraphForVoltageSourceCurrent,
                       ImGuiWindowFlags_NoCollapse)) {
        // Update graph with current AC solution
        graph_->setACsolution(acSolution_);
        graph_->setCircuit(circuit);
        graph_->setNumNodes(num_nodes);
        graph_->renderVoltageSourceCurrentGraphAC();
      }
      ImGui::End();
    }
  }

  /**
   * Renders the right sidebar showing simulation measurements and results.
   * Displays node voltages, component currents, and analysis graphs.
   * Content depends on measurement options selected in settings.
   */
  void renderMeasurements() {
    ImGui::SetNextWindowPos(
        ImVec2(windowSize.x - 200, ImGui::GetFrameHeight() + 65));
    ImGui::SetNextWindowSize(
        ImVec2(200, windowSize.y - ImGui::GetFrameHeight() - 65));
    ImGui::Begin("##Measurements", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    // DC Analysis Results
    if (isDCMode) {
      int num_nodes = circuit.getNodeCount();
      if (solution_.size() >= num_nodes) {
        ImGui::Text("Node Voltages");
        for (int i = 0; i < num_nodes; ++i) {
          if (solution_(i) == 0.0) continue;
          ImGui::Text("Node V_%d: %.2f V", (i + 1), solution_(i));
        }

        // voltage source currents
        ImGui::Text("Volt.Src Currents");
        std::vector<std::string> current_names;
        for (const auto& comp : circuit.getComponents()) {
          if (dynamic_cast<VoltageSource*>(comp.get()) ||
              dynamic_cast<ACVoltageSource*>(comp.get())) {
            current_names.push_back(comp->getName());  // e.g. "V4", "V5"
          }
        }

        int current_idx_offset = num_nodes;
        for (size_t i = 0; i < current_names.size(); ++i) {
          int idx = current_idx_offset + static_cast<int>(i);
          if (idx < solution_.size()) {
            ImGui::Text("I_%s: %.4f A", current_names[i].c_str(),
                        solution_(idx));
          }
        }

        /**
         * Display calculated currents for all passive components (resistors,
         * inductors, capacitors) in the results/measurements panel. This
         * allows users to review the current through each passive component
         * after simulation, regardless of when the simulation was last run.
         * Skips voltage and current sources, computes current using Ohm's
         * law, and shows the result.
         */
        ImGui::Separator();
        ImGui::Text("Passive Comp. Currents");
        for (const auto& comp : circuit.getComponents()) {
          // Only print for non-voltage/current sources
          std::string name = comp->getName();
          if (!name.empty() && name[0] != 'V' && name[0] != 'I') {
            int comNodeA = comp->getNodes()[0];
            int comNodeB = comp->getNodes()[1];

            double vA = (comNodeA > 0) ? solution_(comNodeA - 1) : 0.0;
            double vB = (comNodeB > 0) ? solution_(comNodeB - 1) : 0.0;

            double compCurrent = 0.0;
            if (comp->getValue() != 0.0) {
              compCurrent = (vA - vB) / comp->getValue();
            }
            ImGui::Text("%s_I_rms: %.4f A", name.c_str(), compCurrent);
          }
        }
      }

    } else if (isACMode) {
      int num_nodes = circuit.getNodeCount();
      if (acSolution_.size() >= num_nodes) {
        ImGui::Text("Node Voltages (RMS)");
        for (int i = 0; i < num_nodes; ++i) {
          std::complex<double> Vnode = acSolution_(i);

          // RMS magnitude
          double Vrms = std::abs(Vnode) / std::sqrt(2.0);
          if (Vrms == 0.0) continue;

          ImGui::Text("Node V_%d: %.4f V", (i + 1), Vrms);
        }

        // voltage source currents
        ImGui::Text("Volt.Src Currents (RMS)");
        std::vector<std::string> current_names;
        for (const auto& comp : circuit.getComponents()) {
          if (dynamic_cast<VoltageSource*>(comp.get()) ||
              dynamic_cast<ACVoltageSource*>(comp.get())) {
            current_names.push_back(comp->getName());  // e.g. "V4", "V5"
          }
        }

        int current_idx_offset = num_nodes;
        for (size_t i = 0; i < current_names.size(); ++i) {
          int idx = current_idx_offset + static_cast<int>(i);
          std::complex<double> Isrc = acSolution_(idx);

          // RMS magnitude
          double Isrc_rms = std::abs(Isrc) / std::sqrt(2.0);
          ImGui::Text("I_%s: %.4f A", current_names[i].c_str(), Isrc_rms);
        }

        // Graph window is now handled separately below
        ImGui::Separator();

        ImGui::Separator();
        ImGui::Text("Passive Comp. Currents");
        for (const auto& comp : circuit.getComponents()) {
          // Only print for non-voltage/current sources
          std::string name = comp->getName();
          if (!name.empty() && name[0] != 'V' && name[0] != 'I') {
            int comNodeA = comp->getNodes()[0];
            int comNodeB = comp->getNodes()[1];

            // Bounds check before accessing acSolution_
            std::complex<double> solu_A = 0.0;
            std::complex<double> solu_B = 0.0;
            if (comNodeA > 0 && comNodeA <= acSolution_.size())
              solu_A = acSolution_(comNodeA - 1);

            if (comNodeB > 0 && comNodeB <= acSolution_.size())
              solu_B = acSolution_(comNodeB - 1);

            // RMS magnitude
            double magA = std::abs(solu_A) / std::sqrt(2.0);
            double magB = std::abs(solu_B) / std::sqrt(2.0);

            double vA = (comNodeA > 0) ? magA : 0.0;
            double vB = (comNodeB > 0) ? magB : 0.0;

            double compCurrent = 0.0;
            if (comp->getValue() != 0.0) {
              double omega = 2.0 * M_PI *
                             50.0;  // Assuming 50 Hz for reactance calculations
              switch (name[0]) {
                case 'R':
                  compCurrent = (vA - vB) / comp->getValue();
                  break;
                case 'L': {
                  double Xl = omega * comp->getValue();  // Inductive reactance
                  if (Xl != 0.0) {
                    compCurrent = (vA - vB) / Xl;
                  }
                  break;
                }
                case 'C': {
                  double Xc = omega * comp->getValue();  // Capacitive reactance
                  if (Xc != 0.0) {
                    compCurrent = (vA - vB) * Xc;
                  }
                  break;
                }
                default:
                  break;
              }
            }
            ImGui::Text("%s_I_rms: %.4f A", name.c_str(), compCurrent);
          }
        }
      } else {
        ImGui::Text("No simulation data.");
        ImGui::Separator();
        ImGui::Text("No node voltages found.");
      }
    } else {
      ImGui::Text("No analysis run yet.");
      ImGui::Separator();
      ImGui::Text("Press 'Run DC' or 'Run AC'");
    }

    // Circuit Netlist Display
    ImGui::Separator();
    ImGui::Text("Circuit Netlist");
    ImGui::Text("(GND node is 0)");

    const auto& components = circuit.getComponents();

    if (!components.empty()) {
      for (const auto& comp : components) {
        if (comp) {
          ImGui::Text("%s %d %d %.4g", comp->getName().c_str(),
                      comp->getNodes()[0], comp->getNodes()[1],
                      comp->getValue());
        }
      }

    } else {
      ImGui::Text("No circuit components built");
    }

    ImGui::End();
  }

  /**
   * Main update loop - handles ImGui updates and renders all UI panels.
   * Called every frame before render().
   * Order: ImGui update -> Initialize grid -> Render all panels
   */
  void update() {
    ImGui::SFML::Update(window, deltaClock.restart());

    // Initialize canvas grid after ImGui is ready (first frame only)
    if (!gridInitialized) {
      generateCanvasGrid();
      gridInitialized = true;
    }

    // Render all UI panels in order
    renderMenuBar();        // Top menu (File, Edit, Window, Help)
    renderSimpleToolbar();  // Toolbar with settings and component buttons
    // Component bar removed - canvas now uses full left space

    // Render component edit popup BEFORE canvas so click handlers can detect
    // it This is safe because we check isEditPopupCurrentlyOpen in click
    // handlers
    renderComponentEditPopup();

    renderCanvas();        // Main canvas area with grid and components
    renderMeasurements();  // Right sidebar with simulation results

    // Separate moveable/resizable graph window
    renderNodeVoltageGraphWindow();
    renderVoltageSourceCurrentGraphWindow();

    // Render file dialogs
    renderSaveDialog();
    renderLoadDialog();
    // renderExportNetlistDialog();
    // renderImportNetlistDialog();
  }

  /**
   * Refreshes the file browser with directories and files in
   * currentBrowsePath
   */
  void refreshFileBrowser() {
    browseDirectories.clear();
    browseFiles.clear();

    // Add parent directory option if not at root
    if (currentBrowsePath != "/" && !currentBrowsePath.empty()) {
      browseDirectories.push_back("..");
    }

    // Scan directory for files and subdirectories
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE findHandle =
        FindFirstFileA((currentBrowsePath + "\\*").c_str(), &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {
      do {
        if (findData.cFileName[0] == '.') continue;  // Skip hidden files
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          browseDirectories.push_back(findData.cFileName);
        } else {
          browseFiles.push_back(findData.cFileName);
        }
      } while (FindNextFileA(findHandle, &findData));
      FindClose(findHandle);
    }
#else
    DIR* dir = opendir(currentBrowsePath.c_str());
    if (dir) {
      struct dirent* entry;
      struct stat statbuf;
      while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;  // Skip hidden files
        std::string fullPath = currentBrowsePath + "/" + entry->d_name;
        if (stat(fullPath.c_str(), &statbuf) == 0) {
          if (S_ISDIR(statbuf.st_mode)) {
            browseDirectories.push_back(entry->d_name);
          } else {
            browseFiles.push_back(entry->d_name);
          }
        }
      }
      closedir(dir);
    }
#endif

    // Sort both lists
    std::sort(browseDirectories.begin(), browseDirectories.end());
    std::sort(browseFiles.begin(), browseFiles.end());
  }

  /**
   * Gets the home directory path for the current user
   */
  std::string getHomePath() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    return home ? home : "C:\\";
#else
    const char* home = getenv("HOME");
    if (home) return home;
    // Fallback: try to get from pwd.h
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    return pw ? pw->pw_dir : "/";
#endif
  }

  /**
   * Renders the Save Circuit dialog with file browser
   */
  void renderSaveDialog() {
    if (showSaveDialog) {
      ImGui::OpenPopup("Save Circuit");
      showSaveDialog = false;
      // Initialize file browser on first open
      if (currentBrowsePath.empty()) {
        currentBrowsePath = getHomePath();
        refreshFileBrowser();
      }
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Save Circuit", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Current location: %s", currentBrowsePath.c_str());
      ImGui::Separator();

      // Directory/File browser
      ImGui::BeginChild("BrowserPane", ImVec2(550, 300), true);

      // Show directories
      for (const auto& dir : browseDirectories) {
        std::string label = "[DIR] " + dir;
        if (ImGui::Selectable(label.c_str(), false)) {
          if (dir == "..") {
            // Go to parent directory
            size_t lastSlash = currentBrowsePath.rfind('/');
            if (lastSlash != std::string::npos && lastSlash > 0) {
              currentBrowsePath = currentBrowsePath.substr(0, lastSlash);
            }
          } else {
            // Enter subdirectory
            if (currentBrowsePath.back() != '/') {
              currentBrowsePath += '/';
            }
            currentBrowsePath += dir;
          }
          refreshFileBrowser();
        }
      }

      // Show .cir files
      for (const auto& file : browseFiles) {
        if (file.find(".cir") != std::string::npos) {
          if (ImGui::Selectable(file.c_str(), selectedFileName == file)) {
            selectedFileName = file;
          }
        }
      }

      ImGui::EndChild();

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Filename input
      ImGui::Text("Filename:");
      ImGui::SameLine();
      ImGui::InputText("##filename", saveFilenameBuffer,
                       sizeof(saveFilenameBuffer));

      ImGui::Spacing();

      if (ImGui::Button("Save", ImVec2(120, 0))) {
        std::string filename = std::string(saveFilenameBuffer);
        // Add .cir extension if not present
        if (filename.find(".cir") == std::string::npos) {
          filename += ".cir";
        }
        // Construct full path
        std::string fullPath = currentBrowsePath;
        if (fullPath.back() != '/') {
          fullPath += '/';
        }
        fullPath += filename;
        saveCircuitToFile(fullPath);
        currentFilename = fullPath;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  /**
   * Renders the Load Circuit dialog
   */
  void renderLoadDialog() {
    if (showLoadDialog) {
      ImGui::OpenPopup("Load Circuit");
      showLoadDialog = false;
      // Initialize file browser on first open
      if (currentBrowsePath.empty()) {
        currentBrowsePath = getHomePath();
        refreshFileBrowser();
      }
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Load Circuit", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Current location: %s", currentBrowsePath.c_str());
      ImGui::Separator();

      // Directory/File browser
      ImGui::BeginChild("BrowserPaneLoad", ImVec2(550, 300), true);

      // Show directories
      for (const auto& dir : browseDirectories) {
        std::string label = "[DIR] " + dir;
        if (ImGui::Selectable(label.c_str(), false)) {
          if (dir == "..") {
            // Go to parent directory
            size_t lastSlash = currentBrowsePath.rfind('/');
            if (lastSlash != std::string::npos && lastSlash > 0) {
              currentBrowsePath = currentBrowsePath.substr(0, lastSlash);
            }
          } else {
            // Enter subdirectory
            if (currentBrowsePath.back() != '/') {
              currentBrowsePath += '/';
            }
            currentBrowsePath += dir;
          }
          refreshFileBrowser();
        }
      }

      // Show .cir files
      for (const auto& file : browseFiles) {
        if (file.find(".cir") != std::string::npos) {
          if (ImGui::Selectable(file.c_str(), selectedFileName == file)) {
            selectedFileName = file;
            // Auto-populate filename field
            strncpy(loadFilenameBuffer, file.c_str(),
                    sizeof(loadFilenameBuffer) - 1);
          }
        }
      }

      ImGui::EndChild();

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Filename input
      ImGui::Text("Filename:");
      ImGui::SameLine();
      ImGui::InputText("##filename_load", loadFilenameBuffer,
                       sizeof(loadFilenameBuffer));

      ImGui::Spacing();

      if (ImGui::Button("Load", ImVec2(120, 0))) {
        std::string filename = std::string(loadFilenameBuffer);
        // Add .cir extension if not present
        if (filename.find(".cir") == std::string::npos) {
          filename += ".cir";
        }
        // Construct full path
        std::string fullPath = currentBrowsePath;
        if (fullPath.back() != '/') {
          fullPath += '/';
        }
        fullPath += filename;
        loadCircuitFromFile(fullPath);
        currentFilename = fullPath;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  /**
   * Main render function - draws the entire application frame.
   * Order: Background -> ImGui content -> SFML component sprites -> Display
   */
  void render() {
    window.clear(appBackColor);  // Dark background for application
    ImGui::SFML::Render(window);

    // Draw component sprites on top of ImGui (only for existing components)
    for (const auto& comp : visualComponents) {
      auto spriteIter = componentSprites.find(comp.name);
      if (spriteIter != componentSprites.end()) {
        window.draw(spriteIter->second);
      }
    }

    window.display();
  }

  /**
   * Canvas Grid System Functions
   * These functions handle the grid that appears only in the canvas area
   */

  /**
   * Initializes the canvas grid system.
   * Called once in constructor after SFML/ImGui setup.
   */
  void initCanvasGrids() { generateCanvasGrid(); }

  /**
   * Generates grid lines for the fixed A4 canvas.
   * Creates both vertical and horizontal lines based on A4 dimensions.
   * Grid is always fixed at A4 size regardless of window size.
   */
  void generateCanvasGrid() {
    vLines.clear();
    hLines.clear();

    // Generate vertical grid lines for A4 canvas
    for (float x = 0; x <= A4_WIDTH; x += gridSpacing) {
      sf::RectangleShape line(sf::Vector2f(1.0f, A4_HEIGHT));
      line.setPosition(x, 0);
      line.setFillColor(gridColor);
      vLines.push_back(line);
    }

    // Generate horizontal grid lines for A4 canvas
    for (float y = 0; y <= A4_HEIGHT; y += gridSpacing) {
      sf::RectangleShape line(sf::Vector2f(A4_WIDTH, 1.0f));
      line.setPosition(0, y);
      line.setFillColor(gridColor);
      hLines.push_back(line);
    }
  }

  /**
   * Draws the A4 canvas background and grid with extended scrollable area.
   * This function draws the extended canvas background and grid using ImGui's
   * draw list. The canvas extends beyond A4 size for scrolling capability.
   */
  void drawCanvasBackground() {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Calculate extended dimensions
    float extendedWidth = A4_WIDTH * 3.0f;
    float extendedHeight = A4_HEIGHT * 2.0f;

    // Convert SFML colors to ImGui format
    ImU32 paperColor = IM_COL32(250, 250, 250, 255);  // Clean white canvas
    ImU32 lightGreyGridColor =
        IM_COL32(180, 180, 180, 100);  // Subtle grid lines
    ImU32 borderColor = IM_COL32(150, 150, 150, 255);

    // Draw extended paper background
    drawList->AddRectFilled(
        canvasPos,
        ImVec2(canvasPos.x + extendedWidth, canvasPos.y + extendedHeight),
        paperColor);

    // Draw grid lines within extended canvas area
    // Vertical lines
    for (float x = 0; x <= extendedWidth; x += gridSpacing) {
      drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                        ImVec2(canvasPos.x + x, canvasPos.y + extendedHeight),
                        lightGreyGridColor, 1.0f);
    }

    // Horizontal lines
    for (float y = 0; y <= extendedHeight; y += gridSpacing) {
      drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                        ImVec2(canvasPos.x + extendedWidth, canvasPos.y + y),
                        lightGreyGridColor, 1.0f);
    }
  }

  /**
   * Draws and handles interaction with visual components on A4 canvas.
   * Features: Drawing components, selection, dragging, rotation with Ctrl+R,
   * multi-selection with drag rectangle
   */
  void drawAndHandleVisualComponents(ImVec2 canvasPos, ImDrawList* drawList) {
    ImVec2 mousePos = ImGui::GetMousePos();
    // Extended canvas area for component interaction
    float extendedWidth = A4_WIDTH * 3.0f;
    float extendedHeight = A4_HEIGHT * 2.0f;
    bool mouseInCanvas = (mousePos.x >= canvasPos.x &&
                          mousePos.x <= canvasPos.x + extendedWidth &&
                          mousePos.y >= canvasPos.y &&
                          mousePos.y <= canvasPos.y + extendedHeight);

    // Handle mouse input for component interaction
    if (mouseInCanvas && !wireMode && selectedLabelIndex == -1) {
      // Convert screen coordinates to canvas-relative coordinates
      ImVec2 canvasRelativePos =
          ImVec2(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
      handleComponentSelection(canvasRelativePos, drawList, canvasPos);
    }

    // // Handle right-click on components
    // if (mouseInCanvas && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    //   ImVec2 canvasRelativePos =
    //       ImVec2(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
    //   int rightClickedComponent =
    //   getComponentAtPosition(canvasRelativePos);

    //   if (rightClickedComponent >= 0 &&
    //       rightClickedComponent < visualComponents.size()) {
    //     ImGui::OpenPopup("ResistorPopup");
    //     selectedComponentIndex = rightClickedComponent;
    //     printf("Right-clicked on component %d at (%.1f, %.1f)\n",
    //            rightClickedComponent, canvasRelativePos.x,
    //            canvasRelativePos.y);
    //   }
    // }

    // Handle dragging of selected components (only if not dragging label)
    if (draggingLabelIndex == -1) {
      // Pass screen coordinates to handleComponentDragging for proper drag
      // detection
      handleComponentDragging(mousePos, canvasPos);
    }

    // Handle rotation with Ctrl+R for selected components
    handleComponentRotation();

    // Draw all visual components
    for (int i = 0; i < visualComponents.size(); i++) {
      drawVisualComponentWithImage(visualComponents[i], drawList, i, canvasPos);
    }

    // Draw group bounding box for selected components
    if (selectedComponents.size() > 1) {
      drawGroupBoundingBox(drawList);
    }
  }

  /**
   * Deletes all selected components and wires from the canvas.
   */
  void deleteSelectedItems() {
    if (selectedComponents.empty() && selectedWires.empty()) {
      return;  // Nothing to delete
    }

    // Count items being deleted for feedback
    int deletedComponents = selectedComponents.size();
    int deletedWires = selectedWires.size();

    // Delete selected wires (in reverse order to maintain indices)
    std::sort(selectedWires.rbegin(), selectedWires.rend());
    for (int wireIndex : selectedWires) {
      if (wireIndex >= 0 && wireIndex < visualWires.size()) {
        VisualWire& wire = visualWires[wireIndex];

        // Remove wire-carried node positions from
        // positionToWireCarryNodeNumber
        if (wire.carriedNodeNumber >= 0) {
          // Remove start position
          if (!wire.points.empty()) {
            ImVec2 wireStartPos = wire.points.front();
            auto wireStartKey = positionToKey(wireStartPos);
            if (positionToWireCarryNodeNumber.find(wireStartKey) !=
                positionToWireCarryNodeNumber.end()) {
              positionToWireCarryNodeNumber.erase(wireStartKey);
              printf(
                  "Removed wire start position (%.0f, %.0f) with node %d "
                  "from "
                  "positionToWireCarryNodeNumber\n",
                  wireStartPos.x, wireStartPos.y, wire.carriedNodeNumber);
            }
          }

          // Remove end position
          if (wire.points.size() > 1) {
            ImVec2 wireEndPos = wire.points.back();
            auto wireEndKey = positionToKey(wireEndPos);
            if (positionToWireCarryNodeNumber.find(wireEndKey) !=
                positionToWireCarryNodeNumber.end()) {
              positionToWireCarryNodeNumber.erase(wireEndKey);
              printf(
                  "Removed wire end position (%.0f, %.0f) with node %d from "
                  "positionToWireCarryNodeNumber\n",
                  wireEndPos.x, wireEndPos.y, wire.carriedNodeNumber);
            }
          }
        }

        visualWires.erase(visualWires.begin() + wireIndex);
      }
    }

    // Delete selected components and their corresponding sprites (in reverse
    // order to maintain indices)
    std::sort(selectedComponents.rbegin(), selectedComponents.rend());
    for (int compIndex : selectedComponents) {
      if (compIndex >= 0 && compIndex < visualComponents.size()) {
        VisualComponent& comp = visualComponents[compIndex];

        // Get component's node positions
        auto [nodeAPos, nodeBPos] = getComponentNodePositions(comp);
        int nodeANum = comp.nodeA;
        int nodeBNum = comp.nodeB;

        printf(
            "Deleting component %d (%s) with nodeA=%d at (%.0f, %.0f) and "
            "nodeB=%d at (%.0f, %.0f)\n",
            compIndex, comp.name.c_str(), nodeANum, nodeAPos.x, nodeAPos.y,
            nodeBNum, nodeBPos.x, nodeBPos.y);

        // BEFORE deletion: Find wires at nodeA and nodeB positions and
        // transfer node numbers Check for wires at nodeA position
        for (auto& wire : visualWires) {
          for (const auto& wirePoint : wire.points) {
            float dx = wirePoint.x - nodeAPos.x;
            float dy = wirePoint.y - nodeAPos.y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist <= 20.0f) {  // SNAP_DISTANCE
              // Found wire at nodeA position - transfer nodeA's node number
              // to this wire
              wire.carriedNodeNumber = nodeANum;

              // Store the node number at both wire start and end positions
              ImVec2 wireStart = wire.points.front();
              ImVec2 wireEnd = wire.points.back();

              storeWireCarryNodeAtPosition(nodeANum, wireStart);
              storeWireCarryNodeAtPosition(nodeANum, wireEnd);

              printf(
                  "Component deleted: Wire at nodeA position (%.0f, %.0f) "
                  "now "
                  "carries node %d\n",
                  nodeAPos.x, nodeAPos.y, nodeANum);
              printf(
                  "  Stored node %d at wire start (%.0f, %.0f) and end "
                  "(%.0f, "
                  "%.0f)\n",
                  nodeANum, wireStart.x, wireStart.y, wireEnd.x, wireEnd.y);
              break;
            }
          }
        }

        // Check for wires at nodeB position
        for (auto& wire : visualWires) {
          for (const auto& wirePoint : wire.points) {
            float dx = wirePoint.x - nodeBPos.x;
            float dy = wirePoint.y - nodeBPos.y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist <= 20.0f) {  // SNAP_DISTANCE
              // Found wire at nodeB position - transfer nodeB's node number
              // to this wire
              wire.carriedNodeNumber = nodeBNum;

              // Store the node number at both wire start and end positions
              ImVec2 wireStart = wire.points.front();
              ImVec2 wireEnd = wire.points.back();

              storeWireCarryNodeAtPosition(nodeBNum, wireStart);
              storeWireCarryNodeAtPosition(nodeBNum, wireEnd);

              printf(
                  "Component deleted: Wire at nodeB position (%.0f, %.0f) "
                  "now "
                  "carries node %d\n",
                  nodeBPos.x, nodeBPos.y, nodeBNum);
              printf(
                  "  Stored node %d at wire start (%.0f, %.0f) and end "
                  "(%.0f, "
                  "%.0f)\n",
                  nodeBNum, wireStart.x, wireStart.y, wireEnd.x, wireEnd.y);
              break;
            }
          }
        }

        // Remove position-to-node mappings for this component
        auto nodeAPosKey = positionToKey(nodeAPos);
        auto nodeBPosKey = positionToKey(nodeBPos);

        if (positionToNodeNumber.find(nodeAPosKey) !=
            positionToNodeNumber.end()) {
          positionToNodeNumber.erase(nodeAPosKey);
          printf(
              "Removed nodeA position (%.0f, %.0f) from "
              "positionToNodeNumber\n",
              nodeAPos.x, nodeAPos.y);
        }

        if (positionToNodeNumber.find(nodeBPosKey) !=
            positionToNodeNumber.end()) {
          positionToNodeNumber.erase(nodeBPosKey);
          printf(
              "Removed nodeB position (%.0f, %.0f) from "
              "positionToNodeNumber\n",
              nodeBPos.x, nodeBPos.y);
        }
        // store node number for start and end of wire

        // Remove corresponding SFML sprite before deleting the component
        const std::string& componentName = comp.name;
        auto spriteIter = componentSprites.find(componentName);
        if (spriteIter != componentSprites.end()) {
          componentSprites.erase(spriteIter);
        }

        // Remove the visual component
        visualComponents.erase(visualComponents.begin() + compIndex);
      }
    }

    // CRITICAL: Rebuild componentNodeToNumber map after deletion
    // because component indices have shifted
    componentNodeToNumber.clear();
    for (int i = 0; i < visualComponents.size(); i++) {
      const auto& comp = visualComponents[i];
      componentNodeToNumber[{i, 0}] = comp.nodeA;
      componentNodeToNumber[{i, 1}] = comp.nodeB;
      printf("Remapped component %d (%s) to nodes [%d, %d]\n", i,
             comp.name.c_str(), comp.nodeA, comp.nodeB);
    }

    // Update wire connections after component deletion
    updateWireConnectionsAfterDeletion();

    // Clean up any orphaned sprites (additional safety measure)
    cleanupOrphanedSprites();

    // Rebuild the circuit to reflect deleted components
    updateAllComponentNodeValues();

    // After deletion, ensure wires maintain their node numbers consistently
    // Wires keep their node numbers regardless of component changes
    ensureWireConsistency();

    // DO NOT sync component nodes after deletion - this would cause
    // remaining components to adopt node numbers from disconnected wires.
    // Components keep their original node numbers after deletion.
    // Only the circuit is rebuilt with updated component indices.
    updateCircuitComponentNodes();

    // Clear selections after deletion
    clearAllSelections();
    selectedComponentIndex = -1;
    isDraggingComponent = false;
    isSelecting = false;

    // Optional: Print deletion feedback to console for debugging
    printf("Deleted %d components and %d wires\n", deletedComponents,
           deletedWires);
  }

  /**
   * Removes orphaned SFML sprites that no longer correspond to existing
   * visual components.
   */
  void cleanupOrphanedSprites() {
    // Create a set of existing component names for fast lookup
    std::set<std::string> existingComponentNames;
    for (const auto& comp : visualComponents) {
      existingComponentNames.insert(comp.name);
    }

    // Remove sprites that don't have corresponding components
    auto spriteIter = componentSprites.begin();
    while (spriteIter != componentSprites.end()) {
      if (existingComponentNames.find(spriteIter->first) ==
          existingComponentNames.end()) {
        spriteIter = componentSprites.erase(spriteIter);
      } else {
        ++spriteIter;
      }
    }
  }

  /**
   * Updates wire connections after component deletion by removing invalid
   * references.
   */
  void updateWireConnectionsAfterDeletion() {
    // Clean up wire connections that reference deleted components
    for (auto& wire : visualWires) {
      // Check start component connection
      if (wire.startComponentIndex >= 0 &&
          wire.startComponentIndex >= visualComponents.size()) {
        wire.startComponentIndex = -1;
        wire.startNodeIndex = -1;
      }

      // Check end component connection
      if (wire.endComponentIndex >= 0 &&
          wire.endComponentIndex >= visualComponents.size()) {
        wire.endComponentIndex = -1;
        wire.endNodeIndex = -1;
      }
    }

    // Update component indices for remaining wires
    // This is needed because deleting components shifts indices
    for (auto& wire : visualWires) {
      int deletedComponentsBefore = 0;

      // Count deleted components before the start component
      if (wire.startComponentIndex >= 0) {
        for (int deletedIndex : selectedComponents) {
          if (deletedIndex < wire.startComponentIndex) {
            deletedComponentsBefore++;
          }
        }
        wire.startComponentIndex -= deletedComponentsBefore;
      }

      deletedComponentsBefore = 0;
      // Count deleted components before the end component
      if (wire.endComponentIndex >= 0) {
        for (int deletedIndex : selectedComponents) {
          if (deletedIndex < wire.endComponentIndex) {
            deletedComponentsBefore++;
          }
        }
        wire.endComponentIndex -= deletedComponentsBefore;
      }
    }
  }

  /**
   * Handles component selection logic including single-click,
   * multi-selection, and drag-selection rectangle. Selected components remain
   * grouped until mouse click on empty space or ESC key is pressed.
   */
  void handleComponentSelection(ImVec2 mousePos, ImDrawList* drawList,
                                ImVec2 canvasPos) {
    // Handle ESC key to clear selection
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      clearAllSelections();
      selectedComponentIndex = -1;
      isDraggingComponent = false;
      isSelecting = false;
      return;
    }

    // Handle Delete key to delete selected items (but not if edit popup is
    // open)
    if ((ImGui::IsKeyPressed(ImGuiKey_Delete) ||
         ImGui::IsKeyPressed(ImGuiKey_Backspace) ||
         (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X))) &&
        !isEditPopupCurrentlyOpen) {
      deleteSelectedItems();
      return;
    }

    // Left mouse button pressed
    // But only if mouse is not over the graph window or the edit popup
    bool isOverGraphWindowClick = false;
    ImGuiWindow* graphWindowClick =
        ImGui::FindWindowByName("AC Analysis Graph");
    if (graphWindowClick != nullptr) {
      isOverGraphWindowClick =
          ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    }

    // Only block canvas clicks if the ComponentEditPopup is currently open
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !isOverGraphWindowClick && !isEditPopupCurrentlyOpen) {
      int clickedComponent = getComponentAtPosition(mousePos);

      if (clickedComponent >= 0) {
        // Clicked on a component
        if (!ImGui::GetIO().KeyCtrl) {
          // Check if clicked component is already in selection
          bool isAlreadySelected =
              std::find(selectedComponents.begin(), selectedComponents.end(),
                        clickedComponent) != selectedComponents.end();

          if (!isAlreadySelected) {
            // Single selection (clear others unless component is already
            // selected)
            clearAllSelections();
            selectedComponents.clear();
          }
        }

        // Toggle or add to selection
        if (std::find(selectedComponents.begin(), selectedComponents.end(),
                      clickedComponent) != selectedComponents.end()) {
          // Component already selected
          if (ImGui::GetIO().KeyCtrl) {
            // Remove from selection only if Ctrl is held
            selectedComponents.erase(
                std::remove(selectedComponents.begin(),
                            selectedComponents.end(), clickedComponent),
                selectedComponents.end());
            visualComponents[clickedComponent].isSelected = false;

            // Update selectedComponentIndex if we removed the current one
            if (selectedComponentIndex == clickedComponent) {
              selectedComponentIndex =
                  selectedComponents.empty() ? -1 : selectedComponents[0];
            }
          } else {
            // Component is already selected and no Ctrl - just setup dragging
            // for the group
            selectedComponentIndex = clickedComponent;
          }
        } else {
          // Add component to selection
          selectedComponents.push_back(clickedComponent);
          visualComponents[clickedComponent].isSelected = true;
          selectedComponentIndex = clickedComponent;
        }

        // Setup dragging for single component or group if we have selections
        if (!selectedComponents.empty()) {
          isDraggingComponent = true;
          dragOffset = ImVec2(
              mousePos.x - visualComponents[selectedComponentIndex].position.x,
              mousePos.y - visualComponents[selectedComponentIndex].position.y);
        }
      } else {
        // Clicked on empty space - clear selection or start drag selection
        if (!ImGui::GetIO().KeyCtrl) {
          clearAllSelections();
          selectedComponents.clear();
          selectedComponentIndex = -1;
        }

        // Start selection rectangle
        isSelecting = true;
        selectionStart = mousePos;
        selectionEnd = mousePos;
      }
    }

    // Right mouse button pressed - open edit popup or resistor popup
    // But only if mouse is not over the graph window and no popup is already
    // open
    bool isOverGraphWindowRight = false;
    ImGuiWindow* graphWindowRight =
        ImGui::FindWindowByName("AC Analysis Graph");
    if (graphWindowRight != nullptr) {
      isOverGraphWindowRight =
          ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    }

    // Don't process right-click if ComponentEditPopup is already open
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !isOverGraphWindowRight && !isEditPopupCurrentlyOpen) {
      // Right-click on canvas - set flag to open edit popup
      openEditPopupFromCanvas = true;
      printf("Right-clicked on canvas - flagging edit popup to open\n");
    }

    // Handle drag selection rectangle
    if (isSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      selectionEnd = mousePos;

      // Convert canvas-relative coordinates to screen coordinates for drawing
      ImVec2 screenStart = ImVec2(selectionStart.x + canvasPos.x,
                                  selectionStart.y + canvasPos.y);
      ImVec2 screenEnd =
          ImVec2(selectionEnd.x + canvasPos.x, selectionEnd.y + canvasPos.y);

      // Draw selection rectangle in screen coordinates
      ImVec2 rectMin = ImVec2(std::min(screenStart.x, screenEnd.x),
                              std::min(screenStart.y, screenEnd.y));
      ImVec2 rectMax = ImVec2(std::max(screenStart.x, screenEnd.x),
                              std::max(screenStart.y, screenEnd.y));

      drawList->AddRect(rectMin, rectMax, IM_COL32(66, 135, 245, 255), 0.0f, 0,
                        2.0f);
      drawList->AddRectFilled(rectMin, rectMax, IM_COL32(66, 135, 245, 60));

      // Update selection based on rectangle (use canvas-relative coordinates)
      ImVec2 canvasRectMin = ImVec2(std::min(selectionStart.x, selectionEnd.x),
                                    std::min(selectionStart.y, selectionEnd.y));
      ImVec2 canvasRectMax = ImVec2(std::max(selectionStart.x, selectionEnd.x),
                                    std::max(selectionStart.y, selectionEnd.y));
      updateRectangleSelection(canvasRectMin, canvasRectMax);
    }

    // Finish drag selection - keep components selected
    if (isSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      isSelecting = false;
      // Don't clear selection here - components remain selected
    }

    // Stop component dragging but keep selection
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      if (isDraggingComponent) {
        // Update ONLY node POSITIONS in maps for all moved components
        // Do NOT change node numbers during movement
        for (int compIndex : selectedComponents) {
          if (compIndex >= 0 && compIndex < visualComponents.size()) {
            VisualComponent& comp = visualComponents[compIndex];
            // Get the new node positions after movement
            auto [nodeAPos, nodeBPos] = getComponentNodePositions(comp);

            // Store updated positions in map - node numbers stay the same
            storeNodeAtPosition(comp.nodeA, nodeAPos);
            storeNodeAtPosition(comp.nodeB, nodeBPos);
            printf("Updated component %s positions after movement\n",
                   comp.name.c_str());
          }
        }

        // Update ONLY wire POSITIONS for all moved wires FIRST
        // Do NOT change node numbers during movement
        for (int wireIndex : selectedWires) {
          if (wireIndex >= 0 && wireIndex < visualWires.size()) {
            VisualWire& wire = visualWires[wireIndex];
            // Update wire start position in positionToWireCarryNodeNumber
            if (!wire.points.empty() && wire.carriedNodeNumber >= 0) {
              ImVec2 wireStartPos = wire.points[0];
              storeWireCarryNodeAtPosition(wire.carriedNodeNumber,
                                           wireStartPos);
              printf(
                  "Updated wire start position in "
                  "positionToWireCarryNodeNumber at (%.0f, %.0f)\n",
                  wireStartPos.x, wireStartPos.y);
            }

            // Update wire end position in positionToWireCarryNodeNumber
            if (wire.points.size() > 1 && wire.carriedNodeNumber >= 0) {
              ImVec2 wireEndPos = wire.points[wire.points.size() - 1];
              storeWireCarryNodeAtPosition(wire.carriedNodeNumber, wireEndPos);
              printf(
                  "Updated wire end position in "
                  "positionToWireCarryNodeNumber "
                  "at (%.0f, %.0f)\n",
                  wireEndPos.x, wireEndPos.y);
            }
          }
        }

        // After movement, update circuit with new component positions
        // Run node-number checker so components reconcile with any nearby
        // wire positions. If no matching wire position is found, components
        // retain their original node numbers.
        syncComponentNodesFromWireCarry();
        updateCircuitComponentNodes();
        ensureWireConsistency();
      }
      isDraggingComponent = false;
      // Don't clear selection here - components remain selected as a group
    }
  }

  /**
   * Returns the index of the component at the given position, or -1 if none.
   */
  int getComponentAtPosition(ImVec2 mousePos) {
    // Check components in reverse order for top-most selection
    for (int i = visualComponents.size() - 1; i >= 0; i--) {
      const VisualComponent& comp = visualComponents[i];
      ImVec2 compMin = ImVec2(comp.position.x - comp.size.x / 2,
                              comp.position.y - comp.size.y / 2);
      ImVec2 compMax = ImVec2(comp.position.x + comp.size.x / 2,
                              comp.position.y + comp.size.y / 2);

      if (mousePos.x >= compMin.x && mousePos.x <= compMax.x &&
          mousePos.y >= compMin.y && mousePos.y <= compMax.y) {
        return i;
      }
    }
    return -1;
  }

  /**
   * Clears selection state for all components and wires.
   */
  void clearAllSelections() {
    for (auto& comp : visualComponents) {
      comp.isSelected = false;
    }
    selectedComponents.clear();
    selectedWires.clear();
  }

  /**
   * Updates component and wire selection based on selection rectangle.
   */
  void updateRectangleSelection(ImVec2 rectMin, ImVec2 rectMax) {
    if (!ImGui::GetIO().KeyCtrl) {
      clearAllSelections();
    }

    // Select components that intersect with selection rectangle
    for (int i = 0; i < visualComponents.size(); i++) {
      const VisualComponent& comp = visualComponents[i];
      ImVec2 compMin = ImVec2(comp.position.x - comp.size.x / 2,
                              comp.position.y - comp.size.y / 2);
      ImVec2 compMax = ImVec2(comp.position.x + comp.size.x / 2,
                              comp.position.y + comp.size.y / 2);

      // Check if component intersects with selection rectangle
      bool intersects = !(compMax.x < rectMin.x || compMin.x > rectMax.x ||
                          compMax.y < rectMin.y || compMin.y > rectMax.y);

      if (intersects) {
        if (std::find(selectedComponents.begin(), selectedComponents.end(),
                      i) == selectedComponents.end()) {
          selectedComponents.push_back(i);
          visualComponents[i].isSelected = true;
        }
      }
    }

    // Select wires that have any point within the selection rectangle
    for (int i = 0; i < visualWires.size(); i++) {
      const VisualWire& wire = visualWires[i];
      if (wire.points.size() < 2) continue;

      bool wireSelected = false;

      // Check if any wire point is within the selection rectangle
      for (const ImVec2& point : wire.points) {
        if (point.x >= rectMin.x && point.x <= rectMax.x &&
            point.y >= rectMin.y && point.y <= rectMax.y) {
          wireSelected = true;
          break;
        }
      }

      if (wireSelected) {
        if (std::find(selectedWires.begin(), selectedWires.end(), i) ==
            selectedWires.end()) {
          selectedWires.push_back(i);
        }
      }
    }
  }

  /**
   * Snaps a coordinate to the nearest grid point.
   */
  ImVec2 snapToGrid(ImVec2 position) {
    int snapX =
        ((int)(position.x + gridSpacing / 2) / gridSpacing) * gridSpacing;
    int snapY =
        ((int)(position.y + gridSpacing / 2) / gridSpacing) * gridSpacing;
    return ImVec2(snapX, snapY);
  }

  // ============================================================================
  // NODE CONNECTION SYSTEM FUNCTIONS
  // ============================================================================

  /**
   * Checks if a component is a ground component.
   * @param comp The visual component to check
   * @return true if component is ground type, false otherwise
   */
  bool isGroundComponent(const VisualComponent& comp) {
    return comp.type == "ground";
  }

  /**
   * Determines the polarity of a component node based on component type.
   * Regular components: NodeA = positive, NodeB = negative
   * Voltage sources: NodeA = negative, NodeB = positive
   * @param comp The component to check
   * @param nodeIndex Node index (0 = NodeA, 1 = NodeB)
   * @return "+" for positive, "-" for negative
   */
  std::string getNodePolarity(const VisualComponent& comp, int nodeIndex) {
    if (comp.type == "ground") {
      // Ground components have no polarity - they are reference
      return "";
    } else if (comp.type == "voltage_source") {
      // Voltage source: NodeA = negative, NodeB = positive
      return (nodeIndex == 1) ? "+" : "-";
    } else {
      // Regular components: NodeA = positive, NodeB = negative
      return (nodeIndex == 0) ? "+" : "-";
    }
  }

  /**
   * Gets the current node number for a specific component node.
   * @param componentIndex Index of the component in visualComponents
   * @param nodeIndex Node index (0 = NodeA, 1 = NodeB)
   * @return Node number (0 = ground, >0 = assigned number, -1 = invalid
   * component)
   */
  int getNodeNumber(int componentIndex, int nodeIndex) {
    if (componentIndex < 0 || componentIndex >= visualComponents.size()) {
      return -1;  // Invalid component index
    }

    std::pair<int, int> componentNode = {componentIndex, nodeIndex};
    auto it = componentNodeToNumber.find(componentNode);

    // If not found in map, check if it's a ground component
    if (it == componentNodeToNumber.end()) {
      if (isGroundComponent(visualComponents[componentIndex])) {
        return 0;  // Ground components are always node 0
      }
      return 0;  // Unconnected nodes default to 0
    }

    return it->second;
  }

  /**
   * Sets a component node to ground (node 0) and updates the mapping.
   * @param componentIndex Index of the component
   * @param nodeIndex Node index (0 = NodeA, 1 = NodeB)
   */
  void setNodeToGround(int componentIndex, int nodeIndex) {
    if (componentIndex >= 0 && componentIndex < visualComponents.size()) {
      componentNodeToNumber[{componentIndex, nodeIndex}] = 0;

      // Update the component's node values
      if (nodeIndex == 0) {
        visualComponents[componentIndex].nodeA = 0;
      } else {
        visualComponents[componentIndex].nodeB = 0;
      }

      printf("Set component %s node %s to ground (0)\n",
             visualComponents[componentIndex].name.c_str(),
             (nodeIndex == 0) ? "A" : "B");
    }
  }

  /**
   * Converts all nodes with a specific node number to ground (0).
   * Used when connecting an existing network to ground.
   * @param nodeNumber The node number to convert to ground
   */
  void convertNodeToGround(int nodeNumber) {
    if (nodeNumber <= 0) return;  // Already ground or invalid

    printf("Converting all nodes with number %d to ground (0)\n", nodeNumber);

    // Update all mappings with this node number to 0
    for (auto& [componentNode, currentNumber] : componentNodeToNumber) {
      if (currentNumber == nodeNumber) {
        currentNumber = 0;

        // Update the actual component values
        int compIndex = componentNode.first;
        int nodeIndex = componentNode.second;
        if (compIndex < visualComponents.size()) {
          if (nodeIndex == 0) {
            visualComponents[compIndex].nodeA = 0;
          } else {
            visualComponents[compIndex].nodeB = 0;
          }
        }
      }
    }

    // Remove the connection from nodeConnections
    nodeConnections.erase(
        std::remove_if(nodeConnections.begin(), nodeConnections.end(),
                       [nodeNumber](const NodeConnection& conn) {
                         return conn.nodeNumber == nodeNumber;
                       }),
        nodeConnections.end());
  }

  /**
   * Checks if a connection involves a ground component.
   * @param comp1Index Index of first component
   * @param node1Index Node index of first component
   * @param comp2Index Index of second component
   * @param node2Index Node index of second component
   * @return true if either component is ground, false otherwise
   */
  bool connectionIncludesGround(int comp1Index, int node1Index, int comp2Index,
                                int node2Index) {
    bool comp1IsGround =
        (comp1Index >= 0 && comp1Index < visualComponents.size() &&
         isGroundComponent(visualComponents[comp1Index]));
    bool comp2IsGround =
        (comp2Index >= 0 && comp2Index < visualComponents.size() &&
         isGroundComponent(visualComponents[comp2Index]));

    return comp1IsGround || comp2IsGround;
  }

  /**
   * Handles ground connections by propagating ground to all connected nodes.
   * @param comp1Index Index of first component
   * @param node1Index Node index of first component (0=A, 1=B)
   * @param comp2Index Index of second component
   * @param node2Index Node index of second component (0=A, 1=B)
   */
  void propagateGroundConnection(int comp1Index, int node1Index, int comp2Index,
                                 int node2Index) {
    printf("Propagating ground connection between components %d-%s and %d-%s\n",
           comp1Index, (node1Index == 0) ? "A" : "B", comp2Index,
           (node2Index == 0) ? "A" : "B");

    // Get existing node numbers for both nodes
    int existingNode1 = getNodeNumber(comp1Index, node1Index);
    int existingNode2 = getNodeNumber(comp2Index, node2Index);

    // Convert any existing non-ground networks to ground
    if (existingNode1 > 0) {
      convertNodeToGround(existingNode1);
    }
    if (existingNode2 > 0) {
      convertNodeToGround(existingNode2);
    }

    // Set both nodes to ground
    setNodeToGround(comp1Index, node1Index);
    setNodeToGround(comp2Index, node2Index);
  }

  /**
   * Handles regular (non-ground) connections between component nodes.
   * @param comp1Index Index of first component
   * @param node1Index Node index of first component (0=A, 1=B)
   * @param comp2Index Index of second component
   * @param node2Index Node index of second component (0=A, 1=B)
   */
  void createRegularConnection(int comp1Index, int node1Index, int comp2Index,
                               int node2Index) {
    int existingNode1 = getNodeNumber(comp1Index, node1Index);
    int existingNode2 = getNodeNumber(comp2Index, node2Index);

    // This function should only be called for non-ground connections
    // Ground connections are handled by connectionIncludesGround check before
    // this So we can proceed with regular node assignment without additional
    // ground checks

    int targetNodeNumber = -1;

    if (existingNode1 > 0 && existingNode2 > 0) {
      // Both nodes already have numbers - merge the networks
      targetNodeNumber = std::min(existingNode1, existingNode2);
      int mergeNodeNumber = std::max(existingNode1, existingNode2);

      // Update all nodes with the higher number to use the lower number
      for (auto& [componentNode, nodeNumber] : componentNodeToNumber) {
        if (nodeNumber == mergeNodeNumber) {
          nodeNumber = targetNodeNumber;
        }
      }

      // Merge connections
      for (auto& connection : nodeConnections) {
        if (connection.nodeNumber == mergeNodeNumber) {
          // Find the target connection and merge
          for (auto& targetConnection : nodeConnections) {
            if (targetConnection.nodeNumber == targetNodeNumber) {
              targetConnection.connectedNodes.insert(
                  connection.connectedNodes.begin(),
                  connection.connectedNodes.end());
              break;
            }
          }
        }
      }

      // Remove the merged connection
      nodeConnections.erase(
          std::remove_if(nodeConnections.begin(), nodeConnections.end(),
                         [mergeNodeNumber](const NodeConnection& conn) {
                           return conn.nodeNumber == mergeNodeNumber;
                         }),
          nodeConnections.end());

      printf("Merged node networks %d and %d into node %d\n", existingNode1,
             existingNode2, targetNodeNumber);

    } else if (existingNode1 > 0) {
      // First node has a number, add second node to same network
      targetNodeNumber = existingNode1;

    } else if (existingNode2 > 0) {
      // Second node has a number, add first node to same network
      targetNodeNumber = existingNode2;

    } else {
      // Neither node has a number, create new network
      targetNodeNumber = nextNodeNumber++;

      // Create new connection
      NodeConnection newConnection(targetNodeNumber);
      nodeConnections.push_back(newConnection);

      printf("Created new node network %d\n", targetNodeNumber);
    }

    // Update mappings and component values
    componentNodeToNumber[{comp1Index, node1Index}] = targetNodeNumber;
    componentNodeToNumber[{comp2Index, node2Index}] = targetNodeNumber;

    // Update component node values
    if (node1Index == 0) {
      visualComponents[comp1Index].nodeA = targetNodeNumber;
    } else {
      visualComponents[comp1Index].nodeB = targetNodeNumber;
    }

    if (node2Index == 0) {
      visualComponents[comp2Index].nodeA = targetNodeNumber;
    } else {
      visualComponents[comp2Index].nodeB = targetNodeNumber;
    }

    // Update the connection's node list
    for (auto& connection : nodeConnections) {
      if (connection.nodeNumber == targetNodeNumber) {
        connection.connectedNodes.insert({comp1Index, node1Index});
        connection.connectedNodes.insert({comp2Index, node2Index});
        break;
      }
    }

    printf("Connected components %d-%s and %d-%s to node %d\n", comp1Index,
           (node1Index == 0) ? "A" : "B", comp2Index,
           (node2Index == 0) ? "A" : "B", targetNodeNumber);
  }

  /**
   * Creates a connection between two component nodes.
   * Automatically handles ground propagation and network merging.
   * @param comp1Index Index of first component
   * @param node1Index Node index of first component (0=A, 1=B)
   * @param comp2Index Index of second component
   * @param node2Index Node index of second component (0=A, 1=B)
   */
  void createNodeConnection(int comp1Index, int node1Index, int comp2Index,
                            int node2Index) {
    printf(
        "DEBUG: createNodeConnection called with comp1=%d, node1=%d, "
        "comp2=%d, "
        "node2=%d\n",
        comp1Index, node1Index, comp2Index, node2Index);

    // Validate input parameters
    if (comp1Index < 0 || comp1Index >= visualComponents.size() ||
        comp2Index < 0 || comp2Index >= visualComponents.size() ||
        node1Index < 0 || node1Index > 1 || node2Index < 0 || node2Index > 1) {
      printf(
          "Invalid connection parameters: comp1=%d, node1=%d, comp2=%d, "
          "node2=%d\n",
          comp1Index, node1Index, comp2Index, node2Index);
      return;
    }

    printf("DEBUG: Connection is valid, components: %s and %s\n",
           visualComponents[comp1Index].name.c_str(),
           visualComponents[comp2Index].name.c_str());

    // Check if connection involves ground
    if (connectionIncludesGround(comp1Index, node1Index, comp2Index,
                                 node2Index)) {
      propagateGroundConnection(comp1Index, node1Index, comp2Index, node2Index);
    } else {
      createRegularConnection(comp1Index, node1Index, comp2Index, node2Index);
    }

    // Update all component node values to ensure consistency
    updateAllComponentNodeValues();

    // Update circuit component nodes when connections change
    updateCircuitComponentNodes();
  }

  /**
   * Updates all component nodeA and nodeB values based on current
   * connections. Should be called after any connection changes to ensure
   * consistency.
   */
  void updateAllComponentNodeValues() {
    for (int i = 0; i < visualComponents.size(); i++) {
      visualComponents[i].nodeA = getNodeNumber(i, 0);
      visualComponents[i].nodeB = getNodeNumber(i, 1);
      // Do not force ground components to node 0 unless actually connected
    }
  }

  /**
   * Initializes a newly placed component for node connection system.
   * Sets up ground components properly and assigns default node values.
   * @param componentIndex Index of the component that was just placed
   */
  void initializeComponentNodes(int componentIndex) {
    if (componentIndex < 0 || componentIndex >= visualComponents.size()) {
      return;
    }

    auto& comp = visualComponents[componentIndex];

    // Preserve the node numbers already assigned to the component
    // Only initialize the componentNodeToNumber map with existing values
    if (isGroundComponent(comp)) {
      // Ground stays at node 0
      componentNodeToNumber[{componentIndex, 0}] = 0;
      componentNodeToNumber[{componentIndex, 1}] = 0;
      printf(
          "Initialized ground component %s with node 0 (preserving existing "
          "assignment)\n",
          comp.name.c_str());
    } else {
      // Preserve the already-assigned node numbers from component creation
      componentNodeToNumber[{componentIndex, 0}] = comp.nodeA;
      componentNodeToNumber[{componentIndex, 1}] = comp.nodeB;
      printf(
          "Initialized component %s with preserved nodes: nodeA=%d, "
          "nodeB=%d\n",
          comp.name.c_str(), comp.nodeA, comp.nodeB);
    }

    // Rebuild circuit to include the new component
    updateCircuitComponentNodes();
  }

  /**
   * Finds an existing wire that is close to the given point.
   * @param point The point to check
   * @param tolerance Maximum distance to consider "close"
   * @return Index of the wire if found, -1 if no wire is close enough
   */
  int findWireAtPoint(ImVec2 point, float tolerance = 10.0f) {
    for (int wireIndex = 0; wireIndex < visualWires.size(); wireIndex++) {
      const auto& wire = visualWires[wireIndex];

      // Check each segment of the wire
      for (int i = 0; i < wire.points.size() - 1; i++) {
        ImVec2 segmentStart = wire.points[i];
        ImVec2 segmentEnd = wire.points[i + 1];

        // Calculate distance from point to line segment
        float dist =
            distanceFromPointToLineSegment(point, segmentStart, segmentEnd);

        if (dist <= tolerance) {
          printf("Found wire %d at distance %.2f from point (%.1f, %.1f)\n",
                 wireIndex, dist, point.x, point.y);
          return wireIndex;
        }
      }
    }
    return -1;
  }

  /**
   * Calculates the distance from a point to a line segment.
   */
  float distanceFromPointToLineSegment(ImVec2 point, ImVec2 lineStart,
                                       ImVec2 lineEnd) {
    ImVec2 line = ImVec2(lineEnd.x - lineStart.x, lineEnd.y - lineStart.y);
    ImVec2 pointToStart = ImVec2(point.x - lineStart.x, point.y - lineStart.y);

    float lineLength = sqrt(line.x * line.x + line.y * line.y);
    if (lineLength == 0) {
      // Line segment is just a point
      return sqrt(pointToStart.x * pointToStart.x +
                  pointToStart.y * pointToStart.y);
    }

    // Project point onto line
    float projection = (pointToStart.x * line.x + pointToStart.y * line.y) /
                       (lineLength * lineLength);

    if (projection < 0) {
      // Point is closest to line start
      return sqrt(pointToStart.x * pointToStart.x +
                  pointToStart.y * pointToStart.y);
    } else if (projection > 1) {
      // Point is closest to line end
      ImVec2 pointToEnd = ImVec2(point.x - lineEnd.x, point.y - lineEnd.y);
      return sqrt(pointToEnd.x * pointToEnd.x + pointToEnd.y * pointToEnd.y);
    } else {
      // Point projects onto the line segment
      ImVec2 projected = ImVec2(lineStart.x + projection * line.x,
                                lineStart.y + projection * line.y);
      ImVec2 pointToProjected =
          ImVec2(point.x - projected.x, point.y - projected.y);
      return sqrt(pointToProjected.x * pointToProjected.x +
                  pointToProjected.y * pointToProjected.y);
    }
  }

  /**
   * Gets the node number associated with an existing wire.
   * @param wireIndex Index of the wire in visualWires
   * @return Node number of the wire, or 0 if no node assigned
   */
  int getWireNodeNumber(int wireIndex) {
    if (wireIndex < 0 || wireIndex >= visualWires.size()) {
      return 0;
    }

    const auto& wire = visualWires[wireIndex];

    // FIRST: Check the wire's carried node number - this is the primary
    // source of truth
    if (wire.carriedNodeNumber > 0) {
      printf("Wire %d carries node number %d\n", wireIndex,
             wire.carriedNodeNumber);
      return wire.carriedNodeNumber;
    }

    // SECOND: Check the start component node (fallback)
    if (wire.startComponentIndex != -1) {
      int startNodeNumber =
          getNodeNumber(wire.startComponentIndex, wire.startNodeIndex);
      if (startNodeNumber > 0) {
        printf("Wire %d has node number %d from start component\n", wireIndex,
               startNodeNumber);
        return startNodeNumber;
      }
    }

    // THIRD: Check the end component node (fallback)
    if (wire.endComponentIndex != -1) {
      int endNodeNumber =
          getNodeNumber(wire.endComponentIndex, wire.endNodeIndex);
      if (endNodeNumber > 0) {
        printf("Wire %d has node number %d from end component\n", wireIndex,
               endNodeNumber);
        return endNodeNumber;
      }
    }

    printf("Wire %d has no assigned node number\n", wireIndex);
    return 0;
  }

  /**
   * Connects a component node to an existing wire's node network.
   * @param componentIndex Index of the component
   * @param nodeIndex Node index of the component (0=A, 1=B)
   * @param wireNodeNumber Node number from the existing wire
   */
  void connectComponentToWireNode(int componentIndex, int nodeIndex,
                                  int wireNodeNumber) {
    if (wireNodeNumber < 0) {
      printf("Cannot connect to wire node: invalid node number %d\n",
             wireNodeNumber);
      return;
    }

    int compNodeNumber = getNodeNumber(componentIndex, nodeIndex);
    printf("Connecting component %s node %s (node %d) to wire node %d\n",
           visualComponents[componentIndex].name.c_str(),
           (nodeIndex == 0) ? "A" : "B", compNodeNumber, wireNodeNumber);

    // If either the component node or the wire node is GND, propagate GND to
    // the whole network
    if (compNodeNumber == 0 && wireNodeNumber > 0) {
      // Component is GND, wire is not: convert wire's network to GND
      convertNodeToGround(wireNodeNumber);
      wireNodeNumber = 0;
    } else if (wireNodeNumber == 0 && compNodeNumber > 0) {
      // Wire is GND, component is not: convert component's network to GND
      convertNodeToGround(compNodeNumber);
      compNodeNumber = 0;
    }

    // Update the component node mapping
    componentNodeToNumber[{componentIndex, nodeIndex}] = wireNodeNumber;

    // Update the component's node value
    if (nodeIndex == 0) {
      visualComponents[componentIndex].nodeA = wireNodeNumber;
    } else {
      visualComponents[componentIndex].nodeB = wireNodeNumber;
    }

    // Store the wire node number at the component's position
    VisualComponent& comp = visualComponents[componentIndex];
    auto [nodeAPos, nodeBPos] = getComponentNodePositions(comp);
    ImVec2 nodePos = (nodeIndex == 0) ? nodeAPos : nodeBPos;
    storeNodeAtPosition(wireNodeNumber, nodePos);
    // here we also store in wire position map for consistency
    storeWireCarryNodeAtPosition(wireNodeNumber, nodePos);
    printf("Stored wire node %d at component position (%.0f, %.0f)\n",
           wireNodeNumber, nodePos.x, nodePos.y);

    // Find the existing node connection and add this component to it
    for (auto& connection : nodeConnections) {
      if (connection.nodeNumber == wireNodeNumber) {
        connection.connectedNodes.insert({componentIndex, nodeIndex});
        printf("Added component %s node %s to existing node network %d\n",
               visualComponents[componentIndex].name.c_str(),
               (nodeIndex == 0) ? "A" : "B", wireNodeNumber);
        return;
      }
    }

    // If no existing connection found, create a new one
    NodeConnection newConnection(wireNodeNumber);
    newConnection.connectedNodes.insert({componentIndex, nodeIndex});
    nodeConnections.push_back(newConnection);

    printf("Created new node network %d for component %s node %s\n",
           wireNodeNumber, visualComponents[componentIndex].name.c_str(),
           (nodeIndex == 0) ? "A" : "B");
  }

  /**
   * Updates circuit component node assignments based on current visual
   * connections. Called when wire connections change node assignments.
   */
  void updateCircuitComponentNodes() {
    // Clear the existing circuit and rebuild with current values and
    // connections
    circuit = Circuit();

    // Rebuild circuit with current visual component values and node
    // assignments
    for (int i = 0; i < visualComponents.size(); i++) {
      const auto& visualComp = visualComponents[i];
      int nodeA = getNodeNumber(i, 0);
      int nodeB = getNodeNumber(i, 1);

      printf("DEBUG: Visual component %s (type: %s) has value %.6f\n",
             visualComp.name.c_str(), visualComp.type.c_str(),
             visualComp.value);

      // Add component to circuit with current nodes and values
      if (visualComp.type == "resistor") {
        circuit.addResistor(visualComp.name, visualComp.value, nodeA, nodeB);

        printf("Rebuilt resistor %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);

      } else if (visualComp.type == "inductor") {
        circuit.addInductor(visualComp.name, visualComp.value, nodeA, nodeB);
        printf("Rebuilt inductor %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      } else if (visualComp.type == "capacitor") {
        circuit.addCapacitor(visualComp.name, visualComp.value, nodeA, nodeB);
        printf("Rebuilt capacitor %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      } else if (visualComp.type == "voltage_source") {
        circuit.addVoltageSource(visualComp.name, visualComp.value, nodeA,
                                 nodeB);
        printf("Rebuilt voltage source %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      } else if (visualComp.type == "ac_voltage_source") {
        circuit.addACVoltageSource(visualComp.name, visualComp.value, nodeA,
                                   nodeB);
        printf("Rebuilt AC voltage source %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      } else if (visualComp.type == "dc_current_source") {
        circuit.addDCCurrentSource(visualComp.name, visualComp.value, nodeA,
                                   nodeB);
        printf("Rebuilt current source %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      } else if (visualComp.type == "ac_current_source") {
        circuit.addACCurrentSource(visualComp.name, visualComp.value, nodeA,
                                   nodeB);
        printf("Rebuilt AC current source %s: nodes [%d,%d] value %.6f\n",
               visualComp.name.c_str(), nodeA, nodeB, visualComp.value);
      }
      // Note: Ground components are not added to circuit as they're just
      // reference points
    }

    printf("Circuit rebuilt with %zu components\n",
           circuit.getComponents().size());
  }

  /**
   * Handles dragging of selected components with grid snapping.
   */
  void handleComponentDragging(ImVec2 screenPos, ImVec2 canvasPos) {
    // Extended canvas area for interaction
    float extendedWidth = A4_WIDTH * 3.0f;
    float extendedHeight = A4_HEIGHT * 2.0f;
    bool mouseInCanvas = (screenPos.x >= canvasPos.x &&
                          screenPos.x <= canvasPos.x + extendedWidth &&
                          screenPos.y >= canvasPos.y &&
                          screenPos.y <= canvasPos.y + extendedHeight);

    if (isDraggingComponent && !selectedComponents.empty() &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left) && mouseInCanvas) {
      // Convert screen coordinates to canvas-relative coordinates
      ImVec2 mousePos =
          ImVec2(screenPos.x - canvasPos.x, screenPos.y - canvasPos.y);

      // Calculate movement delta
      ImVec2 newPos =
          ImVec2(mousePos.x - dragOffset.x, mousePos.y - dragOffset.y);

      // Snap to grid
      int snapX =
          ((int)(newPos.x + gridSpacing / 2) / gridSpacing) * gridSpacing;
      int snapY =
          ((int)(newPos.y + gridSpacing / 2) / gridSpacing) * gridSpacing;
      ImVec2 snappedPos = ImVec2(snapX, snapY);

      // Get reference component for calculating delta
      if (selectedComponentIndex >= 0 &&
          selectedComponentIndex < visualComponents.size()) {
        ImVec2 delta = ImVec2(
            snappedPos.x - visualComponents[selectedComponentIndex].position.x,
            snappedPos.y - visualComponents[selectedComponentIndex].position.y);

        // Move all selected components by the same delta
        for (int compIndex : selectedComponents) {
          if (compIndex >= 0 && compIndex < visualComponents.size()) {
            visualComponents[compIndex].position.x += delta.x;
            visualComponents[compIndex].position.y += delta.y;
          }
        }

        // Move all selected wires by the same delta
        for (int wireIndex : selectedWires) {
          if (wireIndex >= 0 && wireIndex < visualWires.size()) {
            for (ImVec2& point : visualWires[wireIndex].points) {
              point.x += delta.x;
              point.y += delta.y;
            }
          }
        }
      }
    }
  }

  /**
   * Handles rotation of selected components with Ctrl+R.
   * Rotates the entire component including its visual representation and node
   * positions while maintaining original component dimensions.
   */
  void handleComponentRotation() {
    if (!selectedComponents.empty() && ImGui::GetIO().KeyCtrl &&
        ImGui::IsKeyPressed(ImGuiKey_R)) {
      for (int compIndex : selectedComponents) {
        if (compIndex >= 0 && compIndex < visualComponents.size()) {
          VisualComponent& comp = visualComponents[compIndex];

          // Update component rotation
          comp.rotation += 90.0f;
          if (comp.rotation >= 360.0f) {
            comp.rotation = 0.0f;
          }

          // Update label position based on new rotation (only for passive
          // components)
          if (comp.type == "resistor" || comp.type == "inductor" ||
              comp.type == "capacitor") {
            comp.labelOffset = getDefaultLabelOffset(comp.type, comp.rotation);
            comp.hasCustomLabelPos = false;  // Reset custom positioning
          }

          // Don't swap dimensions - let SFML handle the visual rotation
          // The component size should remain constant, only rotation changes
        }
      }
    }
  }

  /**
   * Draws a bounding box around all selected components and wires to show
   * they are grouped.
   */
  void drawGroupBoundingBox(ImDrawList* drawList) {
    if (selectedComponents.empty() && selectedWires.empty()) return;

    // Calculate bounding box for all selected components and wires
    ImVec2 minPos = ImVec2(FLT_MAX, FLT_MAX);
    ImVec2 maxPos = ImVec2(-FLT_MAX, -FLT_MAX);

    // Include selected components in bounding box
    for (int compIndex : selectedComponents) {
      if (compIndex >= 0 && compIndex < visualComponents.size()) {
        const VisualComponent& comp = visualComponents[compIndex];
        ImVec2 compMin = ImVec2(comp.position.x - comp.size.x / 2 - 5,
                                comp.position.y - comp.size.y / 2 - 5);
        ImVec2 compMax = ImVec2(comp.position.x + comp.size.x / 2 + 5,
                                comp.position.y + comp.size.y / 2 + 5);

        minPos.x = std::min(minPos.x, compMin.x);
        minPos.y = std::min(minPos.y, compMin.y);
        maxPos.x = std::max(maxPos.x, compMax.x);
        maxPos.y = std::max(maxPos.y, compMax.y);
      }
    }

    // Include selected wires in bounding box
    for (int wireIndex : selectedWires) {
      if (wireIndex >= 0 && wireIndex < visualWires.size()) {
        const VisualWire& wire = visualWires[wireIndex];
        for (const ImVec2& point : wire.points) {
          minPos.x = std::min(minPos.x, point.x - 5);
          minPos.y = std::min(minPos.y, point.y - 5);
          maxPos.x = std::max(maxPos.x, point.x + 5);
          maxPos.y = std::max(maxPos.y, point.y + 5);
        }
      }
    }

    // Add padding around the group
    float padding = 10.0f;
    minPos.x -= padding;
    minPos.y -= padding;
    maxPos.x += padding;
    maxPos.y += padding;

    // Draw dashed bounding box
    ImU32 groupBoxColor = IM_COL32(66, 135, 245, 180);
    ImU32 groupFillColor = IM_COL32(66, 135, 245, 30);

    // Fill background
    drawList->AddRectFilled(minPos, maxPos, groupFillColor);

    // Draw dashed border
    float dashLength = 8.0f;
    float gapLength = 4.0f;

    // Top edge
    drawDashedLine(drawList, ImVec2(minPos.x, minPos.y),
                   ImVec2(maxPos.x, minPos.y), groupBoxColor, 2.0f, dashLength,
                   gapLength);
    // Bottom edge
    drawDashedLine(drawList, ImVec2(minPos.x, maxPos.y),
                   ImVec2(maxPos.x, maxPos.y), groupBoxColor, 2.0f, dashLength,
                   gapLength);
    // Left edge
    drawDashedLine(drawList, ImVec2(minPos.x, minPos.y),
                   ImVec2(minPos.x, maxPos.y), groupBoxColor, 2.0f, dashLength,
                   gapLength);
    // Right edge
    drawDashedLine(drawList, ImVec2(maxPos.x, minPos.y),
                   ImVec2(maxPos.x, maxPos.y), groupBoxColor, 2.0f, dashLength,
                   gapLength);

    // Draw corner handles to show it's a group
    float handleSize = 6.0f;
    ImU32 handleColor = IM_COL32(255, 255, 255, 255);

    // Corner handles
    drawList->AddRectFilled(
        ImVec2(minPos.x - handleSize / 2, minPos.y - handleSize / 2),
        ImVec2(minPos.x + handleSize / 2, minPos.y + handleSize / 2),
        handleColor);
    drawList->AddRectFilled(
        ImVec2(maxPos.x - handleSize / 2, minPos.y - handleSize / 2),
        ImVec2(maxPos.x + handleSize / 2, minPos.y + handleSize / 2),
        handleColor);
    drawList->AddRectFilled(
        ImVec2(minPos.x - handleSize / 2, maxPos.y - handleSize / 2),
        ImVec2(minPos.x + handleSize / 2, maxPos.y + handleSize / 2),
        handleColor);
    drawList->AddRectFilled(
        ImVec2(maxPos.x - handleSize / 2, maxPos.y - handleSize / 2),
        ImVec2(maxPos.x + handleSize / 2, maxPos.y + handleSize / 2),
        handleColor);
  }

  /**
   * Draws a dashed line between two points.
   */
  void drawDashedLine(ImDrawList* drawList, ImVec2 start, ImVec2 end,
                      ImU32 color, float thickness, float dashLength,
                      float gapLength) {
    ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
    float totalLength =
        sqrtf(direction.x * direction.x + direction.y * direction.y);

    if (totalLength < 0.001f) return;

    direction.x /= totalLength;
    direction.y /= totalLength;

    float currentPos = 0.0f;
    bool drawing = true;

    while (currentPos < totalLength) {
      float segmentLength = drawing ? dashLength : gapLength;
      float nextPos = std::min(currentPos + segmentLength, totalLength);

      if (drawing) {
        ImVec2 segmentStart = ImVec2(start.x + direction.x * currentPos,
                                     start.y + direction.y * currentPos);
        ImVec2 segmentEnd = ImVec2(start.x + direction.x * nextPos,
                                   start.y + direction.y * nextPos);
        drawList->AddLine(segmentStart, segmentEnd, color, thickness);
      }

      currentPos = nextPos;
      drawing = !drawing;
    }
  }

  /**
   * Saves the current circuit to a file in a custom text format.
   * Saves components and wires with all their properties.
   */
  void saveCircuitToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      printf("ERROR: Could not open file for saving: %s\n", filename.c_str());
      return;
    }

    // Write file header
    file << "# Circuit Simulator File Format v2.0\n";
    file << "# Complete circuit state with component positions, properties, "
            "wires, and node mappings\n\n";

    // ========== SAVE COMPONENTS ==========
    file << "[COMPONENTS]\n";
    file << visualComponents.size() << "\n";
    for (const auto& comp : visualComponents) {
      // Format:
      // name|type|posX|posY|sizeX|sizeY|rotation|value|nodeA|nodeB|labelOffsetX|labelOffsetY|hasCustomLabel
      file << comp.name << "|" << comp.type << "|" << comp.position.x << "|"
           << comp.position.y << "|" << comp.size.x << "|" << comp.size.y << "|"
           << comp.rotation << "|" << comp.value << "|" << comp.nodeA << "|"
           << comp.nodeB << "|" << comp.labelOffset.x << "|"
           << comp.labelOffset.y << "|" << (comp.hasCustomLabelPos ? 1 : 0)
           << "\n";
    }

    // ========== SAVE WIRES ==========
    file << "\n[WIRES]\n";
    file << visualWires.size() << "\n";
    for (const auto& wire : visualWires) {
      // Format:
      // numPoints|startCompIdx|startNodeIdx|endCompIdx|endNodeIdx|thickness|carriedNodeNumber|color
      file << wire.points.size() << "|" << wire.startComponentIndex << "|"
           << wire.startNodeIndex << "|" << wire.endComponentIndex << "|"
           << wire.endNodeIndex << "|" << wire.thickness << "|"
           << wire.carriedNodeNumber << "|" << wire.color << "\n";

      // Save all points of the wire (format: x,y x,y x,y ...)
      for (const auto& point : wire.points) {
        file << point.x << "," << point.y << " ";
      }
      file << "\n";
    }

    // ========== SAVE NODE SYSTEM STATE ==========
    file << "\n[NODE_STATE]\n";
    file << nextNodeNumber << "\n";

    // Save positionToNodeNumber map (position-based node assignments for
    // components)
    file << positionToNodeNumber.size() << "\n";
    for (const auto& entry : positionToNodeNumber) {
      // Format: posX,posY:nodeNumber
      file << entry.first.first << "," << entry.first.second << ":"
           << entry.second << "\n";
    }

    // Save positionToWireCarryNodeNumber map (position-based node assignments
    // for wires)
    file << positionToWireCarryNodeNumber.size() << "\n";
    for (const auto& entry : positionToWireCarryNodeNumber) {
      // Format: posX,posY:nodeNumber
      file << entry.first.first << "," << entry.first.second << ":"
           << entry.second << "\n";
    }

    // ========== SAVE COMPONENT NODE MAPPING ==========
    file << "\n[COMPONENT_NODE_MAPPING]\n";
    file << componentNodeToNumber.size() << "\n";
    for (const auto& entry : componentNodeToNumber) {
      // Format: compIdx,nodeIdx:nodeNumber
      file << entry.first.first << "," << entry.first.second << ":"
           << entry.second << "\n";
    }

    file.close();
    printf("✓ Circuit saved to: %s\n", filename.c_str());
    printf("  Components: %lu, Wires: %lu, Nodes: %d\n",
           visualComponents.size(), visualWires.size(), nextNodeNumber - 1);
  }

  /**
   * Loads a circuit from a file.
   * Clears current circuit and loads components and wires.
   */
  void loadCircuitFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      printf("ERROR: Could not open file for loading: %s\n", filename.c_str());
      return;
    }

    // Clear current circuit
    visualComponents.clear();
    visualWires.clear();
    selectedComponents.clear();
    selectedWires.clear();
    nodeConnections.clear();
    componentNodeToNumber.clear();
    positionToNodeNumber.clear();
    positionToWireCarryNodeNumber.clear();
    nextNodeNumber = 1;

    std::string line;

    // ========== LOAD COMPONENTS ==========
    // Skip header lines
    while (std::getline(file, line) &&
           line.find("[COMPONENTS]") == std::string::npos) {
    }

    int numComponents;
    file >> numComponents;
    file.ignore();  // Skip newline

    for (int i = 0; i < numComponents; i++) {
      std::getline(file, line);
      std::istringstream iss(line);
      std::string token;
      std::vector<std::string> tokens;

      while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
      }

      if (tokens.size() >= 13) {
        VisualComponent comp(tokens[0], tokens[1], ImVec2(0, 0));
        comp.name = tokens[0];
        comp.type = tokens[1];
        comp.position.x = std::stof(tokens[2]);
        comp.position.y = std::stof(tokens[3]);
        comp.size.x = std::stof(tokens[4]);
        comp.size.y = std::stof(tokens[5]);
        comp.rotation = std::stof(tokens[6]);
        comp.value = std::stof(tokens[7]);
        comp.nodeA = std::stoi(tokens[8]);
        comp.nodeB = std::stoi(tokens[9]);
        comp.labelOffset.x = std::stof(tokens[10]);
        comp.labelOffset.y = std::stof(tokens[11]);
        comp.hasCustomLabelPos = (std::stoi(tokens[12]) == 1);

        visualComponents.push_back(comp);
      }
    }

    // ========== LOAD WIRES ==========
    // Skip to wires section
    while (std::getline(file, line) &&
           line.find("[WIRES]") == std::string::npos) {
    }

    int numWires;
    file >> numWires;
    file.ignore();  // Skip newline

    for (int i = 0; i < numWires; i++) {
      std::getline(file, line);
      std::istringstream iss(line);
      std::string token;
      std::vector<std::string> tokens;

      while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
      }

      if (tokens.size() >= 7) {
        VisualWire wire;
        int numPoints = std::stoi(tokens[0]);
        wire.startComponentIndex = std::stoi(tokens[1]);
        wire.startNodeIndex = std::stoi(tokens[2]);
        wire.endComponentIndex = std::stoi(tokens[3]);
        wire.endNodeIndex = std::stoi(tokens[4]);
        wire.thickness = std::stof(tokens[5]);
        wire.carriedNodeNumber = std::stoi(tokens[6]);
        wire.color = static_cast<ImU32>(std::stoul(tokens[7]));

        // Load points
        std::getline(file, line);
        std::istringstream pointStream(line);
        std::string pointStr;

        while (pointStream >> pointStr) {
          size_t commaPos = pointStr.find(',');
          if (commaPos != std::string::npos) {
            float x = std::stof(pointStr.substr(0, commaPos));
            float y = std::stof(pointStr.substr(commaPos + 1));
            wire.points.push_back(ImVec2(x, y));
          }
        }

        visualWires.push_back(wire);
      }
    }

    // ========== LOAD NODE STATE ==========
    // Skip to node state section
    while (std::getline(file, line) &&
           line.find("[NODE_STATE]") == std::string::npos) {
    }

    // Load next node number
    file >> nextNodeNumber;
    file.ignore();  // Skip newline

    // Load positionToNodeNumber map
    int numPositions;
    file >> numPositions;
    file.ignore();  // Skip newline
    for (int i = 0; i < numPositions; i++) {
      std::getline(file, line);
      size_t colonPos = line.find(':');
      if (colonPos != std::string::npos) {
        std::string posStr = line.substr(0, colonPos);
        int nodeNum = std::stoi(line.substr(colonPos + 1));

        size_t commaPos = posStr.find(',');
        if (commaPos != std::string::npos) {
          int x = std::stoi(posStr.substr(0, commaPos));
          int y = std::stoi(posStr.substr(commaPos + 1));
          positionToNodeNumber[{x, y}] = nodeNum;
        }
      }
    }

    // Load positionToWireCarryNodeNumber map
    int numWirePositions;
    file >> numWirePositions;
    file.ignore();  // Skip newline
    for (int i = 0; i < numWirePositions; i++) {
      std::getline(file, line);
      size_t colonPos = line.find(':');
      if (colonPos != std::string::npos) {
        std::string posStr = line.substr(0, colonPos);
        int nodeNum = std::stoi(line.substr(colonPos + 1));

        size_t commaPos = posStr.find(',');
        if (commaPos != std::string::npos) {
          int x = std::stoi(posStr.substr(0, commaPos));
          int y = std::stoi(posStr.substr(commaPos + 1));
          positionToWireCarryNodeNumber[{x, y}] = nodeNum;
        }
      }
    }

    // ========== LOAD COMPONENT NODE MAPPING ==========
    // Skip to component node mapping section
    while (std::getline(file, line) &&
           line.find("[COMPONENT_NODE_MAPPING]") == std::string::npos) {
    }

    int numMappings;
    file >> numMappings;
    file.ignore();  // Skip newline
    for (int i = 0; i < numMappings; i++) {
      std::getline(file, line);
      size_t colonPos = line.find(':');
      if (colonPos != std::string::npos) {
        std::string keyStr = line.substr(0, colonPos);
        int nodeNum = std::stoi(line.substr(colonPos + 1));

        size_t commaPos = keyStr.find(',');
        if (commaPos != std::string::npos) {
          int compIdx = std::stoi(keyStr.substr(0, commaPos));
          int nodeIdx = std::stoi(keyStr.substr(commaPos + 1));
          componentNodeToNumber[{compIdx, nodeIdx}] = nodeNum;
        }
      }
    }

    file.close();

    // Rebuild wire node carry map from loaded wires
    // This ensures wires' carried node numbers are in the position map
    for (const auto& wire : visualWires) {
      if (wire.carriedNodeNumber >= 0) {
        for (const auto& point : wire.points) {
          storeWireCarryNodeAtPosition(wire.carriedNodeNumber, point);
        }
      }
    }

    // Rebuild circuit with loaded components
    updateCircuitComponentNodes();

    printf("Circuit loaded from: %s\n", filename.c_str());
    printf("Components: %lu, Wires: %lu, Nodes: %d\n", visualComponents.size(),
           visualWires.size(), nextNodeNumber - 1);
  }

  /**
   * Loads component images from the resources folder.
   * Creates textures for each component type.
   * Falls back to drawing shapes if images can't be loaded.
   */
  void loadComponentImages() {
    std::vector<std::string> componentTypes = {"resistor",
                                               "capacitor",
                                               "inductor",
                                               "voltage_source",
                                               "ac_voltage_source",
                                               "dc_current_source",
                                               "ac_current_source",
                                               "ground",
                                               "wire"};

    for (const std::string& type : componentTypes) {
      std::string imagePath = "resources/components/" + type + ".png";

      sf::Texture& texture = componentTextures[type];
      if (texture.loadFromFile(imagePath)) {
        imagesLoaded = true;
        printf("Loaded image for %s\n", type.c_str());
      } else {
        // Image couldn't be loaded, will use fallback drawing
        componentTextures.erase(type);
        printf("Could not load image for %s, using shape fallback\n",
               type.c_str());
      }
    }
  } /**
     * Draws a visual component using image if available, otherwise draws
     * shapes. Handles rotation, selection highlighting, and proper
     * positioning.
     */
  void drawVisualComponentWithImage(const VisualComponent& comp,
                                    ImDrawList* drawList, int componentIndex,
                                    ImVec2 canvasPos) {
    ImU32 borderColor =
        comp.isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 0, 0, 255);
    float borderThickness = comp.isSelected ? 3.0f : 1.0f;

    // Check if we have a texture for this component type
    if (componentTextures.find(comp.type) != componentTextures.end()) {
      // Prepare sprite for SFML rendering (similar to your approach)
      sf::Sprite& sprite = componentSprites[comp.name];
      sprite.setTexture(componentTextures[comp.type]);

      // Set origin to center (like your setSerial function)
      sf::Vector2u texSize = componentTextures[comp.type].getSize();
      sprite.setOrigin(texSize.x / 2.0f, texSize.y / 2.0f);

      // Set position and rotation with canvas offset for proper alignment
      sprite.setPosition(canvasPos.x + comp.position.x,
                         canvasPos.y + comp.position.y);
      sprite.setRotation(comp.rotation);

      // Scale to fit component size if needed
      float scaleX = comp.size.x / texSize.x;
      float scaleY = comp.size.y / texSize.y;
      sprite.setScale(scaleX, scaleY);

      // Draw selection border in ImGui if selected
      if (comp.isSelected) {
        ImVec2 min = ImVec2(canvasPos.x + comp.position.x - comp.size.x / 2,
                            canvasPos.y + comp.position.y - comp.size.y / 2);
        ImVec2 max = ImVec2(canvasPos.x + comp.position.x + comp.size.x / 2,
                            canvasPos.y + comp.position.y + comp.size.y / 2);
        drawList->AddRect(min, max, borderColor, 0.0f, 0, borderThickness);
      }
    }

    // Always draw terminals, labels, and values (for image mode only)
    drawComponentExtras(comp, drawList, componentIndex, canvasPos);
  }

  /**
   * Draws common component elements: terminals, labels, and values.
   * Terminals are positioned based on component rotation.
   * Labels and values always remain horizontal regardless of component
   * rotation.
   */
  void drawComponentExtras(const VisualComponent& comp, ImDrawList* drawList,
                           int componentIndex, ImVec2 canvasPos) {
    // Calculate rotated terminal positions
    ImVec2 terminal1, terminal2;
    getRotatedTerminalPositions(comp, terminal1, terminal2);

    // Draw component connection points (terminals)
    float terminalSize = 2.0f;

    // Ground components have only one terminal
    if (comp.type == "ground") {
      ImVec2 screenTerminal1 =
          ImVec2(canvasPos.x + terminal1.x, canvasPos.y + terminal1.y);
      drawList->AddCircleFilled(screenTerminal1, terminalSize,
                                IM_COL32(0, 51, 153, 255));
      drawList->AddCircle(screenTerminal1, terminalSize, IM_COL32(0, 0, 0, 255),
                          12, 1.0f);
    } else {
      // Other components have two terminals
      ImVec2 screenTerminal1 =
          ImVec2(canvasPos.x + terminal1.x, canvasPos.y + terminal1.y);
      ImVec2 screenTerminal2 =
          ImVec2(canvasPos.x + terminal2.x, canvasPos.y + terminal2.y);
      drawList->AddCircleFilled(screenTerminal1, terminalSize,
                                IM_COL32(0, 51, 153, 255));
      drawList->AddCircleFilled(screenTerminal2, terminalSize,
                                IM_COL32(0, 51, 153, 255));
      drawList->AddCircle(screenTerminal1, terminalSize, IM_COL32(0, 0, 0, 255),
                          12, 1.0f);
      drawList->AddCircle(screenTerminal2, terminalSize, IM_COL32(0, 0, 0, 255),
                          12, 1.0f);
    }

    // Calculate moveable label position with canvas offset
    ImVec2 labelPos =
        ImVec2(canvasPos.x + comp.position.x + comp.labelOffset.x,
               canvasPos.y + comp.position.y + comp.labelOffset.y);

    // Create separate name and value texts
    std::string nameText = comp.name;
    std::string valueText = "";

    if (comp.value != 0.0f) {
      char valueBuffer[32];
      if (comp.type == "resistor") {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.0f OHM", comp.value);
      } else if (comp.type == "capacitor") {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.2e F", comp.value);
      } else if (comp.type == "inductor") {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.3f H", comp.value);
      } else if (comp.type == "voltage_source") {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.1f V", comp.value);
      } else if (comp.type == "current_source") {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.3f A", comp.value);
      } else {
        snprintf(valueBuffer, sizeof(valueBuffer), "%.2f", comp.value);
      }
      valueText = std::string(valueBuffer);
    }

    // Calculate text sizes using Arial font
    ImVec2 nameSize =
        arialFont ? arialFont->CalcTextSizeA(arialFont->FontSize, FLT_MAX, 0.0f,
                                             nameText.c_str())
                  : ImGui::CalcTextSize(nameText.c_str());
    ImVec2 valueSize =
        comp.value != 0.0f
            ? (arialFont
                   ? arialFont->CalcTextSizeA(arialFont->FontSize, FLT_MAX,
                                              0.0f, valueText.c_str())
                   : ImGui::CalcTextSize(valueText.c_str()))
            : ImVec2(0, 0);  // Calculate total label size (width = max of
                             // name/value, height = both
    // lines + spacing)
    float maxWidth = std::max(nameSize.x, valueSize.x);
    float totalHeight =
        nameSize.y + (comp.value != 0.0f ? valueSize.y + 2
                                         : 0);  // 2px spacing between lines

    // Calculate positions for name (top) and value (bottom)
    ImVec2 namePos = labelPos;
    ImVec2 valuePos = ImVec2(labelPos.x, labelPos.y + nameSize.y + 2);

    // Calculate label bounds for interaction (no visible background)
    ImVec2 labelMin = ImVec2(labelPos.x - 2, labelPos.y - 2);
    ImVec2 labelMax =
        ImVec2(labelPos.x + maxWidth + 2, labelPos.y + totalHeight + 2);

    // Draw name text (top line) with Arial font
    drawList->AddText(arialFont, 0.0f, namePos, IM_COL32(0, 0, 0, 255),
                      nameText.c_str());

    // Draw value text (bottom line) if it exists
    if (comp.value != 0.0f) {
      drawList->AddText(arialFont, 0.0f, valuePos, IM_COL32(100, 100, 100, 255),
                        valueText.c_str());
    }
  }

  /**
   * Calculates the rotated terminal positions for a component based on its
   * rotation angle. Uses the same logic as getComponentNodePositions to
   * ensure perfect synchronization between visual terminals and wire
   * connections.
   */
  void getRotatedTerminalPositions(const VisualComponent& comp,
                                   ImVec2& terminal1, ImVec2& terminal2) {
    // Use the same logic as getComponentNodePositions for perfect sync
    auto [nodeA, nodeB] = getComponentNodePositions(comp);
    terminal1 = nodeA;
    terminal2 = nodeB;
  }

  /**
   * Draw all visual wires on the canvas
   */
  void drawVisualWires(ImDrawList* drawList, ImVec2 canvasPos) {
    for (int wireIndex = 0; wireIndex < visualWires.size(); wireIndex++) {
      const auto& wire = visualWires[wireIndex];
      if (wire.points.size() < 2) continue;  // Need at least 2 points

      // Check if this wire is selected
      bool isWireSelected =
          std::find(selectedWires.begin(), selectedWires.end(), wireIndex) !=
          selectedWires.end();

      std::vector<ImVec2> screenPoints;
      // Convert all wire points from canvas to screen coordinates
      for (const auto& point : wire.points) {
        screenPoints.push_back(
            ImVec2(point.x + canvasPos.x, point.y + canvasPos.y));
      }

      // Draw all wire segments with selection highlighting
      ImU32 wireColor = isWireSelected
                            ? IM_COL32(255, 255, 0, 255)
                            : wire.color;  // Yellow for selected wires
      float wireThickness = isWireSelected
                                ? wire.thickness + 1.5f
                                : wire.thickness;  // Thicker for selected wires

      for (size_t i = 0; i < screenPoints.size() - 1; i++) {
        drawList->AddLine(screenPoints[i], screenPoints[i + 1], wireColor,
                          wireThickness);
      }

      // Draw connection points with selection highlighting
      ImU32 pointColor = isWireSelected ? IM_COL32(255, 255, 0, 255)
                                        : IM_COL32(0, 0, 255, 255);
      float pointSize = isWireSelected ? 3.0f : 2.0f;

      for (size_t pointIndex = 0; pointIndex < screenPoints.size();
           pointIndex++) {
        const ImVec2& screenPoint = screenPoints[pointIndex];
        drawList->AddCircleFilled(screenPoint, pointSize, pointColor);
      }

      // Display wire's carried node number at the middle of the wire
      if (wire.carriedNodeNumber >= 0 && screenPoints.size() >= 2) {
        // Calculate midpoint of the wire for label placement
        ImVec2 midpoint = screenPoints[screenPoints.size() / 2];

        // Format the node number label
        char nodeLabel[32];
        snprintf(nodeLabel, sizeof(nodeLabel), "Node %d",
                 wire.carriedNodeNumber);

        // Draw text label with background for readability
        ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(
            ImGui::GetFontSize(), FLT_MAX, 0.0f, nodeLabel);
        ImVec2 labelPos = ImVec2(midpoint.x - textSize.x / 2,
                                 midpoint.y - ImGui::GetFontSize() - 5);

        // Draw background rectangle
        ImVec2 bgMin = ImVec2(labelPos.x - 3, labelPos.y - 2);
        ImVec2 bgMax =
            ImVec2(labelPos.x + textSize.x + 3, labelPos.y + textSize.y + 2);
        drawList->AddRectFilled(bgMin, bgMax,
                                IM_COL32(0, 0, 0, 180));  // Dark background

        // Draw text
        ImU32 textColor = IM_COL32(255, 255, 255, 255);  // White text
        drawList->AddText(labelPos, textColor, nodeLabel);
      }
    }
  }

  // ================= OUT-OF-CLASS FUNCTION DEFINITIONS =====================

  std::pair<ImVec2, ImVec2> getComponentNodePositions(
      const App::VisualComponent& comp) {
    ImVec2 nodeA, nodeB;
    ImVec2 min = ImVec2(comp.position.x - comp.size.x / 2,
                        comp.position.y - comp.size.y / 2);
    ImVec2 max = ImVec2(comp.position.x + comp.size.x / 2,
                        comp.position.y + comp.size.y / 2);
    if (comp.type == "ground") {
      nodeA = ImVec2(comp.position.x, min.y);
      nodeB = nodeA;
    } else {
      switch ((int)comp.rotation % 360) {
        case 0:
          nodeA = ImVec2(min.x, comp.position.y);
          nodeB = ImVec2(max.x, comp.position.y);
          break;
        case 90:
          nodeA = ImVec2(comp.position.x, min.y);
          nodeB = ImVec2(comp.position.x, max.y);
          break;
        case 180:
          nodeA = ImVec2(max.x, comp.position.y);
          nodeB = ImVec2(min.x, comp.position.y);
          break;
        case 270:
          nodeA = ImVec2(comp.position.x, max.y);
          nodeB = ImVec2(comp.position.x, min.y);
          break;
        default:
          nodeA = ImVec2(min.x, comp.position.y);
          nodeB = ImVec2(max.x, comp.position.y);
          break;
      }
    }
    return std::make_pair(nodeA, nodeB);
  }

  std::pair<int, int> findClosestComponentNode(ImVec2 position,
                                               ImVec2 canvasOffset,
                                               float snapRadius) {
    int closestComponentIndex = -1;
    int closestNodeIndex = -1;
    float closestDistance = snapRadius;
    for (int i = 0; i < visualComponents.size(); i++) {
      auto [nodeA, nodeB] = getComponentNodePositions(visualComponents[i]);
      nodeA = ImVec2(canvasOffset.x + nodeA.x, canvasOffset.y + nodeA.y);
      nodeB = ImVec2(canvasOffset.x + nodeB.x, canvasOffset.y + nodeB.y);
      float distA = sqrtf((nodeA.x - position.x) * (nodeA.x - position.x) +
                          (nodeA.y - position.y) * (nodeA.y - position.y));
      if (distA < closestDistance) {
        closestDistance = distA;
        closestComponentIndex = i;
        closestNodeIndex = 0;
      }
      if (visualComponents[i].type != "ground") {
        float distB = sqrtf((nodeB.x - position.x) * (nodeB.x - position.x) +
                            (nodeB.y - position.y) * (nodeB.y - position.y));
        if (distB < closestDistance) {
          closestDistance = distB;
          closestComponentIndex = i;
          closestNodeIndex = 1;
        }
      }
    }

    return std::make_pair(closestComponentIndex, closestNodeIndex);
  }

  // Add missing closing brace and semicolon for App class
};
