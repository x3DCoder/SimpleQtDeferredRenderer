#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

struct V2F {
	vec3 pos;
	vec3 normal;
	vec3 color;
};

layout(location = 0) in V2F v2f;

struct GBuffers {
	highp vec4 albedo;
	lowp  vec3 normal;
	highp vec3 position;
};

layout(location = 0) out highp vec4 gBuffer_albedo;
layout(location = 1) out lowp  vec3 gBuffer_normal;
layout(location = 2) out highp vec3 gBuffer_position;

void WriteGBuffers(GBuffers gBuffers) {
	gBuffer_albedo = gBuffers.albedo;
	gBuffer_normal = gBuffers.normal;
	gBuffer_position = gBuffers.position;
}

void main() {
	GBuffers gBuffers;
	
	gBuffers.albedo = vec4(v2f.color, 1);
	gBuffers.normal = v2f.normal;
	gBuffers.position = v2f.pos;
	
	WriteGBuffers(gBuffers);
}
