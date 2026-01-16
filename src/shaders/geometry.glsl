#version 430 core

layout(lines) in;
layout(triangle_strip, max_vertices = 20) out;

in teData{
    vec4 color;
    vec3 pos;
    vec3 N;
    vec3 B;
    vec2 halfExt;
    float u;
} gsIN[];

flat out vec4 gsColor;

uniform mat4 MVP;
uniform float Scale;
uniform int Selection;

const ivec2 corners[4] = ivec2[4](ivec2(-1,-1), ivec2(+1,-1), ivec2(+1,+1), ivec2(-1,+1));
const ivec2 endCorners[4] = ivec2[4](ivec2(+1,+1), ivec2(+1,-1), ivec2(-1,+1), ivec2(-1,-1));

void makePoint(int i, int j, bool end){
    vec3 pos;
    if (end){
        pos = gsIN[j].pos
            + gsIN[j].halfExt.x * endCorners[i].x * gsIN[j].N
            + gsIN[j].halfExt.y * endCorners[i].y * gsIN[j].B;
    }
    else{
        pos = gsIN[j].pos
            + gsIN[j].halfExt.x * corners[i].x * gsIN[j].N
            + gsIN[j].halfExt.y * corners[i].y * gsIN[j].B;
    }
    
    gl_Position = MVP * vec4(pos, 1.0);
    gsColor = gsIN[j].color;
    if (end) gsColor.rgb *= 0.95;
    else if (i%2==0) gsColor.rgb *= 0.9;

    EmitVertex();
}

void makeSelectionPoint(int j){
    vec4 pos = MVP * vec4(gsIN[j].pos, 1.0);
    pos.z = 0.0;
    float dim = gsIN[j].halfExt.x * Scale;
    gsColor = vec4(1.0, 1.0, 1.0, 2.0) - gsIN[j].color;

    gl_Position = pos + vec4(dim, 0.0, 0.0, 0.0);
    EmitVertex();
    gl_Position = pos + vec4(-dim * 0.5, -dim, 0.0, 0.0);
    EmitVertex();
    gl_Position = pos + vec4(-dim * 0.5, dim, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}

void main(){
    for(int i = 0; i <= 4; i++)
        for(int j = 0; j < 2; j++)
            makePoint(i%4, j, false);
    EndPrimitive();

    if (gsIN[0].u == 0.0f){
        for(int i = 2; i < 6; i++) // start at 2 because the orientation flips
            makePoint(i%4, 0, true);
        EndPrimitive();
    }
    else if (gsIN[1].u == 1.0f){
        for(int i = 0; i < 4; i++)
            makePoint(i, 1, true);
        EndPrimitive();
    }

    // draw selection point
    if (Selection == 0) return;
    makeSelectionPoint(1);
    if (gsIN[0].u == 0.0f)
        makeSelectionPoint(0);
}