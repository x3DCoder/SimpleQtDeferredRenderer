#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

layout(std430, push_constant) uniform MVP {
	mat4 projectionMatrix;
	mat4 modelViewMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

void main(void) {
    gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1);
}
