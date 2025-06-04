#version 430

layout(lines) in;
layout(triangle_strip, max_vertices = 14) out;

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
}