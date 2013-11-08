// Simple 2D OpenGL Program

//Includes vec, mat, and other include files as well as macro defs
#define GL3_PROTOTYPES

// Include the vector and matrix utilities from the textbook, as well as some
// macro definitions.
#include "Angel.h"
#include <stdio.h>
#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#endif

// My includes
#include <sstream>
#include <iostream>
#include <fstream>
#include "Obj.h"
#include "SceneLoader.h"

using namespace std;
// globals
const std::string DATA_DIRECTORY_PATH = "./Data/";
int *idx_size = new int();

typedef Angel::vec4  color4;
typedef Angel::vec4  point4;


// global variables
GLuint  model_view;  // model-view matrix uniform shader variable location
GLuint  projection; // projection matrix uniform shader variable location

GLuint vao;

GLuint buffer;
GLint num_indicies;
GLint num_verts;

GLint num_objects;

int load_scene_by_file(string filename, vector<string>& obj_filename_list)
{
	ifstream input_scene_file;
	string line;
	string filepath;
	int status = -1;

	filepath = DATA_DIRECTORY_PATH + filename;

	input_scene_file.open(filepath);
	if(input_scene_file.is_open())
	{
		getline(input_scene_file, line);
		while(!input_scene_file.eof())
		{
			getline(input_scene_file, line);
			obj_filename_list.push_back(line);
			cout << line << endl;
		}
		status = 0;
		input_scene_file.close();
	} else {
		status = -1;
	}
	return status;
}

// OpenGL initialization
vector<Obj*> obj_data;
void
init(GLfloat in_eye[3], GLfloat in_at[3], GLfloat in_up[3])
{
    // Create a vertex array object
    GLuint *vaos;
	vaos = (GLuint *) malloc( sizeof(GLuint) * obj_data.size() );
    glGenVertexArrays( obj_data.size(), vaos );
	
	GLuint *vbos;
	vbos = (GLuint *) malloc( sizeof(GLuint) * obj_data.size() );
	glGenBuffers( obj_data.size(), vbos );
	
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "./src/vshader.glsl", "./src/fshader.glsl" );
    glUseProgram( program );
	
	for( int i = 0; i < obj_data.size(); i++ )
	{
		glBindVertexArray( vaos[i] );
		obj_data[i]->vao = vaos[i];
		glBindBuffer( GL_ARRAY_BUFFER, vbos[i] );
		GLsizei num_bytes_vert_data = sizeof(GLfloat) * obj_data[i]->indexed_vertices.size();
		GLsizei num_bytes_norm_data = sizeof(GLfloat) * obj_data[i]->indexed_normals.size();
		GLvoid *vert_data = obj_data[i]->indexed_vertices.data();
		GLvoid *norm_data = obj_data[i]->indexed_normals.data();
		glBufferData( GL_ARRAY_BUFFER, num_bytes_vert_data + num_bytes_vert_data, NULL, GL_STATIC_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, num_bytes_vert_data, vert_data );
		glBufferSubData( GL_ARRAY_BUFFER, num_bytes_vert_data, num_bytes_norm_data, norm_data );

		GLint vertLoc = glGetAttribLocation( program, "vPosition" );
		glEnableVertexAttribArray( vertLoc );
		glVertexAttribPointer( vertLoc, obj_data[i]->vertex_element_size, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0 );
		GLint normLoc = glGetAttribLocation( program, "vNormal" );
		glEnableVertexAttribArray( normLoc );
		glVertexAttribPointer( normLoc, obj_data[i]->normal_element_size, GL_FLOAT, GL_FALSE, 0, (GLvoid *) num_bytes_vert_data );
		int linked;
		glGetProgramiv( program, GL_LINK_STATUS, &linked );
		if( linked != GL_TRUE )
		{
			int maxLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
			maxLength = maxLength + 1;
			GLchar *pLinkInfoLog = new GLchar[maxLength];
			glGetProgramInfoLog(program, maxLength, &maxLength, pLinkInfoLog);
			cerr << *pLinkInfoLog << endl;
		}

		num_verts = obj_data[i]->indexed_vertices.size();
	}
    // Initialize shader lighting parameters
    // RAM: No need to change these...we'll learn about the details when we
    // cover Illumination and Shading
    point4 light_position( 0., 1.5, 1.3, 1.0 );
    color4 light_ambient( 0.2, 0.2, 0.2, 1.0 );
    color4 light_diffuse( 1.0, 1.0, 1.0, 1.0 );
    color4 light_specular( 1.0, 1.0, 1.0, 1.0 );

    color4 material_ambient( 1.0, 0.0, 1.0, 1.0 );
    color4 material_diffuse( 1.0, 0.8, 0.0, 1.0 );
    color4 material_specular( 1.0, 0.8, 0.0, 1.0 );
    float  material_shininess = 100.0;

    color4 ambient_product = light_ambient * material_ambient;
    color4 diffuse_product = light_diffuse * material_diffuse;
    color4 specular_product = light_specular * material_specular;

    glUniform4fv( glGetUniformLocation(program, "AmbientProduct"),
		  1, ambient_product );
    glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"),
		  1, diffuse_product );
    glUniform4fv( glGetUniformLocation(program, "SpecularProduct"),
		  1, specular_product );

    glUniform4fv( glGetUniformLocation(program, "LightPosition"),
		  1, light_position );

    glUniform1f( glGetUniformLocation(program, "Shininess"),
		 material_shininess );


    model_view = glGetUniformLocation( program, "ModelView" );
    projection = glGetUniformLocation( program, "Projection" );



    mat4 p = Perspective(90.0, 1.0, 0.1, 4.0);
    point4  eye( in_eye[0], in_eye[1], in_eye[2], 1.0);
    point4  at( in_at[0], in_at[1], in_at[2], 1.0 );
    vec4    up( in_up[0], in_up[1], in_up[2], 0.0 );


    mat4  mv = LookAt( eye, at, up );
    //vec4 v = vec4(0.0, 0.0, 1.0, 1.0);

    glUniformMatrix4fv( model_view, 1, GL_TRUE, mv );
    glUniformMatrix4fv( projection, 1, GL_TRUE, p );


    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 );
}

//----------------------------------------------------------------------------

void
display( void )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	for( auto obj : obj_data )
	{
		glBindVertexArray( obj->vao );
		glDrawArrays( GL_TRIANGLES, 0, obj->indexed_vertices.size() );
	}
    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
	case 033:  // Escape key
	case 'q': case 'Q':
	    exit( EXIT_SUCCESS );
	    break;
    }
}

//----------------------------------------------------------------------------



/*
 *  simple.c
 *  This program draws a red rectangle on a white background.
 *
 * Still missing the machinery to move to 3D
 */

/* glut.h includes gl.h and glu.h*/

int main(int argc, char** argv)
{
	string data_filename = "test.scn";
	string application_info = "CS450AssignmentTwo: ";
	string *window_title = new string;
	GLfloat eye_position[] = { 0., 0., 1., 1.};
	GLfloat at_position[] = { 0., 0., 0., 1. };
	GLfloat up_vector[] = { 0., 1., 0., 0. };

	if(argc != 11) {
		cerr << "USAGE: Expected 10 arguments but found '" << (argc - 1) << "'" << endl;
		cerr << "CS450AssignmentTwo SCENE_FILENAME FROM_X FROM_Y FROM_Z AT_X AT_Y AT_Z UP_X UP_Z UP_Y" << endl;
		cerr << "SCENE_FILENAME: A .scn filename existing in the ./CS450AssignmentTwo/Data/ directory." << endl;
		cerr << "FROM_X, FROM_Y, FROM_Z*: Floats passed to the LookAt function representing the point in the scene of the eye." << endl;
		cerr << "AT_X, AT_Y, AT_Z*: Floats passed to the LookAt function representing the point in the scene where the eye is looking." << endl;
		cerr << "UP_X, UP_Y, UP_Z*: Floats passed to the LookAt function representing the vector that describes the up direction within scene for the eye." << endl;
		cerr << "*These points and vectors will not be converted to a homogenous coordinate system." << endl;
		return -1;
	}
	data_filename = argv[1];

	eye_position[0] = atof(argv[2]);
	eye_position[1] = atof(argv[3]);
	eye_position[2] = atof(argv[4]);

	at_position[0] = atof(argv[5]);
	at_position[1] = atof(argv[6]);
	at_position[2] = atof(argv[7]);

	up_vector[0] = atof(argv[8]);
	up_vector[1] = atof(argv[9]);
	up_vector[2] = atof(argv[10]);

	cout << "Loading scene file: '" << data_filename.c_str() << "'" << endl;
	cout << "Eye position: {" << eye_position[0] << ", " << eye_position[1] << ", " << eye_position[2] << "}" << endl;
	cout << "At position: {" << at_position[0] << ", " << at_position[1] << ", " << at_position[2] << "}" << endl;
	cout << "Up vector: {" << up_vector[0] << ", " << up_vector[1] << ", " << up_vector[2] << "}" << endl;
	
	SceneLoader *input_scene = new SceneLoader( DATA_DIRECTORY_PATH );
	input_scene->load_file( data_filename );
	for( auto tmp : input_scene->loaded_objs )
	{
		obj_data.push_back(tmp);
	}
	glutInit(&argc, argv);
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DEPTH_TEST);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion (3, 2);
    glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
#endif
	window_title->append(application_info);
	window_title->append(data_filename);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(500, 300);
    glutCreateWindow(window_title->c_str());
    printf("%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	init(eye_position, at_position, up_vector);

    //NOTE:  callbacks must go after window is created!!!
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutMainLoop();

    return(0);
}
