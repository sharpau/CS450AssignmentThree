#version 330 core

uniform mat4 Projection;
uniform mat4 ModelView;

// params for selection
// flag: 0 == normal, 1 == selection, 2 == absolute coloring (manipulators)
uniform int flag;
uniform int selectionColorR;
uniform int selectionColorG;
uniform int selectionColorB;
uniform int selectionColorA;

in vec4 VertexColor;
in vec4 VertexNormal;
in vec4 VertexPosition;

mat3 NormalMatrix;

out vec4 Color;
out vec3 Normal;

void main()
{
	Color = vec4(.5, .5, .5, 1.);
	NormalMatrix = mat3(transpose(inverse(ModelView)));
	Normal = vec3(normalize(NormalMatrix * VertexNormal.xyz));
	gl_Position = Projection * ModelView * VertexPosition;
}