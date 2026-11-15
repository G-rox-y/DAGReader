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
layout(std430, binding = 4) buffer MeshIndices { uint indices[]; };
layout(std430, binding = 5) buffer DrawCounts { uint vertCount; uint idxCount; };

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

// i = corner index, used only for body shading (i%2==0 → darken)
VertexOut makePoint(vec3 p, vec3 N, vec3 B, vec2 h, ivec2 cor, vec4 col, bool end, int i) {
    vec3 world = p + h.x * cor.x * N + h.y * cor.y * B;
    vec4 c = col;
    if (i%2==0) c.rgb *= 0.9;
    return VertexOut(MVP * vec4(world, 1.0), c);
}

void emitVert(inout uint vOff, inout uint lv, VertexOut v) { verts[vOff + lv++] = v; }
void emitIdx(inout uint iOff, inout uint li, uint a, uint b, uint c) {
    indices[iOff + li++] = a;
    indices[iOff + li++] = b;
    indices[iOff + li++] = c;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= pts.length()) return;

    ControlPoints b = pts[idx];
    Appearance a = aps[idx];
    vec4 baseColor = unpackUnorm4x8(a.color);

    // init frame data
    vec3 T0 = normalize(3.0 * (b.P[1].xyz - b.P[0].xyz));
    vec3 ref = (abs(T0.y) > 0.9) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    vec3 N0 = normalize(cross(ref, T0));

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

    // tesselation data
    float tess = computeTess(b, a.halfExt.x);
    int numSegs = max(2, int(floor(tess * 0.5 + 0.5)) * 2); // nearest even, same as hw

    // data for index buffer
    uint nRings = uint(numSegs) + 1; //shared seams
    uint myVerts = 4*nRings; // start cap + body + end cap
    uint myIdx = 6 + 24*uint(numSegs) + 6; // caps + body tris + caps

    uint vOff = atomicAdd(vertCount, myVerts);
    uint iOff = atomicAdd(idxCount, myIdx);

    uint lv = 0; // local vertex offset inside this curve
    uint li = 0; // local index offset inside this curve

    // first ring
    for (int c = 0; c < 4; c++)
        emitVert(vOff, lv, makePoint(prevPos, prevN, prevB, a.halfExt, corners[c], prevColor, false, c));

    emitIdx(iOff, li, vOff+2, vOff+1, vOff+0);
    emitIdx(iOff, li, vOff+3, vOff+2, vOff+0);

    uint prevRing = 0; // local vertex index where the previous ring starts

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

        uint nextRing = lv;
        for (int c = 0; c < 4; c++)
            emitVert(vOff, lv, makePoint(pos, N, B, a.halfExt, corners[c], ptCol, false, c));

        // 4 faces, each 2 triangles = 8 triangles per segment
        for (int side = 0; side < 4; side++) {
            int c0 = side;
            int c1 = (side + 1) % 4;

            uint p0 = vOff + prevRing + c0;
            uint p1 = vOff + prevRing + c1;
            uint n0 = vOff + nextRing + c0;
            uint n1 = vOff + nextRing + c1;

            emitIdx(iOff, li, n0, p0, p1);
            emitIdx(iOff, li, n0, p1, n1);
        }

        prevRing = nextRing;
        prevPos = pos; prevN = N; prevB = B; prevColor = ptCol;
    }

    emitIdx(iOff, li, vOff+prevRing+0, vOff+prevRing+1, vOff+prevRing+2);
    emitIdx(iOff, li, vOff+prevRing+2, vOff+prevRing+3, vOff+prevRing+0);
}