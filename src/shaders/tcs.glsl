#version 430

layout(vertices = 1) out;

struct BezierBox{
    vec4 P[4];
    uint color;
    vec2 halfExt;
};

layout(std430, binding = 0) buffer Boxes { BezierBox box[]; };

in flat uint vInstanceID[];
out flat uint tcInstanceID[];

void main(){
    tcInstanceID[gl_InvocationID] = vInstanceID[0];

    if (gl_InvocationID == 0) {
        uint idx = vInstanceID[0];
        float L = length(box[idx].P[3].xyz - box[idx].P[0].xyz);
        float tess = clamp(L * 8.0, 8.0, 32.0);
        gl_TessLevelOuter[0] = 2.0;
        gl_TessLevelOuter[1] = tess;
    }
}