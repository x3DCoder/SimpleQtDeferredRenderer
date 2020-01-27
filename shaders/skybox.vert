#version 460 core

layout(location = 0) out int index;

void main(void) {
    index = gl_VertexIndex;
}
