#version 430 core

layout(isolines, fractional_even_spacing, cw) in;

struct ControlPoints{
    vec4 P[4];
};

struct Appearance{
    uint color;
    vec2 halfExt;
};

layout(std430, binding = 0) buffer Points { ControlPoints pts[]; };
layout(std430, binding = 1) buffer Appearances { Appearance aps[]; };

patch in uint patchID;
patch in vec3 T0;
patch in vec3 N0;

out teData{
    vec4 color;
    vec3 pos;
    vec3 N; // RMF normal
    vec3 B; // RMF binormal
    vec2 halfExt;
    float u;
} teOUT;

// helper to calculate the coordinates of the point on u[0,1] along the bezier
vec3 bezier(vec4 P[4], float u){
    float w0 = pow(1.0 - u, 3.0);
    float w1 = 3.0 * u * pow(1.0 - u, 2.0);
    float w2 = 3.0 * (u*u) * (1.0 - u);
    float w3 = u*u*u;
    return  P[0].xyz * w0 + P[1].xyz * w1 + P[2].xyz * w2 + P[3].xyz * w3;
}

// helper to calculate the tangent of the point on u[0,1] along the bezier
vec3 bezierDeriv(vec4 P[4], float u) {
    vec3 a = 3.0 * (P[1].xyz - P[0].xyz);
    vec3 b = 6.0 * (P[2].xyz - P[1].xyz);
    vec3 c = 3.0 * (P[3].xyz - P[2].xyz);
    return a * (1.0-u)*(1.0-u) + b * (1.0-u)*u + c * u*u;
}

void main(){
    uint idx = patchID;
    Appearance a = aps[idx];
    ControlPoints b = pts[idx];

    // position on the curve [0,1] where 0 is the beginning and 1 is the end
    float u = gl_TessCoord.x;
    teOUT.u = u;

    // calculate the coordinates of that point
    vec3 pos = bezier(b.P, u);
    teOUT.pos = pos;

    // tangent on u
    vec3 T = normalize(bezierDeriv(b.P, u));

    // make it so that the segment rotates around the tangent in a constant manner
    // i call this a rotation maximizing frame
    int rotationSegments = 3;
    int segNum = int(u * gl_TessLevelOuter[1]) % rotationSegments + 1;
    float flipAngle = (1.5708 / rotationSegments) * segNum;

    // calculate the normal and the binormal
    vec3 N = N0 * cos(flipAngle) + cross(T, N0) * sin(flipAngle) + T * dot(T, N0) * (1.0 - cos(flipAngle));
    teOUT.N = N;
    teOUT.B = normalize(cross(T, N));

    // copy over the extension dimensions
    teOUT.halfExt = a.halfExt;

    // unpack the color here because its less expensive since tes is parallelized
    teOUT.color = unpackUnorm4x8(a.color);

    gl_Position = vec4(pos, 1.0);
};