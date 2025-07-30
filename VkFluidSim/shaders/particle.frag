#version 450

layout(location = 0) in vec3 fragColor;


layout(location = 0) out vec4 outColor;

void main() {

    float dist = distance(gl_PointCoord, vec2(0.5));
    if (dist > 0.5){
        discard;
    }

    outColor = vec4(fragColor, 1.0);
    // outColor = vec4(0.2, 0.6, 1.0, 1.0);

    
}