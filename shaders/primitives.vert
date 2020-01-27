#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

layout(set = 0, binding = 0) uniform Camera {
	int width;
	int height;
	dvec3 worldPosition;
	double fov;
	dvec3 lookDirection;
	double znear;
	dvec3 viewUp;
	double zfar;
	dmat4 viewMatrix;
	dmat4 projectionMatrix;
} camera;

layout(std430, push_constant) uniform ModelViewMatrix {
	mat4 modelViewMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

struct V2F {
	vec3 pos;
	vec3 normal;
	vec3 color;
};

layout(location = 0) out V2F v2f;

void main(void) {
    gl_Position = mat4(camera.projectionMatrix) * modelViewMatrix * vec4(pos, 1);
    v2f.pos = (modelViewMatrix * vec4(pos, 1)).xyz;
    v2f.normal = normalize(transpose(inverse(mat3(modelViewMatrix))) * normal);
    v2f.color = color;
}
