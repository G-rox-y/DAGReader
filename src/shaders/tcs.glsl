#version 430 core

layout(vertices = 1) out;

struct BezierBox{
    vec4 P[4];
    uint color;
    vec2 halfExt;
};

layout(std430, binding = 0) buffer Boxes { BezierBox box[]; };

in flat uint vInstanceID[];
patch out uint patchID;
patch out vec3 T0; // tangent for first point
patch out vec3 N0; // normal for first point

void main(){
    if (gl_InvocationID != 0) return; // safety

    patchID = vInstanceID[0];
    BezierBox b = box[patchID];

    // calculate the initial frame
    T0 = normalize(3.0 * (b.P[1] - b.P[0])).xyz;
    vec3 ref = (abs(T0.y) > 0.9) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    N0 = normalize(cross(ref, T0));

    // calculate number of segments
    float L = length(b.P[3].xyz - b.P[0].xyz);
    float tess = clamp(L * 8.0, 16.0, 64.0);

    gl_TessLevelOuter[0] = 2.0;
    gl_TessLevelOuter[1] = tess;
}