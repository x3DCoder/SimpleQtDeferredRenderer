#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

void main() {
	// Full screen triangle using 3 empty vertices
	gl_Position = vec4(vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f + -1.0f, 0.0f, 1.0f);
}
