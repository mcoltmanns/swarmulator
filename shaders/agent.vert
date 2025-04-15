#version 430
struct Agent {
    vec4 position;
    vec4 direction;
    vec2 signals;
    vec2 info;
};

// vertex position of the agent
layout (location=0) in vec3 vertex_position;

// constant inputs
layout (location=0) uniform mat4 projection_matrix;
layout (location=1) uniform mat4 view_matrix;
layout (location=2) uniform float agent_scale;
layout (location=3) uniform int num_agents;

// data buffers we're reading from
// buffer of agents
layout(std430, binding=0) buffer ssbo0 { Agent agents[]; };

// only output frag color
out vec4 fragColor;

void main() {
    // get position and direction info from buffers
    vec3 position = agents[gl_InstanceID].position.xyz;
    vec3 direction = agents[gl_InstanceID].direction.xyz;

    // change color depending on direction
    fragColor.rgb = abs(direction) + 0.2; // direction vectors get normed in the compute shader
    fragColor.a = gl_InstanceID >= num_agents ? 0.0 : 1.0; // only show this agent if they're alive

    float scale = 0.005 * agent_scale;
    vec3 vertexView = vertex_position * scale;

    // right now triangle points towards the camera
    // make it point in the direction of its movement in view space
    // first, find angle of direction in view space
    vec2 dirView = (view_matrix * vec4(direction, 0)).xy;
    float dirAngle = atan(dirView.y, dirView.x);

    // triangle tip is at 90 degrees in view space
    // to point it at dirAngle, rotate by (90 - dirangle) degs backwards
    float rot = dirAngle - radians(90);

    // this is just weird rotation matrix multiplication
    vec2 xv = vec2(cos(rot), sin(rot));
    vec2 yv = vec2(-sin(rot), cos(rot));
    vertexView.xy = vertexView.x * xv + vertexView.y * yv;

    vertexView += (view_matrix * vec4(position, 1)).xyz;

    gl_Position = projection_matrix * vec4(vertexView, 1);
}
