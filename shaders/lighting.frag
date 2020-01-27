#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

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

// G-Buffers
layout(set = 1, input_attachment_index = 0, binding = 0) uniform highp subpassInput gBuffer_albedo;
layout(set = 1, input_attachment_index = 1, binding = 1) uniform lowp  subpassInput gBuffer_normal;
layout(set = 1, input_attachment_index = 2, binding = 2) uniform highp subpassInput gBuffer_position;

layout(set = 1, binding = 3) uniform sampler2D shadowMap;

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

	// Blinn-Phong

	// diffuse
	vec3 norm = normalize(gBuffers.normal);
	vec3 lightDir = normalize(lightSource.viewPosition - gBuffers.position);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightSource.color * lightSource.intensity;

	// specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(-gBuffers.position);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightSource.color;

	// Spot light
	if (lightSource.type == 1) {
		float innerCutOff = cos(radians(lightSource.innerAngle));
		float outerCutOff = cos(radians(lightSource.outerAngle));
		float theta = dot(lightDir, normalize(-lightSource.viewDirection));
		float epsilon = (innerCutOff - outerCutOff);
		float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);
		diffuse *= intensity;
		specular *= intensity;
	}

	color *= diffuse + specular;

	if (length(color) < 0.001) discard;
	out_color = vec4(color, 1);
}
