#version 150 core
uniform mat4 ModelView;
uniform mat4 Projection;
uniform int triangle_count;

in vec4 vPosition;
in vec4 vVelocity;
in vec4 vColor;

out vec4 color;
out vec4 position_out;
out vec4 velocity_out;
uniform samplerBuffer geometry_tbo;
uniform float time_step = .02;

float XMIN = -1.;
float XMAX = 1.;
float YMIN = -1.;
float YMAX = 1.;
float ZMIN = -1.;
float ZMAX = 1.;

bool intersect(vec3 origin, vec3 direction, vec3 v0, vec3 v1, vec3 v2, out vec3 point)
{
	vec3 u, v, n;
	vec3 w0, w;
	float r, a, b;
	
	u = (v1 - v0);
	v = (v2 - v0);
	n = cross(u, v);
	w0 = origin - v0;
	a = -dot(n, w0);
	b = dot(n, direction);
	r = a / b;
	
	if (r < 0.0 || r > 1.0)
		return false;
	
	point = origin + r * direction;
	float uu, uv, vv, wu, wv, D;
	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	w = point - v0;
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv * uv - uu * vv;
	float s, t;
	s = (uv * wv - vv * wu) / D;
	
	if (s < 0.0 || s > 1.0)
		return false;
	t = (uv * wu - uu * wv) / D;
	
	if (t < 0.0 || (s + t) > 1.0)
		return false;
	return true;
}

vec3 reflect_vector(vec3 v, vec3 n)
{
	return v - 2.0 * dot(v, n) * n;
}

void main(void)
{
	vec3 acceleration = vec3(0.0, -0.3, 0.0);
	vec3 new_velocity = vVelocity.xyz + acceleration * time_step;
	vec4 new_position = vPosition + vec4(new_velocity * time_step, 0.0);
	vec3 v0, v1, v2;
	vec3 point;
	int i;
	for (i = 0; i < triangle_count; i++)
	{
		v0 = texelFetch(geometry_tbo, i * 3).xyz;
		v1 = texelFetch(geometry_tbo, i * 3 + 1).xyz;
		v2 = texelFetch(geometry_tbo, i * 3 + 2).xyz;
	
		if (intersect(vPosition.xyz, vPosition.xyz - new_position.xyz, v0, v1, v2, point))
		{
			vec3 n = normalize(cross(v1 - v0, v2 - v0));
			new_position = vec4(point + reflect_vector(new_position.xyz - point, n), 1.0);
			new_velocity = 0.8 * reflect_vector(new_velocity, n);
		}
	}

	if (new_position.y < -40.0)
	{
		new_position = vec4(-new_position.x * 0.3, vPosition.y + 80.0,
		0.0, 1.0);
		new_velocity *= vec3(0.2, 0.1, -0.3);
	}
	velocity_out = vec4(new_velocity * 0.9999, 0.);
	

	position_out = new_position;
	gl_Position = Projection * (ModelView * vPosition);
	color = vColor;
} // Note the mystical semicolon the red book includes... Don't really know why