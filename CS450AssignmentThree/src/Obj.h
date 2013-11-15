#ifndef __OBJH__
#define __OBJH__
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Angel.h"

// for reading in and representing Wavefront OBJ files
struct SOA {
	std::vector<GLfloat> positions;
	std::vector<GLfloat> normals;
	std::vector<GLfloat> colors;
	GLuint positions_stride;
	GLuint normals_stride;
	GLuint colors_stride;
	GLuint num_vertices;
};
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

	// data
	struct SOA data_soa;

	GLuint vao;
	// Object Transformations
	mat4 scaleXYZ;
	mat4 rotateX;
	mat4 rotateY;
	mat4 rotateZ;
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

	// for custom coloring during selection process
	GLubyte selectionR;
	GLubyte selectionG;
	GLubyte selectionB;
	GLubyte selectionA;
	bool selected;
	GLfloat thetaX;
	GLfloat thetaY;
	GLfloat thetaZ;
	//methods
	int add_vertex(GLfloat x, GLfloat y, GLfloat z = 0., GLfloat w = 0.);
	int add_texture_coord(GLfloat texture_coord_u, GLfloat texture_coord_v, GLfloat texture_coord_w = 0);
	int add_normal(GLfloat normal_x, GLfloat normal_y, GLfloat normal_z, GLfloat normal_w = 0);
	int add_param_vertex(GLfloat vertex_u, GLfloat vertex_v = 0, GLfloat vertex_w = 0);
	int add_face(GLuint vertex_idx, GLuint texture_coord_idx = 0, GLuint normal_idx = 0);
	void load_data( void );

	// internal data
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> param_space_vertices;
	std::vector<GLfloat> texture_coords;
	std::vector<GLfloat> normals;
	std::vector<GLuint> vertex_indicies;
	std::vector<GLint> texture_coord_indicies;
	std::vector<GLuint> normal_indicies;

};
#endif // END __OBJH__