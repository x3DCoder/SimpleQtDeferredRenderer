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

layout(std430, push_constant) uniform LightSource {
	dvec3 worldPosition;
	vec3 color;
	float intensity;
	vec3 worldDirection;
	int type; // 0 = point, 1 = spot
	vec3 viewPosition;
	float innerAngle;
	vec3 viewDirection;
	float outerAngle;
} lightSource;

struct GBuffers {
	highp vec4 albedo;
	lowp  vec3 normal;
	highp vec3 position;
};

layout(set = 1, input_attachment_index = 0, binding = 0) uniform highp subpassInput gBuffer_albedo;
layout(set = 1, input_attachment_index = 1, binding = 1) uniform lowp  subpassInput gBuffer_normal;
layout(set = 1, input_attachment_index = 2, binding = 2) uniform highp subpassInput gBuffer_position;

GBuffers LoadGBuffers() {
	return GBuffers(
		subpassLoad(gBuffer_albedo).rgba,
		subpassLoad(gBuffer_normal).xyz,
		subpassLoad(gBuffer_position).xyz
	);
}

layout(location = 0) out vec4 out_color;

void main(void) {
    GBuffers gBuffers = LoadGBuffers();
	vec3 color = gBuffers.albedo.rgb;
	
    // TODO lighting

	out_color = vec4(color, 1);
}
