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

layout(location = 0) out highp vec4 gBuffer_albedo;
layout(location = 1) out lowp  vec3 gBuffer_normal;
layout(location = 2) out highp vec3 gBuffer_position;

void main() {
	gBuffer_albedo = vec4(v2f.color, 1);
	gBuffer_normal = normalize(v2f.normal);
	gBuffer_position = v2f.pos;
}
