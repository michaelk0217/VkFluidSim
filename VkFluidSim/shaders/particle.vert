// particle.vert

#version 450

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;
    float density;
    float _padding[3];
};

layout(binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
} particleBuffer;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float viewportHeight;
    float fovy;
    float particleWorldRadius;
    float _padding;
} ubo;

void main() {
    // Calculate the final position in clip space
    // Vulkan's clip space has a Y-coordinate that is inverted compared to OpenGL.
    // The GLM_FORCE_DEPTH_ZERO_TO_ONE and GLM_FORCE_RADIANS defines in your
    // C++ code handle this automatically when you create your projection matrix.

    Particle p = particleBuffer.particles[gl_VertexIndex];

    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(p.position.xyz, 1.0);

    // Pass the input color directly to the fragment shader
    fragColor = p.color.xyz;

    float w_safe = max(gl_Position.w, 0.001);
    float pointDiameter = ubo.particleWorldRadius * 2.0;
    // gl_PointSize = 50.0;
    gl_PointSize = pointDiameter * ubo.viewportHeight / (2 * tan(ubo.fovy * 0.5) * w_safe); 
}