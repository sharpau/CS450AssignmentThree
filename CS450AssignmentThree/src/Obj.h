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
	int add_face(GLuint vertex_idx, GLuint texture_coord_idx = 0, GLuint normal_idx = 0);

	// data
	std::vector<GLfloat> indexed_vertices;
	std::vector<GLfloat> indexed_normals;
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> param_space_vertices;
	std::vector<GLfloat> texture_coords;
	std::vector<GLfloat> normals;
	std::vector<GLuint> vertex_indicies;
	std::vector<GLint> texture_coord_indicies;
	std::vector<GLuint> normal_indicies;

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