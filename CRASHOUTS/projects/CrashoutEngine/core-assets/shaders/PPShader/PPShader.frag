#version 460 core

out vec4 fragColor;

in vec2 texCoord;                   // From vertex shader

uniform sampler2D u_sceneTexture;   // Exported scene as a texture

// Color grading controls
uniform float u_brightness = 0.0;   // 0.0 = no change
uniform float u_contrast = 0.0;     // 0.0 = no change
uniform float u_saturation = 0.0;   // 0.0 = no change
uniform bool  u_active = false;   

void main() {
    // Get the pixel color from game scene
    vec3 color = texture(u_sceneTexture, texCoord).rgb;

    if(u_active){
        // Apply Brightness
        color += u_brightness;

        // Apply Contrast
        // Moves color away from (or towards) middle-gray (0.5)
        color = (color - 0.5) * u_contrast + 0.5;

        // Apply Saturation
        // Converts to grayscale then blends back to original based on saturation uniform
        vec3 grayscale = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
        color = mix(grayscale, color, u_saturation);

        // Final output, Clamp to make sure colors are valid 0.0 - 1.0
        color = clamp(color, 0.0, 1.0);
    }
    
    // Set the final pixel color
    fragColor = vec4(color, 1.0);

}