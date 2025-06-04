#version 430 core

out flat uint vInstanceID;

void main(){
    vInstanceID = gl_InstanceID;
}
