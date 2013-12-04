#version 150 core

in  vec4 vPosition;

out vec4 color;

// for transform buffer
out vec4 world_space_position;

uniform mat4 ModelView;
uniform mat4 Projection;

void main()
{
    // Transform vertex  position into eye coordinates
    vec4 pos = (ModelView * vPosition);
	world_space_position = pos;

    gl_Position = Projection * pos;
	color = vec4(.0);
    
}
