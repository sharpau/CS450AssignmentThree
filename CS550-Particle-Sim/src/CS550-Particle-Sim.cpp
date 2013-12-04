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
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include "Obj.h"
#include "SceneLoader.h"

using namespace std;
// globals
const string DATA_DIRECTORY_PATH = "./Data/";

// special objects
Obj grid;
Obj manips[3];

// all loaded objects
vector<Obj*> obj_data;

typedef Angel::vec4  color4;
typedef Angel::vec4  point4;


// global variables
GLuint  gModelViewLoc;  // model-view matrix uniform shader variable location
GLuint  gProjectionLoc; // projection matrix uniform shader variable location

mat4  gViewTransform;
mat4 gModelView;

//Selection variables
GLuint gSelectionColorR, gSelectionColorG, gSelectionColorB, gSelectionColorA;
int gPicked = -1;
GLint gFlag = 0;
GLuint gSelectFlagLoc;
GLuint gSelectColorRLoc, gSelectColorGLoc, gSelectColorBLoc, gSelectColorALoc;

GLint gVertLoc, gNormLoc, gColorLoc, gTriangleCountLoc;

// camera transforms
mat4 gCameraTranslate, gCameraRotX, gCameraRotY, gCameraRotZ;

// particles stuff
GLuint gNumParticles = 5;

GLint const WORLD_TRIANGLE_BUFF_IDX = 0;
GLint const POSITIONS_VELOCITIES_BUFF_IDX = 1;

// Shader programs
GLuint gParticleProgram, gPassThroughProgram, gRenderProgram;
mat4 gProjection;
int gFrameCount = 0;

GLint gVelocityLoc;
struct Particles
{
	// interleaved position and velocity data
	vector<vec4> pos_vel_data;
	vector<vec4> colors;

	GLuint colors_vbo;
	GLuint normals_vbo;
	GLuint double_buffer_vbo[2];
	GLuint texture_buffer_object;
	GLuint currVB;
	GLuint currTFB;

	GLuint vao;
	GLuint transformFeedbackObject[2];
} gParticleSys;


bool isLinked(GLuint program)
{
	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		int maxLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
		maxLength = maxLength + 1;
		GLchar *pLinkInfoLog = new GLchar[maxLength];
		glGetProgramInfoLog(program, maxLength, &maxLength, pLinkInfoLog);
		cerr << *pLinkInfoLog << endl;
		return false;
	}
	return true;
};

inline float frand(float lower_bound, float upper_bound)
{
	float r = lower_bound + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (upper_bound - lower_bound)));
	return r;
}
void mount_shader(GLint shader_program)
{
	glUseProgram(shader_program);
	gVertLoc = glGetAttribLocation(shader_program, "vPosition");
	gColorLoc = glGetAttribLocation(shader_program, "vColor");
	gModelViewLoc = glGetUniformLocation(shader_program, "ModelView");
	gProjectionLoc = glGetUniformLocation(shader_program, "Projection");
}

//----------------------------------------------------------------------------
// the HSV color model will be as follows
// h : [0 - 360]
// s : [0 - 1]
// v : [0 - 1]
// If you want it differently (in a 2 * pi scale, 256 instead of 1, etc,
// you'll have to change it yourself.
// rgb is returned in 0-1 scale (ready for color3f)
void HSVtoRGB(float hsv[3], float rgb[3]) {
	float tmp1 = hsv[2] * (1 - hsv[1]);
	float tmp2 = hsv[2] * (1 - hsv[1] * (hsv[0] / 60.0f - (int)(hsv[0] / 60.0f)));
	float tmp3 = hsv[2] * (1 - hsv[1] * (1 - (hsv[0] / 60.0f - (int)(hsv[0] / 60.0f))));
	switch ((int)(hsv[0] / 60)) {
	case 0:
		rgb[0] = hsv[2];
		rgb[1] = tmp3;
		rgb[2] = tmp1;
		break;
	case 1:
		rgb[0] = tmp2;
		rgb[1] = hsv[2];
		rgb[2] = tmp1;
		break;
	case 2:
		rgb[0] = tmp1;
		rgb[1] = hsv[2];
		rgb[2] = tmp3;
		break;
	case 3:
		rgb[0] = tmp1;
		rgb[1] = tmp2;
		rgb[2] = hsv[2];
		break;
	case 4:
		rgb[0] = tmp3;
		rgb[1] = tmp1;
		rgb[2] = hsv[2];
		break;
	case 5:
		rgb[0] = hsv[2];
		rgb[1] = tmp1;
		rgb[2] = tmp2;
		break;
	default:
		std::cout << "Inconceivable!\n";
	}

}

void init_particles()
{
	int count = 0;
	GLfloat dHue = (275. - 90.) / (GLfloat)gNumParticles;
	GLfloat hue0 = 90.;
	for (int idx = 0; idx < gNumParticles; idx++)
	{
		GLfloat hsv[3];
		hsv[0] = hue0 + dHue * idx; // frand(90., 275.);
		hsv[1] = 1.;
		hsv[2] = 1.;
		GLfloat rgb[3] = { 0., 0., 0. };
		HSVtoRGB(hsv, rgb);
		gParticleSys.colors.push_back(vec4(rgb[0], rgb[1], rgb[2], 1.));
		GLfloat particle_min_velocity = -1;
		GLfloat particle_max_velocity = 1.;
		GLfloat particle_min_pos = -1.;
		GLfloat particle_max_pos = 1.;

		// position
		GLfloat x = frand(particle_min_pos, particle_max_pos);
		GLfloat y = frand(particle_min_pos, particle_max_pos);
		GLfloat z = frand(particle_min_pos, particle_max_pos);
		vec4 position(x, y, z, 1.);
		gParticleSys.pos_vel_data.push_back(position);
		// velocity
		GLfloat vX = frand(particle_min_velocity, particle_max_velocity);
		GLfloat vY = frand(particle_min_velocity, particle_max_velocity);
		GLfloat vZ = frand(particle_min_velocity, particle_max_velocity);
		vec4 velocity(vX, vY, vZ, 0.);
		gParticleSys.pos_vel_data.push_back(velocity);
	}
}

GLuint gRenderTFBObject, gRenderVbo;

GLuint gRenderVao;

// OpenGL initialization
void
init(void)
{
	bool linkStatus = false;

	//static const char *varyings[] =
	//{
	//	"position_out", "velocity_out"
	//};


	//glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_INTERLEAVED_ATTRIBS);
	//glLinkProgram(gParticleProgram);
    gCameraRotX = Angel::identity();
	gCameraRotY = Angel::identity();
	gCameraRotZ = Angel::identity();
	gCameraTranslate = Angel::identity();
	

	// Load shaders and use the resulting shader program
	// doing this ahead of time so we can use it for setup of special objects
	gParticleProgram = InitShader("./src/vParticleSystemShader.glsl", "./src/fPassThrough.glsl");
	gPassThroughProgram = InitShader("./src/vPassThrough.glsl", "./src/fPassThrough.glsl");

	mount_shader(gParticleProgram);
	glPointSize(10.);

	gVelocityLoc = glGetAttribLocation(gParticleProgram, "vVelocity");
	gTriangleCountLoc = glGetAttribLocation(gParticleProgram, "triangle_count");

	init_particles();

	glGenVertexArrays(1, &gParticleSys.vao);

	glGenTransformFeedbacks(2, gParticleSys.transformFeedbackObject);
	glGenBuffers(1, &gParticleSys.colors_vbo);
	glGenBuffers(2, gParticleSys.double_buffer_vbo); // these serve as vbos and tfbs

	auto generateDoubleBuffers = [](GLint idx )
	{
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gParticleSys.transformFeedbackObject[idx]);
		
		glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.double_buffer_vbo[idx]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.double_buffer_vbo[idx]);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.pos_vel_data.size()), gParticleSys.pos_vel_data.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(gVertLoc);
		glVertexAttribPointer(gVertLoc, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), BUFFER_OFFSET(0));
		glEnableVertexAttribArray(gVelocityLoc);
		glVertexAttribPointer(gVelocityLoc, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), BUFFER_OFFSET(sizeof(vec4)));
		
		glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.colors_vbo);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.colors.size()), gParticleSys.colors.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(gColorLoc);
		glVertexAttribPointer(gColorLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	};

	glBindVertexArray(gParticleSys.vao);

	generateDoubleBuffers(0);
	generateDoubleBuffers(1);

	gParticleSys.currVB = 0;
	gParticleSys.currTFB = 1;



    gModelViewLoc = glGetUniformLocation(gParticleProgram, "ModelView");
	gProjectionLoc = glGetUniformLocation(gParticleProgram, "Projection");

    point4  eye(2.5, 2.5, 2.5, 1.);
    point4  at(0., 0., 0., 1.);
    vec4    up(0., 1., 0., 0.);
	gCameraTranslate = Translate(-eye);
    gViewTransform = LookAt(eye, at, up);
	gModelView = gViewTransform * Angel::identity();

    glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);


	/*glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(gParticleProgram);*/
	linkStatus = isLinked(gParticleProgram);
/*

	mount_shader(gPassThroughProgram);
	glGenVertexArrays(1, &gRenderVao);
	glGenTransformFeedbacks(1, &gRenderTFBObject);
	glGenBuffers(1, &gRenderVbo);

	glBindVertexArray(gRenderVao);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gRenderTFBObject);
	glBindBuffer(GL_ARRAY_BUFFER, gRenderVbo);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gRenderVbo);
	GLuint theSize = (sizeof(GLfloat)* 4 * obj_data[0]->data_soa.positions.size());
	GLvoid *theData = gParticleSys.colors.data();
	glBufferData(GL_ARRAY_BUFFER, theSize, theData, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	static const char *varyings2[] =
	{
		"world_space_position"
	};
	glTransformFeedbackVaryings(gPassThroughProgram, 1, varyings2, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(gPassThroughProgram);
	linkStatus = isLinked(gPassThroughProgram);*/


	glBindVertexArray(gParticleSys.vao);
	mount_shader(gParticleProgram);

    glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

void print_mappable_buffer(GLenum target, string friendly_name, GLuint stride)
{
	bool t = glIsTransformFeedback(gParticleSys.transformFeedbackObject[0]);
	GLfloat *data = (GLfloat *)glMapBuffer(target, GL_READ_ONLY);
	printf("%s:\n", friendly_name.data());
	for (int i = 0; i < gParticleSys.pos_vel_data.size(); i++)
	{
		printf("%i: ", i);
		int idx = stride * i;
		for (int j = 0; j < stride; j++)
		{
			printf("%f, ", data[idx + j]);
		}
		printf("\n");
	}
	glUnmapBuffer(target);
}
void update_particles(void)
{
	static const char *varyings[] =
	{
		"position_out", "velocity_out"
	};
	glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(gParticleProgram);

	glUseProgram(gParticleProgram);
	glUniform1i(gTriangleCountLoc, 0);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.double_buffer_vbo[0]);
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, gNumParticles);
	glEndTransformFeedback();

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.double_buffer_vbo[1]);
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, gNumParticles);
	glEndTransformFeedback();
	
	print_mappable_buffer(GL_TRANSFORM_FEEDBACK_BUFFER, "Updated Position Velocity", 4);

	printf("\n");
	gFrameCount++;
}

void myIdle(void)
{
	update_particles();
	// swap VB and TFB
	glutPostRedisplay();
}

void render_particles(void)
{
	glUseProgram(gParticleProgram);

	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.double_buffer_vbo[0]);
	print_mappable_buffer(GL_ARRAY_BUFFER, "Rendered Positions Velocities", 4);

	glDrawArrays(GL_POINTS, 0, gNumParticles);
}

void render_geometry(void)
{
	glUseProgram(gPassThroughProgram);
	glBindBuffer(GL_ARRAY_BUFFER, gRenderVbo);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gRenderTFBObject);

	glBeginTransformFeedback(GL_TRIANGLES);

	glDrawArrays(GL_TRIANGLES, 0, obj_data[0]->data_soa.positions.size());

	glEndTransformFeedback();

	//print_mappable_buffer(GL_TRANSFORM_FEEDBACK_BUFFER, "Triangle Data", 4);
}

void
draw(bool selection = false) {
	//render_geometry();
	render_particles();
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

void
display( void )
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw();

	// SUPER TODO
	// 2nd pass
	// bind points vao
	// bind points feedback buffer
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


// initial view params
bool ortho;
vector<string> argv_copy;

// This is the most complete version.  Does not assume square aspect ratio
void myReshape2(int w, int h)
{
	glViewport(0,0,w,h);
	float ar = (float)w/(float)h;

	//printf("w = %d, h = %d, ar = %f\n", w, h, ar);

	mat4 projection;

	// starting aspect ratio is 1.0. update it
	if(ortho) {
		if( ar < 1.0) { // (w <= h ){ //taller
			//glOrtho(vl, vr, vl * (GLfloat) h / (GLfloat) w,	vr * (GLfloat) h / (GLfloat) w, 0.1,10.0);
			projection = Ortho(atof(argv_copy[2].c_str()), atof(argv_copy[3].c_str()), atof(argv_copy[2].c_str()) * (GLfloat)h / (GLfloat)w, atof(argv_copy[3].c_str()) * (GLfloat)h / (GLfloat)w, atof(argv_copy[6].c_str()), atof(argv_copy[7].c_str()));
		}
		else { //wider
			//glOrtho(vb * (GLfloat) w / (GLfloat) h, vt * (GLfloat) w / (GLfloat) h, vb, vt,0.1,10.0);
			projection = Ortho(atof(argv_copy[4].c_str()) * (GLfloat) w / (GLfloat) h, atof(argv_copy[5].c_str()) * (GLfloat)w / (GLfloat)h, atof(argv_copy[4].c_str()), atof(argv_copy[5].c_str()), atof(argv_copy[6].c_str()), atof(argv_copy[7].c_str()));
		}
	}
	else {
		projection = Perspective(atof(argv_copy[2].c_str()), ar, atof(argv_copy[3].c_str()), atof(argv_copy[4].c_str()));
	}
	
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, projection);
}

int main(int argc, char** argv)
{
	srand(1000);

	string application_info = "CS450AssignmentThree";
	string *window_title = new string;
	mat4 projection;

	string usage = "Usage:\nCS450AssignmentThree O LEFT RIGHT BOTTOM TOP NEAR FAR\nwhere LEFT RIGHT BOTTOM TOP NEAR and FAR are floating point values that specify the \
orthographic view volume.\nor\nCS450AssignmentThree P FOV NEAR FAR\nwhere FOV is the field of view in degrees, and NEAR and FAR are floating point values that specify the view volume in perspective.\n";
	bool bad_input = false;

	if(argc < 5) {
		bad_input = true;
		cerr << "Not enough arguments.\n";
	}
	else {
		bool o = false;
		bool p = false;
		o = !string("O").compare(argv[1]);
		p = !string("P").compare(argv[1]);

		if (!o && !p) {
			bad_input = true;
			cerr << "1st parameter was neither 'O' nor 'P'.\n";
		}
		else if (o && argc != 8 || p && argc != 5) {
			bad_input = true;
			cerr << "Wrong number of arguments for 'O' or 'P' viewing.\n";
		}

		for (int i = 0; i < argc; i++) {
			argv_copy.push_back(string(argv[i]));
		}

		if (o) {
			ortho = true;
			projection = Ortho(atof(argv_copy[2].c_str()), atof(argv_copy[3].c_str()), atof(argv_copy[4].c_str()), atof(argv_copy[5].c_str()), atof(argv_copy[6].c_str()), atof(argv_copy[7].c_str()));
		}
		else if (p) {
			ortho = false;
			projection = Perspective(atof(argv_copy[2].c_str()), 1.0f, atof(argv_copy[3].c_str()), atof(argv_copy[4].c_str()));
		}
	}

	if(bad_input) {
		cerr << usage;
		cin.get();
		return -1;
	}

	obj_data.push_back(new Obj("./Data/bunnyS.obj"));
	glutInit(&argc, argv);
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DEPTH_TEST);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion (3, 2);
    glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
#endif

	window_title->append(application_info);
	glutInitWindowSize(1024, 1024);
	glutInitWindowPosition(500, 300);
    glutCreateWindow(window_title->c_str());
    printf("%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    glewInit();
#endif
	gProjection = projection;
	init();

    //NOTE:  callbacks must go after window is created!!!
	glutReshapeFunc(myReshape2);
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
	glutIdleFunc(myIdle);
    glutMainLoop();

    return(0);
}
