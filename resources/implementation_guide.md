# Component Image Implementation Guide

## Current Implementation Status

The code has been modified to support loading component images, but there's a technical issue that needs to be resolved:

**Problem**: SFML textures cannot be directly used with ImGui DrawList. They use different graphics contexts.

## Solution Options

### Option 1: Use ImGui-SFML Texture Integration

```cpp
// Convert SFML texture to ImGui texture ID
ImTextureID convertSFMLTextureToImGui(const sf::Texture& texture) {
    return reinterpret_cast<ImTextureID>(texture.getNativeHandle());
}

// Use in drawing:
drawList->AddImage(textureID, min, max);
```

### Option 2: Use stb_image to Load Images Directly for ImGui

```cpp
// Load image data
int width, height, channels;
unsigned char* imageData = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

// Create OpenGL texture
GLuint textureID;
glGenTextures(1, &textureID);
glBindTexture(GL_TEXTURE_2D, textureID);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
```

## Required Image Files

Create these PNG files in `resources/components/`:

- `resistor.png` - Resistor symbol (zigzag line)
- `capacitor.png` - Capacitor symbol (two parallel lines)
- `inductor.png` - Inductor symbol (coil loops)
- `voltage_source.png` - Voltage source (circle with +/- symbols)
- `ground.png` - Ground symbol (triangle with lines)

## Image Specifications

- Format: PNG with transparency
- Size: 64x32 pixels (128x64 for high DPI)
- Background: Transparent
- Line color: Black (#000000)
- Line width: 2-3 pixels

## Current Fallback Behavior

The implementation will:

1. Try to load PNG images
2. If images exist, display "IMG" placeholder (needs proper texture integration)
3. If images don't exist, fall back to original shape drawing

## Next Steps

1. Create the actual PNG component images
2. Implement proper SFML-to-ImGui texture conversion
3. Test with actual image files
