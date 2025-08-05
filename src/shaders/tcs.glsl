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

uniform vec3 CamPos;
uniform float PxPerRad;

void main(){
    if (gl_InvocationID != 0) return; // safety

    patchID = vInstanceID[0];
    BezierBox b = box[patchID];

    // calculate the initial frame
    T0 = normalize(3.0 * (b.P[1] - b.P[0])).xyz;
    vec3 ref = (abs(T0.y) > 0.9) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    N0 = normalize(cross(ref, T0));

    // calculate the angular size and the closeness of the curve
    vec3 C[4];
    for(int i = 0; i < 4; i++)
        C[i] = b.P[i].xyz + CamPos;
    // ^ the camera coordinates seem to be inverted, thats why its a + instead of a -
    vec3 R[4];
    int n = 0;
    float closeness = 1e5;
    for (int i = 0; i < 4; ++i) {
        float L = length(C[i]);
        closeness = min(closeness, L);
        if (L > 1e-6) R[n++] = C[i] / L;
    }
    float maxExt = max(b.halfExt.x, b.halfExt.y);
    closeness /= maxExt;
    float maxAng = 0.0;
    if (n > 1)
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j) 
                maxAng = max(maxAng, acos(clamp(dot(R[i], R[j]), -1.0, 1.0)));

    // calculate the flatness of the curve
    vec3 ln = b.P[3].xyz - b.P[0].xyz;
    float lnL = length(ln);
    float flatness;
    if (lnL < 1e-6){
        flatness = max(
            length(b.P[1].xyz - b.P[0].xyz),
            length(b.P[2].xyz - b.P[0].xyz)
        );
    } else {
        flatness = max(
            length(cross(b.P[1].xyz - b.P[0].xyz, ln))/lnL,
            length(cross(b.P[2].xyz - b.P[0].xyz, ln))/lnL
        );
    }

    // calculate how many segments to use
    float minPx = 40.0, maxPx = 130.0;
    float pxSize = PxPerRad * maxAng;

    float minSeg = 1.0;
    float maxSeg = clamp(flatness / maxExt * 4.0, 8.0, 32.0);

    gl_TessLevelOuter[0] = 1.0;

    if (closeness < 40.0){
        gl_TessLevelOuter[1] = maxSeg;
    }
    else{
        float t = clamp((pxSize - minPx) / (maxPx - minPx), 0.0, 1.0);
        gl_TessLevelOuter[1] = clamp(mix(minSeg, maxSeg, t), minSeg, maxSeg);
    }

    
    
}