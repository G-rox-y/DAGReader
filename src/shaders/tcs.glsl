#version 430 core

layout(vertices = 1) out;

struct ControlPoints{
    vec4 P[4];
};

layout(std430, binding = 0) buffer Points { ControlPoints pts[]; };

in flat uint vInstanceID[];
patch out uint patchID;
patch out vec3 T0; // tangent for first point
patch out vec3 N0; // normal for first point

uniform vec3 CamPos;
uniform float PxPerRad;

void main(){
    if (gl_InvocationID != 0) return; // safety

    patchID = vInstanceID[0];
    ControlPoints b = pts[patchID];

    T0 = normalize(3.0 * (b.P[1].xyz - b.P[0].xyz)).xyz;
    vec3 ref = (abs(T0.y) > 0.9) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    N0 = normalize(cross(ref, T0));

    // camera-relative vectors
    vec3 c[4];
    for(int i = 0; i < 4; i++) c[i] = b.P[i].xyz - CamPos;

    // angular diameter of the curve on screen
    float angularDiam = 0.0;

    // if the camera is *inside* the control hull, just throw max tess at it.
    bool veryClose = false;
    for(int i = 0; i < 4; i++) if(length(c[i]) < 1e-4) veryClose = true;

    if(veryClose){
        angularDiam = 3.14159265;
    }
    else {
        // normalized view directions
        vec3 d[4];
        for(int i = 0; i < 4; i++) d[i] = normalize(c[i]);

        // longitudinal extent: angle between the two terminals.
        float chordAngle = acos(clamp(dot(d[0], d[3]), -1.0, 1.0));

        // lateral extent: how far off the average view-axis do we bulge?
        // this catches side-view curvature when the camera looks along the curve.
        vec3 avgDir = normalize(d[0] + d[1] + d[2] + d[3]);
        if(length(avgDir) < 0.3){
            // directions cancel = curve wraps around the camera.
            angularDiam = 3.14159265;
        }
        else {
            float maxSin = 0.0;
            for(int i = 0; i < 4; i++)
                maxSin = max(maxSin, length(cross(avgDir, d[i])));
            float spreadAngle = 2.0 * asin(clamp(maxSin, 0.0, 1.0));
            angularDiam = max(chordAngle, spreadAngle);
        }
    }

    float pixelSpan = angularDiam * PxPerRad;

    // curvature boost
    // normalised deviation from the straight chord (0 = straight, ~0.25 = hairpin).
    vec3 chordVec = b.P[3].xyz - b.P[0].xyz;
    float chordLen = length(chordVec);
    float normDev = 0.0;
    if(chordLen > 1e-6){
        float perpDist = max(
            length(cross(b.P[1].xyz - b.P[0].xyz, chordVec)),
            length(cross(b.P[2].xyz - b.P[0].xyz, chordVec))
        ) / chordLen;
        normDev = clamp(perpDist / chordLen, 0.0, 1.0);
    }
    // curved geometry needs tighter tessellation for the same pixel footprint.
    float curvatureBoost = 1.0 + 3.0 * normDev;

    // final tessellation level
    float targetPxPerSeg = 3.0; // one generated segment per 3 screen pixels
    float tess = (pixelSpan * curvatureBoost) / targetPxPerSeg;

    const float MIN_TESS = 2.0;
    const float MAX_TESS = 64.0;
    tess = clamp(tess, MIN_TESS, MAX_TESS);

    gl_TessLevelOuter[0] = 1.0; // isoline: one line
    gl_TessLevelOuter[1] = tess; // ...segmented this densely
}