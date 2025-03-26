#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Uniform inputs
uniform vec2 resolution;     // Viewport resolution (in pixels)
uniform float scale;         // Grid scale factor
uniform vec2 offset;         // Grid offset (camera position)
uniform vec4 gridColor;      // Grid line color
uniform vec4 backgroundColor; // Background color
// Add uniform colors for axes
uniform vec4 xAxisColor = vec4(1.0, 0.0, 0.0, 0.8);  // Red for x-axis with 80% opacity
uniform vec4 yAxisColor = vec4(0.0, 1.0, 0.0, 0.8);  // Green for y-axis with 80% opacity

// Debug uniforms
uniform int debugMode;       // 0=off, 1=debug visualization
uniform float debugParam;    // Adjustable parameter for debugging

// Constants for the grid system
const float minCellSize = 0.1;     // Minimum cell size in world units
const float minCellPixelWidth = 2.0; // Minimum cell width in pixels
const float lineWidth = 3.0;        // Width of grid lines in pixels
const vec3 thinColor = vec3(0.5, 0.5, 0.5);  // Color for thin lines
const vec3 thickColor = vec3(0.8, 0.8, 0.8);  // Color for thick lines

// GLSL-style modulo function for proper grid wrapping
float mod2(float x, float y) {
    return x - y * floor(x / y);
}

vec2 mod2(vec2 x, float y) {
    return vec2(mod2(x.x, y), mod2(x.y, y));
}

// Utility functions for grid rendering
float max2(vec2 v) {
    return max(v.x, v.y);
}

float log10(float x) {
    return log(x) / log(10.0);
}

// Main grid rendering function using LOD
vec4 renderGrid(vec2 worldPos) {
    // Calculate screen-space derivatives to determine grid density
    vec2 dudv = vec2(
        length(vec2(dFdx(worldPos.x), dFdy(worldPos.x))),
        length(vec2(dFdx(worldPos.y), dFdy(worldPos.y)))
    );
    
    // Calculate level of detail based on screen-space size
    float lod = max(0.0, log10((max2(dudv) * minCellPixelWidth) / minCellSize) + 1.0);
    float fade = fract(lod);
    
    // Calculate grid sizes for different LOD levels
    float lod0 = minCellSize * pow(10.0, floor(lod));    // Smallest visible grid
    float lod1 = lod0 * 10.0;                           // Medium grid
    float lod2 = lod1 * 10.0;                           // Largest grid
    
    // Calculate grid line intensities with anti-aliasing
    float lod0a = max2(vec2(1.0) - abs(clamp(mod2(worldPos, lod0) / dudv / lineWidth, 0.0, 1.0) * 2.0 - vec2(1.0)));
    float lod1a = max2(vec2(1.0) - abs(clamp(mod2(worldPos, lod1) / dudv / lineWidth, 0.0, 1.0) * 2.0 - vec2(1.0)));
    float lod2a = max2(vec2(1.0) - abs(clamp(mod2(worldPos, lod2) / dudv / lineWidth, 0.0, 1.0) * 2.0 - vec2(1.0)));
    
    // Special handling for axes (x=0, y=0)
    // Use more precise threshold-based detection for axes
    float axisThreshold = dudv.x * lineWidth * 0.8;
    
    // Calculate distance to axes in world space
    float distToXAxis = abs(worldPos.y);
    float distToYAxis = abs(worldPos.x);
    
    // Create smooth transition for axes with anti-aliasing
    float xAxis = smoothstep(axisThreshold, 0.0, distToXAxis);
    float yAxis = smoothstep(axisThreshold, 0.0, distToYAxis);
    
    // Determine color and opacity based on grid level
    vec3 color;
    float alpha;
    
    if (lod2a > 0.0) {
        // Major grid lines
        color = thickColor;
        alpha = lod2a * 0.8;
    } else if (lod1a > 0.0) {
        // Medium grid lines with fade between levels
        color = mix(thickColor, thinColor, fade);
        alpha = lod1a * 0.6;
    } else {
        // Minor grid lines
        color = thinColor;
        alpha = lod0a * (1.0 - fade) * 0.4;
    }
    
    // Apply axis highlighting with specific colors and transparency
    // Make axes stand out more by increasing alpha
    if (xAxis > 0.0) {
        color = xAxisColor.rgb; // X-axis color
        alpha = max(alpha, xAxis * 0.9); // Increased from 0.7 to 0.9
    }
    
    if (yAxis > 0.0) {
        color = yAxisColor.rgb; // Y-axis color
        alpha = max(alpha, yAxis * 0.9); // Increased from 0.7 to 0.9
    }
    
    return vec4(color, alpha);
}


vec2 fragmentToWorldPos(vec2 fragCoord) {
    // Convert from fragment coordinates to centered screen coordinates
    vec2 screenPos = fragCoord - resolution * 0.5;
    
    // Apply camera transformations:
    // 1. Convert to world space by dividing by zoom (scale)
    // 2. Add the camera target position (offset)
    // invert the y axis of the offset
    vec2 newOffset = vec2(offset.x, -offset.y);
    vec2 worldPos = screenPos / scale + newOffset;

    return worldPos;
}

void main()
{
    // Then apply the exact same formula as in Canvas::ScreenToWorld
    vec2 worldPos = fragmentToWorldPos(gl_FragCoord.xy);
    
    // Get the grid color from our LOD-based grid function
    vec4 gridResult = renderGrid(worldPos);
    
    // Mix the grid with background color based on alpha
    vec4 color = mix(backgroundColor, vec4(gridResult.rgb, 1.0), gridResult.a);
    
    // Circular distance fade to avoid distant aliasing
    float distanceFromCenter = length(fragTexCoord - 0.5) * 2.0;
    float fadeFactor = smoothstep(0.7, 1.0, distanceFromCenter);
    color = mix(color, backgroundColor, fadeFactor);
    
    // Debug visualization if enabled
    if (debugMode == 1) {
        int mode = int(mod(debugParam, 5.0));
        
        if (mode == 0) {
            // Visualize world position coordinates
            finalColor = vec4(mod2(worldPos / 100.0, 1.0), 0.5, 1.0);
        }
        else if (mode == 1) {
            // Visualize LOD value
            float lod = max(0.0, log10((max2(vec2(
                length(vec2(dFdx(worldPos.x), dFdy(worldPos.x))),
                length(vec2(dFdx(worldPos.y), dFdy(worldPos.y)))
            )) * minCellPixelWidth) / minCellSize) + 1.0);
            finalColor = vec4(fract(lod), floor(lod)/10.0, 0.0, 1.0);
        }
        else if (mode == 2) {
            // Visualize derivatives
            vec2 dudv = vec2(
                length(vec2(dFdx(worldPos.x), dFdy(worldPos.x))),
                length(vec2(dFdx(worldPos.y), dFdy(worldPos.y)))
            );
            finalColor = vec4(dudv * 50.0, 0.0, 1.0);
        }
        else if (mode == 3) {
            // Visualize grid cells
            float lod = max(0.0, log10((max2(vec2(
                length(vec2(dFdx(worldPos.x), dFdy(worldPos.x))),
                length(vec2(dFdx(worldPos.y), dFdy(worldPos.y)))
            )) * minCellPixelWidth) / minCellSize) + 1.0);
            float lod1 = minCellSize * pow(10.0, floor(lod)) * 10.0;
            vec2 cellCoord = mod2(worldPos, lod1) / lod1;
            finalColor = vec4(cellCoord, 0.0, 1.0);
        }
        else if (mode == 4) {
            // Show grid alpha channel
            vec4 grid = renderGrid(worldPos);
            finalColor = vec4(grid.a, grid.a, grid.a, 1.0);
        }
    } else {
        finalColor = color;
    }
}
