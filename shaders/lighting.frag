#version 460 core

precision highp int;
precision highp float;
precision highp sampler2D;

layout(std430, push_constant) uniform LightSource {
	int type; // 0 = point, 1 = spot, 2 = ambient (skybox)
	vec3 color;
	float intensity;
	vec3 viewPosition;
	float innerAngle;
	vec3 viewDirection;
	float outerAngle;
	mat4 cameraViewToShadowMapMatrix;
} lightSource;

struct GBuffers {
	highp vec4 albedo;
	lowp  vec3 normal;
	highp vec3 position;
};

layout(set = 0, binding = 0) uniform CameraUBO {
	dmat4 cameraViewMatrix;
};

// G-Buffers
layout(set = 1, input_attachment_index = 0, binding = 0) uniform highp subpassInput gBuffer_albedo;
layout(set = 1, input_attachment_index = 1, binding = 1) uniform highp  subpassInput gBuffer_normal;
layout(set = 1, input_attachment_index = 2, binding = 2) uniform highp subpassInput gBuffer_position;

layout(set = 1, binding = 3) uniform sampler2D shadowMap;
layout(set = 1, binding = 4) uniform samplerCube skybox;

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

	// Ambient (skybox)
	if (lightSource.type == 2) {
		// Reflections
		color = texture(skybox, transpose(mat3(cameraViewMatrix)) * reflect((gBuffers.position), gBuffers.normal)).rgb;
		color *= lightSource.intensity * lightSource.color;
	} else {
		// Blinn-Phong

		// diffuse
		vec3 lightDir = normalize(lightSource.viewPosition - gBuffers.position);
		float diff = max(dot(gBuffers.normal, lightDir), 0.0);
		vec3 diffuse = diff * lightSource.color * lightSource.intensity;

		// specular
		float specularStrength = 0.5;
		vec3 viewDir = normalize(-gBuffers.position);
		vec3 reflectDir = reflect(-lightDir, gBuffers.normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = specularStrength * spec * lightSource.color;

		// Spot light
		if (lightSource.type == 1) {
			float innerCutOff = cos(radians(lightSource.innerAngle));
			float outerCutOff = cos(radians(lightSource.outerAngle));
			float theta = dot(lightDir, normalize(-lightSource.viewDirection));
			float epsilon = (innerCutOff - outerCutOff);
			float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

			// Shadow map
			float shadow = 0.0;
			vec4 pos = lightSource.cameraViewToShadowMapMatrix * vec4(gBuffers.position, 1);
			vec3 lightSpacePos = (pos.xyz / abs(pos.w));
			lightSpacePos.z = clamp(lightSpacePos.z, 0, 1);
			float shadowBias = max(0.01 * (1.0 - dot(lightDir, gBuffers.normal)), 0.001);
			int pcfSampleSize = 5;
			vec2 shadowMapSize = 1.0 / textureSize(shadowMap, 0);
			for (int x = -pcfSampleSize; x <= pcfSampleSize; ++x) {
				for (int y = -pcfSampleSize; y <= pcfSampleSize; ++y) {
					float depthMap = texture(shadowMap, lightSpacePos.xy / 2.0 + 0.5 + vec2(x,y) * shadowMapSize).r;
					shadow += (depthMap - shadowBias) > lightSpacePos.z? 1 : 0;
				}
			}
			shadow /= (pcfSampleSize*2+1)*(pcfSampleSize*2+1);
			intensity *= (1-min(1,shadow));

			diffuse *= intensity;
			specular *= intensity;
		}

		color *= diffuse + specular;
	}

	out_color = vec4(color, 1);
}
