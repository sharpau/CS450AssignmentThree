#ifndef __OBJH__
#define __OBJH__
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Angel.h"

// for reading in and representing Wavefront OBJ files
class Obj
{
public:
	// constructors & destructors
	Obj();
	Obj(std::string filename);
	Obj(std::string in_filename, mat4 in_model_view);
	~Obj() {};

	// methods
	int load_from_file(std::string filename);
	int add_vertex(GLfloat x, GLfloat y, GLfloat z = 0., GLfloat w = 0.);
	int add_texture_coord(GLfloat texture_coord_u, GLfloat texture_coord_v, GLfloat texture_coord_w = 0);
	int add_normal(GLfloat normal_x, GLfloat normal_y, GLfloat normal_z, GLfloat normal_w = 0);
	int add_param_vertex(GLfloat vertex_u, GLfloat vertex_v = 0, GLfloat vertex_w = 0);
	int add_face(GLint vertex_idx, GLint texture_coord_idx = 0, GLint normal_idx = 0);

	// data
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> param_space_vertices;
	std::vector<GLfloat> texture_coords;
	std::vector<GLfloat> normals;
	std::vector<GLint> vertex_indicies;
	std::vector<GLint> texture_coord_indicies;
	std::vector<GLint> normal_indicies;

	// Object Transformations
	mat4 scaleXYZ;
	mat4 rotateXYZ;
	mat4 translateXYZ;
	mat4 model_view;

	// file characteristics / metadata
	std::string filename;
	bool is_loaded;
	bool bad_file;
	
	int vertex_element_size;
	int normal_element_size;
	int texture_coord_element_size;
	int param_space_vertex_element_size;
};
#endif // END __OBJH__