#version 430

layout(vertices = 1) out;

struct BezierBox{
    vec4 P[4];
    uint color;
    vec2 halfExt;
};

layout(std430, binding = 0) buffer Boxes { BezierBox box[]; };

in flat uint vInstanceID[];
patch out flat uint patchID;

void main(){
    if (gl_InvocationID == 0) {
        patchID = vInstanceID[0];
        float L = length(box[patchID].P[3].xyz - box[patchID].P[0].xyz);
        float tess = clamp(L * 8.0, 8.0, 32.0);
        gl_TessLevelOuter[0] = 2.0;
        gl_TessLevelOuter[1] = tess;
    }
}