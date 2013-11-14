#version 150

in  vec4 vPosition;
in  vec4 vNormal;
in	vec4 vColor;
out vec4 color;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
uniform float Shininess;

// params for selection
// flag: 0 == normal, 1 == selection, 2 == absolute coloring (manipulators)
uniform int flag;
uniform int selectionColorR;
uniform int selectionColorG;
uniform int selectionColorB;
uniform int selectionColorA;

void main()
{
    // Transform vertex  position into eye coordinates
    vec3 pos = (ModelView * vPosition).xyz;

    vec3 L = normalize( (ModelView * LightPosition).xyz - pos );
    vec3 E = normalize( -pos );
    vec3 H = normalize( L + E );  //halfway vector

    //To correctly transform normals
    vec3      N = normalize( transpose(inverse( ModelView )) * vNormal ).xyz;

    // Compute terms in the illumination equation
    vec4 ambient = AmbientProduct;

    float dr = max( dot(L, N), 0.0 );
    vec4  diffuse = dr *DiffuseProduct;

    float sr = pow( max(dot(N, H), 0.0), Shininess );
    vec4  specular = sr * SpecularProduct;

    if( dot(L, N) < 0.0 ) {
	specular = vec4(0.0, 0.0, 0.0, 1.0);
    }

    gl_Position = Projection * ModelView * vPosition;

    if(flag == 0) {
		color = ambient + diffuse + specular;
		color.a = 1.0;
	} 
	else if(flag == 1) {
		color.r = float(selectionColorR)/float(255);
		color.g = float(selectionColorG)/float(255);
		color.b = float(selectionColorB)/float(255);
		color.a = float(selectionColorA)/float(255);
	}
	else if(flag == 2) {
		color = vColor;
	}
}
