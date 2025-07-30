#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float viewportHeight;
    float fovy;
    float particleWorldRadius;
} ubo;

void main() {
    // Calculate the final position in clip space
    // Vulkan's clip space has a Y-coordinate that is inverted compared to OpenGL.
    // The GLM_FORCE_DEPTH_ZERO_TO_ONE and GLM_FORCE_RADIANS defines in your
    // C++ code handle this automatically when you create your projection matrix.
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // Pass the input color directly to the fragment shader
    fragColor = inColor;

    float w_safe = max(gl_Position.w, 0.001);
    float pointDiameter = ubo.particleWorldRadius * 2.0;
    // gl_PointSize = 50.0;
    gl_PointSize = pointDiameter * ubo.viewportHeight / (2 * tan(ubo.fovy * 0.5) * w_safe); 
}