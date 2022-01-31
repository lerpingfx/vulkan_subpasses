#version 450

layout(location = 0) in vec3 v2fCol;
layout(location = 1) in vec2 v2fUV;
layout(location = 2) in vec2 v2fScreenUV;
layout(location = 3) in vec3 v2fWorldPos;
layout(location = 4) in vec3 v2fWorldNormal;

layout(set = 0, binding = 0) uniform uboFX{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec2 res;
} ubo;

//layout(set = 0, binding = 1) uniform sampler2D textureSampler;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColorAttachment;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepthAttachment;

layout(location = 0) out vec4 outColor;


void main() {

	vec3 worldNormal = normalize(v2fWorldNormal);
	float dotprod = dot(worldNormal, vec3(0.0, 0.0, -1.0));
	float angle = abs(acos(dotprod/(1.0)));

	float outline = smoothstep(3.14*0.25, 3.14*0.5, angle);	
	
	vec3 viewOriginWorldPos = vec3(0.0f, 1.0f, -4.0f);
	vec3 viewDir = normalize(v2fWorldPos - viewOriginWorldPos);
	vec4 inCol = subpassLoad(inputColorAttachment);
	float inDepth = subpassLoad(inputDepthAttachment).x; // depth is [-1, 1]
	 
	// calculate attachment's world-space position
		// Vulkan coordinate system is [-1, 1]
	vec2 ndc_xy = v2fScreenUV * 2.0 - 1.0; // remap to [-1,1]
	vec4 inAttachment_ndc = vec4(ndc_xy, inDepth, 1.0); 
	
	mat4 inv_viewProj = inverse(ubo.proj * ubo.view);
	vec4 inWorld_w = inv_viewProj * inAttachment_ndc;
	vec3 inWorld   = inWorld_w.xyz / inWorld_w.w;
	
	/*
	float T1 = ubo.proj[2][2];
	float T2 = ubo.proj[3][2];
	float viewSpaceZ = -T2 / (inAttachment_ndc.z + T1);
	float clispaceW = -viewSpaceZ;
	vec4 sceneClipSpace = vec4(inAttachment_ndc.xyz * clipSpaceW, clipSpaceW);
	vec4 sceneViewSpace = inverse(ubo.proj) * sceneClipSpace;
	*/
	
	// compare attachment's and fragment's worldspace positions
	float glow =  smoothstep(0.0,1.5,1.0-(inWorld.z-(v2fWorldPos.z-0.65)));
	float highlight = smoothstep(0.0,1.5,1.0-(inWorld.z-(v2fWorldPos.z-0.75)));
	float intersect = smoothstep(0.0,1.0,1.0-(inWorld.z-(v2fWorldPos.z-0.8)));
	
	//float nDepth = inDepth * 0.5 + 0.5; // [0, 1]
	//float a = smoothstep(0.99965, 1.000, nDepth);
	
	vec3 col = mix(vec3(0.0),vec3(1.0,1.0,10.0), outline+glow);
	
	//float pattern = texture(textureSampler, v2fScreenUV .xy);
	
	//float mask = (raymarch * raymarch);
	col += mix(vec3(0.0),vec3(1.0,1.0,10.0), highlight + highlight);
	col += mix(vec3(0.0),vec3(1.0,1.0,10.0), intersect * 4.0 );
    outColor = vec4(col, 0.75);
	
}