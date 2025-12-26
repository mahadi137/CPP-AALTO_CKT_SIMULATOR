#include "SchematicEditor.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include<iostream>
SchematicEditor::SchematicEditor(const sf::FloatRect& bounds) : _viewBounds(bounds){
   //_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
}

void SchematicEditor::draw(sf::RenderWindow& window) const{
    if (_gridVisible)
        drawGrid(window);
    /*
    for (auto* c : _components)
        c->draw(window, _zoomLevel, _panOffset);
    for (auto* w : _wires)
        w->draw(window, _zoomLevel, _panOffset);
    if (_tempWire)
        _tempWire->draw(window, _zoomLevel, _panOffset);
        */
}

void SchematicEditor::handleClick(const sf::Vector2f& pos, sf::Mouse::Button button){
    sf::Vector2f world = screenToWorld(pos);

    if (button == sf::Mouse::Left){
        /*Terminal* t = findTerminalAt(world);
        if (t){
            _isPlanning = true;
            _tempWire = new GUI_Wire(t);
            return;
        }
        _selectedComponent = findComponentAt(world);
        if (_selectedComponent){
            _isDragging = true;
            _dragStart = world;
        }*/
        std::cout<<"Clicked at "<< world.x << "," << world.y << "\n";
    }
}

void SchematicEditor::handleMouseMove(const sf::Vector2f& pos){
    sf::Vector2f world = screenToWorld(pos);
    /*
    if (_isDragging && _selectedComponent){
        sf::Vector2f delta = world - _dragStart;
        _selectedComponent->move(delta);
        _dragStart = world;
    }
    if (_isPlanning && _tempWire)
        _tempWire->setEndpoint(world);
    */
}

void SchematicEditor::handleMouseRelease(const sf::Vector2f& pos, sf::Mouse::Button button){
    sf::Vector2f world = screenToWorld(pos);
    if (button == sf::Mouse::Left){
        /*if (_isPlanning && _tempWire){
            Terminal* t = findTerminalAt(world);
            if (t && t != _tempWire->start()){
                _tempWire->setEndTerminal(t);
                _wires.push_back(_tempWire);
                _tempWire = nullptr;
            }
            else{
                delete _tempWire;
                _tempWire = nullptr;
            }

            _isPlanning = false;
        }
        _isDragging = false;*/
    }
}

/*void SchematicEditor::addComponent(GUI_Component* comp){
   _components.push_back(comp);
}*/

/*void SchematicEditor::removeComponent(GUI_Component* comp){
    auto it = std::find(_components.begin(), _components.end(), comp);
    if (it != _components.end())
        _components.erase(it);
}*/

void SchematicEditor::clearAll(){
    /*_components.clear();
    _wires.clear();
    _selectedComponent = nullptr;
    _tempWire = nullptr;*/
}

void SchematicEditor::setGridVisible(bool visible){
    _gridVisible = visible;
}

void SchematicEditor::zoom(float factor, sf::Vector2f& center){
    _zoomLevel *= factor;
}

void SchematicEditor::pan(sf::Vector2f offset){
    _panOffset += offset;
}

std::string SchematicEditor::exportToNetlist() const{
    return "";
}

void SchematicEditor::loadFromData(const CircuitData& data){}

sf::Vector2f SchematicEditor::screenToWorld(const sf::Vector2f& s) const{
    return (s - sf::Vector2f(_viewBounds.left, _viewBounds.top)) / _zoomLevel - _panOffset;
}

sf::Vector2f SchematicEditor::worldToScreen(const sf::Vector2f& w) const{
    return (w + _panOffset) * _zoomLevel + sf::Vector2f(_viewBounds.left, _viewBounds.top);
}

void SchematicEditor::drawGrid(sf::RenderWindow& window) const{
    float spacing = _gridSpacing * _zoomLevel;
    sf::Vector2u size = window.getSize();
    sf::RectangleShape line;
    line.setFillColor(sf::Color(230, 230, 230));
    for (float x = fmod(_panOffset.x * _zoomLevel, spacing); x < size.x; x += spacing){
        line.setSize({1.f, (float)size.y});
        line.setPosition(x, 0);
        window.draw(line);
    }
    for (float y = fmod(_panOffset.y * _zoomLevel, spacing); y < size.y; y += spacing){
        line.setSize({(float)size.x, 1.f});
        line.setPosition(0, y);
        window.draw(line);
    }
}

sf::Vector2f SchematicEditor::snapToGrid(const sf::Vector2f& pos){
    float gx = round(pos.x / _gridSpacing) * _gridSpacing;
    float gy = round(pos.y / _gridSpacing) * _gridSpacing;
    return sf::Vector2f(gx, gy);
}

/*GUI_Component* SchematicEditor::findComponentAt(const sf::Vector2f& pos){
    for (auto* c : _components){
        if (c->contains(pos))
            return c;
    }
    return nullptr;
}*/

Terminal* SchematicEditor::findTerminalAt(const sf::Vector2f& pos){
    /*for (auto* c : _components){
        Terminal* t = c->findTerminal(pos);
        if (t) return t;
    }*/
    return nullptr;
}
