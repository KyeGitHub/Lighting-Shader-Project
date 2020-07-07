// FRAGMENT SHADER
#version 330

// Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Material structure.
// struct Material code
struct MATERIAL
{
	vec3 ambient;
	vec3 diffuse;
	float shininess;
	vec3 emissive;
	sampler2D diffuseTexture;
	sampler2D normalTexture;
};

// Only one material per drawn model.
uniform MATERIAL material;


// Calculates diffuse colour.
// intensity = factor of angle between light direction and vertex normal.
vec3 CalculateDiffuse(vec3 lightDir, vec3 vertexN, vec3 lightDiffuse)
{
	vec3 finalDiffuse = material.diffuse;

	float intensity = dot(vertexN, -lightDir);
	if (intensity > 0)
		finalDiffuse += lightDiffuse * intensity;

	return finalDiffuse;
}

// Calculates specular colour.
// intensity = factor of angle between reflected light direction by vertex normal (where light goes) and eye to vertex direction (e.g. how much light goes into eye).
//             Then exponentially modified with specular strength and material's own shininess.
vec3 CalculateSpecular(vec3 lightDir, vec3 vertexN, vec3 vertexP, vec3 lightSpecular, float lightSpecularPower)
{
	vec3 finalSpecular = vec3(0, 0, 0);

	vec3 eyeToVertDir = normalize(-vertexP);
	vec3 reflectDir = normalize(reflect(-lightDir, vertexN));

	float intensity = dot(eyeToVertDir, reflectDir);
	if (intensity > 0 && lightSpecularPower > 0) {
		intensity = material.shininess * pow(intensity, lightSpecularPower);
		finalSpecular += lightSpecular * intensity;
	}

	return finalSpecular;
}

// Calculates light attenuation, i.e. how much weaker a point light gets based of distance.
float CalculateAttenuation(float distanceToLightSource, float lightRadius, float lightCutoff)
{
	float d = max(distanceToLightSource - lightRadius, 0.0);
	float denom = d / lightRadius + 1;
	float attenuation = 1 / (denom * denom);
	
	attenuation = (attenuation - lightCutoff) / (1 - lightCutoff);
	attenuation = max(attenuation, 0.0);

	return attenuation;
}


struct DIRECTIONAL_LIGHT
{ 
	int on;

	vec3 direction;

	vec3 ambient;

	vec3 diffuse;
	float diffuseStrength;

	vec3 specular;
	float specularPower;
};

uniform DIRECTIONAL_LIGHT lightDirectional;

struct POINT_LIGHT
{
	int on;

	vec3 position;
	
	vec3 ambient;

	vec3 diffuse;
	float diffuseStrength;

	vec3 specular;
	float specularPower;

	float radius;
	float cutoff;
};

// Up to two point lights.
#define MAX_POINT_LIGHTS 2
uniform POINT_LIGHT lightPoint[MAX_POINT_LIGHTS];

// Calculates total colour of the active directional light. i.e. ambient + diffuse + specular.
vec3 CalculateDirectionalLightColour(vec3 vertexP, vec3 vertexN)
{
	DIRECTIONAL_LIGHT light = lightDirectional;

	vec3 colour = vec3(0, 0, 0);

	if (light.on != 0)
	{
		// Need to convert light direction to view space...
		vec3 lightDirection = mat3(matrixView) * light.direction;

		colour += material.ambient * light.ambient;
		colour += CalculateDiffuse(lightDirection, vertexN, light.diffuse) * light.diffuseStrength;
		colour += CalculateSpecular(lightDirection, vertexN, vertexP, light.specular, light.specularPower);
	}

	return colour;
}

// Calculates total colour of all active point light, i.e. ambient + diffuse + specular (optionally lowered with attenuation)
vec3 CalculatePointLightsColour(vec3 vertexP, vec3 vertexN)
{
	vec3 finalColour = vec3(0, 0, 0);

	for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		POINT_LIGHT light = lightPoint[i];
	
		if (light.on != 0)
		{
			// Need to convert light position to view space...
			vec3 lightPosition = (matrixView * vec4(light.position, 1)).xyz;

			vec3 lightDirection = vertexP - lightPosition;

			float distToP = length(lightDirection);
			if (distToP > 0)
			{
				lightDirection /= distToP;

				vec3 colour = light.ambient;
				colour += CalculateDiffuse(lightDirection, vertexN, light.diffuse) * light.diffuseStrength;
				colour += CalculateSpecular(lightDirection, vertexN, vertexP, light.specular, light.specularPower);

				if (light.radius > 0)
				{
					float attenuation = CalculateAttenuation(distToP, light.radius, light.cutoff);
					colour *= attenuation;
				}

				finalColour += colour;
			}
		}
	}

	return finalColour;
}


// These come from the vertex shader
// pos and normal are in model space
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

out vec4 outColor;

void main(void) 
{
	// albedo 
	vec3 albedo = texture(material.diffuseTexture, vertexTexCoord).rgb;

	// The vertex normal x normal map value 
	vec3 normalMapValue = texture(material.normalTexture, vertexTexCoord).rgb * 2 - vec3(1, 1, 1);
	vec3 normalMapInModelSpace = mat3(matrixModelView) * normalMapValue;
	vec3 normal = normalize(vertexNormal + normalMapInModelSpace);

	// Sum of all lighting 
	vec3 lighting = vec3(0, 0, 0);

	lighting += CalculateDirectionalLightColour(vertexPosition, normal);
	lighting += CalculatePointLightsColour(vertexPosition, normal);

	// add emissive after
	lighting += material.emissive;

	outColor = vec4(albedo * lighting, 1);
}
