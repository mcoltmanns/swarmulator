#version 430
// vertex position of the base particle
// set over in main.c
layout (location=0) in vec3 vertexPosition;

// constant inputs
layout (location=0) uniform mat4 projectionMatrix;
layout (location=1) uniform mat4 viewMatrix;
layout (location=2) uniform float particleScale;

// data buffers we're reading from
// positions and directions of boids
layout(std430, binding=0) buffer ssbo0 { vec4 positions[]; };
layout(std430, binding=1) buffer ssbo1 {vec4 directions[]; };

// only output frag color
out vec4 fragColor;

void main() {
    // get position and direction info from buffers
    vec3 position = positions[gl_InstanceID].xyz;
    vec3 direction = directions[gl_InstanceID].xyz;

    // change color depending on direction
    fragColor.rgb = abs(direction) + 0.2; // direction vectors get normed in the compute shader
    fragColor.a = 1.0;

    float scale = 0.005 * particleScale;
    vec3 vertexView = vertexPosition * scale;

    // right now triangle points towards the camera
    // make it point in the direction of its movement in view space
    // first, find angle of direction in view space
    vec2 dirView = (viewMatrix * vec4(direction, 0)).xy;
    float dirAngle = atan(dirView.y, dirView.x);

    // triangle tip is at 90 degrees in view space
    // to point it at dirAngle, rotate by (90 - dirangle) degs backwards
    float rot = dirAngle - radians(90);

    // this is just weird rotation matrix multiplication
    vec2 xv = vec2(cos(rot), sin(rot));
    vec2 yv = vec2(-sin(rot), cos(rot));
    vertexView.xy = vertexView.x * xv + vertexView.y * yv;

    vertexView += (viewMatrix * vec4(position, 1)).xyz;

    gl_Position = projectionMatrix * vec4(vertexView, 1);
}
