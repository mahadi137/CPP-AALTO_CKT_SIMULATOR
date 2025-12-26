#!/usr/bin/env python3
"""
Simple script to create basic PNG component images for the circuit simulator.
Uses PIL (Python Imaging Library) to generate simple electrical component symbols.

Install PIL: pip install Pillow

Usage: python3 create_component_images.py
"""

try:
    from PIL import Image, ImageDraw
    import os
except ImportError:
    print("PIL (Pillow) not installed. Install with: pip install Pillow")
    exit(1)

def create_resistor_image(size=(0, 0)):
    """Create a resistor symbol (zigzag pattern) - Clean 6-segment design"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency

    # Connection leads
    lead_length = 60 * scale
    start_x = lead_length * 4
    end_x = big_size[0] - lead_length * 4
    body_width = end_x - start_x

    # Zigzag pattern - 6 segments for smoother appearance
    segments = 6
    seg_width = body_width / segments
    amplitude = 40 * scale  # Reduced amplitude for smoother look

    # Create zigzag points
    points = [(start_x, y_center)]
    for i in range(1, segments):
        x = (start_x + i * seg_width)
        y = y_center - amplitude if i % 2 else y_center + amplitude
        points.append((x, y))
    points.append((end_x, y_center))

    color = (0, 51, 153, 255)

    # Draw connection leads
    draw.line([(0, y_center), (start_x, y_center)], fill=color, width=line_width )
    draw.line([(end_x, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw zigzag
    for i in range(len(points) - 1):
        draw.line([points[i], points[i + 1]], fill=color, width=line_width)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_capacitor_image(size=(0, 0)):
    """Create a capacitor symbol (two parallel lines)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency

    # Connection leads
    # lead_length = 30 * scale
    plate_spacing = 80 * scale  # spacing between plates
    plate_height = big_size[1] - 4 * scale  # plate height (much bigger for better visibility)

    # Parallel plates positions
    center_x = big_size[0] // 2
    plate1_x = center_x - plate_spacing // 2
    plate2_x = center_x + plate_spacing // 2

    color = (0, 51, 153, 255)

    # Draw connection leads
    draw.line([(0, y_center), (plate1_x, y_center)], fill=color, width=line_width)
    draw.line([(plate2_x, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw parallel plates
    draw.line([(plate1_x, (big_size[1] - plate_height) // 2), 
               (plate1_x, (big_size[1] + plate_height) // 2)], fill=color, width=line_width)
    draw.line([(plate2_x, (big_size[1] - plate_height) // 2), 
               (plate2_x, (big_size[1] + plate_height) // 2)], fill=color, width=line_width)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_inductor_image(size=(0, 0)):
    """Create an inductor symbol (coil loops)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency

    # Connection leads
    lead_length = 58 * scale
    start_x = lead_length * 4
    end_x = big_size[0] - lead_length * 4
    body_width = end_x - start_x

    # Coil parameters
    coils = 3
    coil_radius = 70 * scale
    coil_spacing = (body_width // coils)

    color = (0, 51, 153, 255)

    # Draw connection leads
    draw.line([(0, y_center), (start_x, y_center)], fill=color, width=line_width)
    draw.line([(end_x, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw coils
    for i in range(coils):
        center_x = start_x + i * coil_spacing + coil_spacing // 2
        # Draw semi-circles for coil effect (upside down)
        bbox = [center_x - coil_radius, y_center - coil_radius, 
                center_x + coil_radius, y_center + coil_radius]
        draw.arc(bbox, 180, 360, fill=color, width=line_width)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_voltage_source_image(size=(0, 0)):
    """Create a voltage source symbol (circle with + and -)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency

    # Connection leads - extend to very edge of image
    # Circle parameters - reasonable size with proper leads
    center_x = big_size[0] // 2
    center_y = y_center
    radius = (big_size[1] // 3)  # Use height to determine circle size for proper aspect ratio

    # Symbol parameters - bigger plus and minus
    plus_size = 28 * scale  # Make symbols much bigger

    color = (0, 51, 153, 255)

    # Draw connection leads - from image border to circle edges
    draw.line([(0, y_center), (center_x - radius, y_center)], fill=color, width=line_width)
    draw.line([(center_x + radius, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw circle
    bbox = [center_x - radius, center_y - radius, center_x + radius, center_y + radius]
    draw.ellipse(bbox, outline=color, width=line_width)

    # Draw bigger + symbol (left - node1) 
    plus_size = plus_size * 2  # Double the plus size
    draw.line([(center_x - radius//2 - plus_size//2, center_y), 
               (center_x - radius//2 + plus_size//2, center_y)], fill=color, width=line_width - 8)
    draw.line([(center_x - radius//2, center_y - plus_size//2), 
               (center_x - radius//2, center_y + plus_size//2)], fill=color, width=line_width -8)

    # Draw bigger - symbol (right - node2)
    draw.line([(center_x + radius//2 - plus_size//2, center_y), 
               (center_x + radius//2 + plus_size//2, center_y)], fill=color, width=line_width-8)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_dc_current_source_image(size=(0, 0)):
    """Create a current source symbol (circle with arrow)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency - match voltage source

    # Connection leads - extend to very edge of image
    # Circle parameters - reasonable size with proper leads
    center_x = big_size[0] // 2
    center_y = y_center
    radius = (big_size[1] // 3)  # Use height to determine circle size for proper aspect ratio

    color = (0, 51, 153, 255)

    # Draw connection leads - from image border to circle edges
    draw.line([(0, y_center), (center_x - radius, y_center)], fill=color, width=line_width)
    draw.line([(center_x + radius, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw circle
    bbox = [center_x - radius, center_y - radius, center_x + radius, center_y + radius]
    draw.ellipse(bbox, outline=color, width=line_width)

    # Draw arrow inside circle (pointing right to indicate current direction)
    arrow_length = radius // 2
    arrow_head_size = 60 * scale  # Match voltage source symbol size
    
    # Arrow shaft (horizontal line)
    start_x = center_x - arrow_length
    end_x = center_x + arrow_length
    draw.line([(start_x, center_y), (end_x, center_y)], fill=color, width=line_width)
    
    # Arrow head (triangle pointing right)
    arrow_tip = end_x
    arrow_head_points = [
        (arrow_tip, center_y),
        (arrow_tip - arrow_head_size, center_y - arrow_head_size//2),
        (arrow_tip - arrow_head_size, center_y + arrow_head_size//2)
    ]
    draw.polygon(arrow_head_points, fill=color)

    # Add "DC" text inside the circle, below the arrow
    try:
        from PIL import ImageFont
        # Create a larger font for visibility
        font_size = int(100 * scale)
        # Try to load a TrueType font, fall back to default if not available
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", font_size)
        except:
            font = ImageFont.load_default()
    except:
        font = None
    
    text = "DC"
    text_color = color
    # Position text inside circle, centered
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]
    text_x = center_x - text_width // 2
    text_y = center_y + radius // 2 - text_height // 2  # Position below the arrow
    draw.text((text_x, text_y), text, fill=text_color, font=font)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_ac_current_source_image(size=(0, 0)):
    """Create an AC current source symbol (circle with sine wave)"""
    """Create a current source symbol (circle with arrow)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency - match voltage source

    # Connection leads - extend to very edge of image
    # Circle parameters - reasonable size with proper leads
    center_x = big_size[0] // 2
    center_y = y_center
    radius = (big_size[1] // 3)  # Use height to determine circle size for proper aspect ratio

    color = (0, 51, 153, 255)

    # Draw connection leads - from image border to circle edges
    draw.line([(0, y_center), (center_x - radius, y_center)], fill=color, width=line_width)
    draw.line([(center_x + radius, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw circle
    bbox = [center_x - radius, center_y - radius, center_x + radius, center_y + radius]
    draw.ellipse(bbox, outline=color, width=line_width)

    # Draw arrow inside circle (pointing right to indicate current direction)
    arrow_length = radius // 2
    arrow_head_size = 60 * scale  # Match voltage source symbol size
    
    # Arrow shaft (horizontal line)
    start_x = center_x - arrow_length
    end_x = center_x + arrow_length
    draw.line([(start_x, center_y), (end_x, center_y)], fill=color, width=line_width)
    
    # Arrow head (triangle pointing right)
    arrow_tip = end_x
    arrow_head_points = [
        (arrow_tip, center_y),
        (arrow_tip - arrow_head_size, center_y - arrow_head_size//2),
        (arrow_tip - arrow_head_size, center_y + arrow_head_size//2)
    ]
    draw.polygon(arrow_head_points, fill=color)

    # Add "AC" text inside the circle, centered
    try:
        from PIL import ImageFont
        # Create a larger font for visibility
        font_size = int(100 * scale)
        # Try to load a TrueType font, fall back to default if not available
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", font_size)
        except:
            font = ImageFont.load_default()
    except:
        font = None
    
    text = "AC"
    text_color = color
    # Position text inside circle, centered below the sine wave
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]
    text_x = center_x - text_width // 2
    text_y = center_y + radius // 2 - text_height // 2  # Position below the wave
    draw.text((text_x, text_y), text, fill=text_color, font=font)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_ac_voltage_source_image(size=(0, 0)):
    """Create an AC voltage source symbol (circle with sine wave)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 16 * scale  # scale line width for consistency

    # Connection leads - extend to very edge of image
    center_x = big_size[0] // 2
    center_y = y_center
    radius = (big_size[1] // 3)  # Use height to determine circle size for proper aspect ratio

    color = (0, 51, 153, 255)

    # Draw connection leads - from image border to circle edges
    draw.line([(0, y_center), (center_x - radius, y_center)], fill=color, width=line_width)
    draw.line([(center_x + radius, y_center), (big_size[0], y_center)], fill=color, width=line_width)

    # Draw circle
    bbox = [center_x - radius, center_y - radius, center_x + radius, center_y + radius]
    draw.ellipse(bbox, outline=color, width=line_width)

    # Draw sine wave inside circle - 1 complete cycle, oriented vertically
    wave_height = radius - 40 * scale
    wave_width = 50 * scale
    wave_points = []
    
    # Generate sine wave points (1 complete cycle, vertical orientation)
    import math
    for i in range(0, int(wave_height * 2)):
        y = center_y - wave_height + i
        # 1 complete cycle of sine wave (2*pi)
        x = center_x + wave_width * math.sin(2 * math.pi * i / (wave_height * 2))
        wave_points.append((x, y))
    
    # Draw sine wave
    if len(wave_points) > 1:
        for i in range(len(wave_points) - 1):
            draw.line([wave_points[i], wave_points[i + 1]], fill=color, width=line_width - 8)

    # Draw + symbol on the left (outside circle)
    plus_size = 28 * scale
    plus_x = center_x - radius - 80 * scale
    plus_y = center_y - 60 * scale  # Offset upward from horizontal center
    # Horizontal line of +
    draw.line([(plus_x - plus_size, plus_y), (plus_x + plus_size, plus_y)], fill=color, width=line_width - 8)
    # Vertical line of +
    draw.line([(plus_x, plus_y - plus_size), (plus_x, plus_y + plus_size)], fill=color, width=line_width - 8)

    # Draw - symbol on the right (outside circle)
    minus_x = center_x + radius + 80 * scale
    minus_y = center_y - 60 * scale  # Same offset as +, both in line
    # Horizontal line of -
    draw.line([(minus_x - plus_size, minus_y), (minus_x + plus_size, minus_y)], fill=color, width=line_width - 8)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_ground_image(size=(0, 0)):
    """Create a ground symbol (triangle with lines)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 8 * scale  # scale line width for consistency

    # Ground symbol parameters
    center_x = big_size[0] // 2
    line_spacing = 40 * scale
    
    # Position the top long line higher up to make room for connection
    top_line_y = y_center - line_spacing * 2
    
    color = (0, 51, 153, 255)

    # Draw vertical connection line from top edge to the top long line
    draw.line([(center_x, 0), (center_x, top_line_y)], fill=color, width=line_width)

    # Draw horizontal ground lines - wider and decreasing in length
    # Long line (base) - moved up and made wider
    draw.line([(center_x - 90 * scale, top_line_y), (center_x + 90 * scale, top_line_y)], 
              fill=color, width=line_width)
    # Medium line - made wider
    draw.line([(center_x - 60 * scale, top_line_y + line_spacing), (center_x + 60 * scale, top_line_y + line_spacing)], 
              fill=color, width=line_width)
    # Short line - made wider
    draw.line([(center_x - 30 * scale, top_line_y + 2 * line_spacing), (center_x + 30 * scale, top_line_y + 2 * line_spacing)], 
              fill=color, width=line_width)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def create_wire_image(size=(0, 0)):
    """Create a wire symbol (simple line with connection dots)"""
    scale = 4  # draw at 4× size for smoothness
    big_size = (size[0] * scale, size[1] * scale)
    
    img = Image.new('RGBA', big_size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    y_center = big_size[1] // 2
    line_width = 8 * scale  # scale line width for consistency

    # Wire parameters - square zigzag pattern
    margin = 20 * scale
    start_x = margin
    end_x = big_size[0] - margin
    
    color = (0, 51, 153, 255)
    
    # Square zigzag parameters
    zigzag_width = end_x - start_x
    segments = 6  # Number of zigzag segments
    segment_width = zigzag_width / segments
    amplitude = 30 * scale  # Height of zigzag squares
    
    # Create square zigzag points
    points = [(start_x, y_center)]
    
    for i in range(1, segments):
        x = start_x + i * segment_width
        # Create square corners - alternate up and down
        if i % 2 == 1:  # Odd segments go up then down
            points.extend([
                (x - segment_width/4, y_center),
                (x - segment_width/4, y_center - amplitude),
                (x + segment_width/4, y_center - amplitude),
                (x + segment_width/4, y_center)
            ])
        else:  # Even segments go down then up
            points.extend([
                (x - segment_width/4, y_center),
                (x - segment_width/4, y_center + amplitude),
                (x + segment_width/4, y_center + amplitude),
                (x + segment_width/4, y_center)
            ])
    
    points.append((end_x, y_center))
    
    # Draw square zigzag pattern
    for i in range(len(points) - 1):
        draw.line([points[i], points[i + 1]], fill=color, width=line_width)

    # Downsample smoothly to target size
    img = img.resize(size, Image.LANCZOS)

    return img

def main():
    # Create output directory
    output_dir = "resources/components"
    os.makedirs(output_dir, exist_ok=True)
    
    # Create component images
    components = {
        "resistor": (create_resistor_image, (800, 500)),    # Match 80x50 aspect ratio (1.6:1)
        "capacitor": (create_capacitor_image, (800, 500)),  # Match 80x50 aspect ratio (1.6:1)
        "inductor": (create_inductor_image, (800, 500)),    # Match 80x50 aspect ratio (1.6:1)
        "voltage_source": (create_voltage_source_image, (800, 500)),  # Match 100x60 aspect ratio
        "ac_voltage_source": (create_ac_voltage_source_image, (800, 500)),  # AC voltage source
        "dc_current_source": (create_dc_current_source_image, (1000, 600)),  # DC current source
        "ac_current_source": (create_ac_current_source_image, (1000, 600)),  # AC current source
        "ground": (create_ground_image, (300, 400)),  # Match 100x60 aspect ratio
        "wire": (create_wire_image, (800, 500))
    }
    
    for name, (func, size) in components.items():
        print(f"Creating {name}.png...")
        img = func(size)
        filepath = os.path.join(output_dir, f"{name}.png")
        img.save(filepath)
        print(f"  Saved: {filepath}")
    
    print(f"\nAll component images created in {output_dir}/")
    print("The circuit simulator should now load these images instead of drawing shapes.")

if __name__ == "__main__":
    main()