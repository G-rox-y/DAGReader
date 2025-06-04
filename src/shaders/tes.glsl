#version 430

layout(isolines, fractional_even_spacing, cw) in;

struct BezierBox{
    vec4 P[4];
    uint color;
    vec2 halfExt;
};

layout(std430, binding = 0) buffer Boxes { BezierBox box[]; };

in flat uint tcInstanceID[];

out teData{
    vec4 color;
    vec3 pos;
    vec3 N; // Frenet normal
    vec3 B; // Frenet binormal
    vec2 halfExt;
    float u;
} teOUT;

vec3 bezier(vec4 P[4], float u){
    float w0 = pow(1.0 - u, 3.0);
    float w1 = 3.0 * u * pow(1.0 - u, 2.0);
    float w2 = 3.0 * (u*u) * (1.0 - u);
    float w3 = u*u*u;
    return  P[0].xyz * w0 + P[1].xyz * w1 + P[2].xyz * w2 + P[3].xyz * w3;
}

vec3 bezierDeriv(vec4 P[4], float u) {
    vec3 a = 3.0 * (P[1].xyz - P[0].xyz);
    vec3 b = 6.0 * (P[2].xyz - P[1].xyz);
    vec3 c = 3.0 * (P[3].xyz - P[2].xyz);
    return a * (1.0-u)*(1.0-u) + b * (1.0-u)*u + c * u*u;
}

vec3 bezierSecondDeriv(vec4 P[4], float u){
    vec3 a = 6.0 * (P[2].xyz - 2.0*P[1].xyz + P[0].xyz);
    vec3 b = 6.0 * (P[3].xyz - 2.0*P[2].xyz + P[1].xyz);
    return mix(a, b, u);
}

void main(){
    uint idx = tcInstanceID[0];
    BezierBox b = box[idx];

    float u = gl_TessCoord.x;

    // TODO: replace frenet frame with rotation minimization frame
    vec3 P = bezier(b.P, u);
    vec3 T = normalize(bezierDeriv(b.P, u));
    vec3 dd = bezierSecondDeriv(b.P, u);
    vec3 Np = dd - dot(dd, T) * T;

    vec3 N = (length(Np) < 1e-4) ? normalize(cross(T, vec3(0.0,1.0,0.0))) : normalize(Np);

    vec3 B = normalize(cross(T, N));

    teOUT.color = unpackUnorm4x8(b.color);
    teOUT.pos = P;
    teOUT.N = N;
    teOUT.B = B;
    teOUT.halfExt = b.halfExt;
    teOUT.u = u;

    gl_Position = vec4(P, 1.0);
};