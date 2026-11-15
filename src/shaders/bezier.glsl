#version 430 core

layout(local_size_x = 64) in;

// input buffers
struct ControlPoints { vec4 P[4]; };
struct Appearance { uint color; vec2 halfExt; };

layout(std430, binding = 0) readonly buffer Points { ControlPoints pts[]; };
layout(std430, binding = 1) readonly buffer Appearances { Appearance aps[]; };

// selection output (same as before)
layout(std430, binding = 2) buffer OutputBuffer {
    int IDs[16];
    float Dists[16];
};
layout(binding = 0, offset = 0) uniform atomic_uint counter;

// generated mesh output 
struct VertexOut { vec4 pos; vec4 color; };
layout(std430, binding = 3) buffer MeshOutput { VertexOut verts[]; };
layout(std430, binding = 4) buffer VertexCounter { uint vertCount; };

// uniforms 
uniform mat4 MVP;
uniform int Selection;
uniform vec3 CamPos;
uniform float PxPerRad;
uniform vec3 MouseNear;
uniform vec3 MouseFar;

// constants 
const ivec2 corners[4] = ivec2[](ivec2(-1,-1), ivec2(-1,+1), ivec2(+1,+1), ivec2(+1,-1));
const ivec2 endCorners[4] = ivec2[](ivec2(+1,+1), ivec2(+1,-1), ivec2(-1,-1), ivec2(-1,+1));
const float PI = 3.14159265;

// bezier helpers 
vec3 bezier(vec4 P[4], float u) {
    float o = 1.0 - u;
    return P[0].xyz * (o*o*o) + P[1].xyz * (3.0*u*o*o) + P[2].xyz * (3.0*u*u*o) + P[3].xyz * (u*u*u);
}
vec3 bezierDeriv(vec4 P[4], float u) {
    vec3 a = 3.0 * (P[1].xyz - P[0].xyz);
    vec3 b = 6.0 * (P[2].xyz - P[1].xyz);
    vec3 c = 3.0 * (P[3].xyz - P[2].xyz);
    return a*(1.0-u)*(1.0-u) + b*(1.0-u)*u + c*u*u;
}
float point2Mouse(vec3 pt) {
    vec3 dir = normalize(MouseFar - MouseNear);
    vec3 toN = pt - MouseNear;
    vec3 closest = MouseNear + dir * dot(toN, dir);
    return length(pt - closest);
}

// tess level logic
float computeTess(ControlPoints b, float halfExt) {
    vec3 c[4], d[4];
    for (int i = 0; i < 4; i++) c[i] = b.P[i].xyz - CamPos;

    bool veryClose = false;
    for (int i = 0; i < 4; i++) if (length(c[i]) < 1e-4) veryClose = true;

    float angularDiam;
    if (veryClose) {
        angularDiam = PI;
    } else {
        for (int i = 0; i < 4; i++) d[i] = normalize(c[i]);
        float chord = asin(clamp(length(cross(d[0], d[3])), 0.0, 1.0));
        vec3 sumDir = d[0] + d[1] + d[2] + d[3];
        vec3 avg = normalize(sumDir);
        if (length(sumDir) < 0.5) { // this means that the curve most likely wraps around the camera
            angularDiam = PI;
        } else {
            float ms = 0.0;
            for (int i = 0; i < 4; i++) ms = max(ms, length(cross(avg, d[i])));
            angularDiam = max(chord, 2.0 * asin(clamp(ms, 0.0, 1.0)));
        }
    }
    float pxSpan = angularDiam * PxPerRad;

    float maxAllowedTess = 8;
    float farthest = 0.0;
    for(int i = 0; i < 4; i++) farthest = max(farthest, length(c[i]));
    if (farthest > 1e-4){
        float widthAngularDiam = halfExt / (farthest * PI);
        maxAllowedTess = angularDiam / (widthAngularDiam * 20.0);
    }

    // calculate how curved is the curve
    vec3 chordVec = b.P[3].xyz - b.P[0].xyz;
    float chordLen = length(chordVec);
    float normDev = 0.0;
    if (chordLen > 1e-6) {
        float pd = max(length(cross(b.P[1].xyz - b.P[0].xyz, chordVec)),
            length(cross(b.P[2].xyz - b.P[0].xyz, chordVec))) / chordLen;
        normDev = clamp(pd / chordLen, 0.0, 1.0);
    }
    float tess = (pxSpan * (1.0 + 3.0 * normDev)) / 3.0;
    return clamp(tess, 2.0, maxAllowedTess);
}

// mesh emit helpers 
void writeTri(VertexOut a, VertexOut b, VertexOut c) {
    uint base = atomicAdd(vertCount, 3);
    verts[base] = a;
    verts[base + 1] = b;
    verts[base + 2] = c;
}

// i = corner index, used only for body shading (i%2==0 → darken)
VertexOut makePoint(vec3 p, vec3 N, vec3 B, vec2 h, ivec2 cor, vec4 col, bool end, int i) {
    vec3 world = p + h.x * cor.x * N + h.y * cor.y * B;
    vec4 c = col;
    if (end)       c.rgb *= 0.95;
    else if (i%2==0) c.rgb *= 0.9;
    return VertexOut(MVP * vec4(world, 1.0), c);
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= pts.length()) return;

    ControlPoints b = pts[idx];
    Appearance a = aps[idx];
    vec4 baseColor = unpackUnorm4x8(a.color);

    // T0 / N0
    vec3 T0 = normalize(3.0 * (b.P[1].xyz - b.P[0].xyz));
    vec3 ref = (abs(T0.y) > 0.9) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    vec3 N0 = normalize(cross(ref, T0));

    float tess = computeTess(b, a.halfExt.x);
    int numSegs = max(2, int(floor(tess * 0.5 + 0.5)) * 2); // nearest even, same as hw

    vec3 prevPos = bezier(b.P, 0.0);
    vec3 prevT = normalize(bezierDeriv(b.P, 0.0));

    float flipA = (1.5708 / 3.0) * 1.0; // segNum at u=0 is always 1
    vec3 prevN = normalize(N0 * cos(flipA) + cross(prevT, N0) * sin(flipA) + prevT * dot(T0, N0) * (1.0 - cos(flipA)));
    vec3 prevB = normalize(cross(prevT, prevN));

    float prevDist  = point2Mouse(prevPos);
    bool prevHover = Selection != 0 && prevDist < a.halfExt.x * 9.0;
    vec4 prevColor = prevHover ? unpackUnorm4x8(-a.color) : baseColor;

    if (prevHover) {
        uint ac = atomicCounterIncrement(counter);
        if (ac < 16) {
            IDs[ac] = int(idx);
            Dists[ac] = prevDist; 
        }
    }

    // start cap
    VertexOut scap[4];
    for (int i = 0; i < 4; i++) {
        int ci = (i + 2) % 4;
        scap[i] = makePoint(prevPos, prevN, prevB, a.halfExt, endCorners[ci], prevColor, true, 0);
    }
    writeTri(scap[2], scap[1], scap[0]);
    writeTri(scap[3], scap[2], scap[0]);

    // body segments + end cap
    for (int seg = 0; seg < numSegs; seg++) {
        float u = float(seg + 1) / float(numSegs);
        vec3 pos = bezier(b.P, u);
        vec3 T = normalize(bezierDeriv(b.P, u));

        int segNum = int(u * tess) % 3 + 1;
        float flipAng = (1.5708 / 3.0) * segNum;
        vec3 N = normalize(N0 * cos(flipAng) + cross(T, N0) * sin(flipAng) + T * dot(T0, N0) * (1.0 - cos(flipAng)));
        vec3 B = normalize(cross(T, N));

        float idist = point2Mouse(pos);
        bool hover = Selection != 0 && idist < a.halfExt.x * 9.0;
        vec4 ptCol = hover ? unpackUnorm4x8(-a.color) : baseColor;

        if (hover) {
            uint ac = atomicCounterIncrement(counter);
            if (ac < 16) {
                IDs[ac] = int(idx);
                Dists[ac] = idist;
            }
        }

        // tube wall: generate the 8-vertex strip, then decompose into 8 independent triangles
        VertexOut strip[8];
        for (int i = 0; i < 4; i++) {
            strip[i*2] = makePoint(prevPos, prevN, prevB, a.halfExt, corners[i], prevColor, false, i);
            strip[i*2+1] = makePoint(pos, N, B, a.halfExt, corners[i], ptCol, false, i);
        }
        for (int i = 0; i < 4; i++){
            writeTri(strip[2*i], strip[(2*i+2)%8], strip[(2*i+1)%8]);
            writeTri(strip[(2*i+1)%8], strip[(2*i+2)%8], strip[(2*i+3)%8]);
        }

        // end cap on last segment
        if (seg == numSegs - 1) {
            VertexOut ecap[4];
            for (int i = 0; i < 4; i++)
                ecap[i] = makePoint(pos, N, B, a.halfExt, endCorners[i], ptCol, true, 0);
            writeTri(ecap[0], ecap[1], ecap[2]);
            writeTri(ecap[2], ecap[3], ecap[0]);
        }

        prevPos = pos; prevN = N; prevB = B; prevColor = ptCol;
    }
}