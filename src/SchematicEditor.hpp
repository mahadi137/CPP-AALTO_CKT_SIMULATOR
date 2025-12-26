#pragma once
#include <vector>
#include <string>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include "Circuit.hpp"

class GUI_Component;
class GUI_Wire;
class Terminal;
class CircuitData;

class SchematicEditor {
public:
     
    explicit SchematicEditor(const sf::FloatRect& bounds);

    void draw(sf::RenderWindow& window) const;
    void handleClick(const sf::Vector2f& pos, sf::Mouse::Button button);
    void handleMouseMove(const sf::Vector2f& pos);
    void handleMouseRelease(const sf::Vector2f& pos, sf::Mouse::Button button);
    
    void addComponent(GUI_Component* comp);
    void removeComponent(GUI_Component* comp);
    void clearAll();
    
    void setGridVisible(bool visible);

    void zoom(float factor, sf::Vector2f& center);
    void pan(sf::Vector2f offset);

    std::string exportToNetlist() const;

    void loadFromData(const CircuitData& data);

    sf::Vector2f screenToWorld(const sf::Vector2f& screen) const;
    sf::Vector2f worldToScreen(const sf::Vector2f& world)const;


private:
    void drawGrid(sf::RenderWindow& window) const;
    sf::Vector2f snapToGrid(const sf::Vector2f& pos);
    GUI_Component* findComponentAt(const sf::Vector2f& pos);
    Terminal* findTerminalAt(const sf::Vector2f& pos);

    float _gridSpacing { 20.f };
    sf::FloatRect _viewBounds;

    vector<GUI_Component*> _components;
    vector<GUI_Wire*> _wires;
    GUI_Component* _selectedComponent { nullptr };

    float _zoomLevel { 1.f };
    sf::Vector2f _panOffset { 0.f , 0.f };
    bool _isPlanning { false };
    bool _isDragging { false };
    sf::Vector2f _dragStart { 0.f, 0.f };
    GUI_Wire* _tempWire { nullptr };
    sf::Font _font;

    bool _gridVisible { true };

};