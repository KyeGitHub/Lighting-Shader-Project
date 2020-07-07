// VERTEX SHADER
#version 330

// Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 vertexPosition;
out vec3 vertexNormal;
out vec2 vertexTexCoord;

void main(void) 
{
	// position to model space
	vertexPosition = (matrixModelView * vec4(aVertex, 1.0)).xyz;

	// normal to model space
	vertexNormal = mat3(matrixModelView) * aNormal;

	// just pass tex coords
	vertexTexCoord = aTexCoord;
	
	// position to model-view-projection space
	gl_Position = matrixProjection * vec4(vertexPosition, 1.0);
}
