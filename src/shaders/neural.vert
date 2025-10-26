#version 430
struct sim_object {
    vec4 position;
    vec4 direction;
    vec4 scale;
    vec4 info;
};

// mesh info
layout (location=0) in vec3 vertex_position;

// matrices
uniform mat4 proj_mat;
uniform mat4 view_mat;
uniform int instance_count; // number of instances in the group

// ssbo for all the objects
layout(std430, binding=0) buffer ssbo0 { sim_object objects[]; };

// outputs
out vec4 frag_color;

void main() {
    sim_object object = objects[gl_InstanceID];

    frag_color.rgb = vec3(object.info.x, object.info.y, 0) + 0.1;
    frag_color.a = gl_InstanceID < instance_count ? 1.0 : 0.0; // only show this thing if the stuff in the buffer isn't junk

    vec3 vertex_view = vertex_position * object.scale.xyz;

    vec2 dir_view = (view_mat * vec4(object.direction.xyz, 0)).xy;
    float dir_angle = atan(dir_view.y, dir_view.x);

    float rot = dir_angle - radians(90);

    vec2 xv = vec2(cos(rot), sin(rot));
    vec2 yv = vec2(-sin(rot), cos(rot));
    vertex_view.xy = vertex_view.x * xv + vertex_view.y * yv;

    vertex_view += (view_mat * vec4(object.position.xyz, 1)).xyz;

    gl_Position = proj_mat * vec4(vertex_view, 1);
}
