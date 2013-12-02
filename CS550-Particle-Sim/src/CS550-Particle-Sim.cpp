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
GLuint gNumParticles = 1000;
GLuint gTransformFeedback;

GLint const WORLD_TRIANGLE_BUFF_IDX = 0;
GLint const POSITIONS_VELOCITIES_BUFF_IDX = 1;
GLuint gTransformBuffers[3]; // 0 = world triangle locations, 1 = positions & velocities

// Shader programs
GLuint gParticleProgram, gPassThroughProgram, gRenderProgram;
mat4 gProjection;
int gFrameCount = 0;

GLint gVelocityLoc;
struct Particles
{
	vector<vec4> positions;
	vector<vec4> colors;
	vector<vec3> velocities;

	GLuint positions_vbo;
	GLuint colors_vbo;
	GLuint velocities_vbo;
	GLuint normals_vbo;
	
	GLuint vao[2];
} gParticleSys;


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
	gNormLoc = glGetAttribLocation(shader_program, "vNormal");
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
		GLfloat x = frand(-1, 1.);
		GLfloat y = frand(-1, 1.);
		GLfloat z = frand(-1, 1.);
		GLfloat hsv[3];
		hsv[0] = hue0 + dHue * idx; // frand(90., 275.);
		hsv[1] = 1.;
		hsv[2] = 1.;
		GLfloat rgb[3] = { 0., 0., 0. };
		HSVtoRGB(hsv, rgb);
		GLfloat vX0 = (.1 * static_cast<GLfloat>(rand()) / static_cast <float> (RAND_MAX))+1.;
		GLfloat vY0 = (.1 * static_cast<GLfloat>(rand()) / static_cast <float> (RAND_MAX)) + 1.;
		GLfloat vZ0 = (.1 * static_cast<GLfloat>(rand()) / static_cast <float> (RAND_MAX)) + 1.;
		GLfloat nX = 0.;
		GLfloat nY = 0.;
		GLfloat nZ = 1.;
		gParticleSys.positions.push_back(vec4(x, y, z, 1.));
		gParticleSys.colors.push_back(vec4(rgb[0], rgb[1], rgb[2], 1.));
		gParticleSys.velocities.push_back(vec3(vX0, vY0, vZ0));
	}
}


// OpenGL initialization
void
init(void)
{
    gCameraRotX = Angel::identity();
	gCameraRotY = Angel::identity();
	gCameraRotZ = Angel::identity();
	gCameraTranslate = Angel::identity();
	

	// Load shaders and use the resulting shader program
	// doing this ahead of time so we can use it for setup of special objects
    gPassThroughProgram = InitShader( "./src/vshader.glsl", "./src/fPassThrough.glsl" );
	gPassThroughProgram = InitShader("./src/vPassThrough.glsl", "./src/fPassThrough.glsl");
	gParticleProgram = InitShader("./src/vParticleSystemShader.glsl", "./src/fPassThrough.glsl");


	mount_shader(gPassThroughProgram);
	gVelocityLoc = glGetAttribLocation(gParticleProgram, "velocity");
	gTriangleCountLoc = glGetAttribLocation(gParticleProgram, "triangle_count");

	// build the special objects not loaded by user
	/*init_grid();
	init_manips();*/
	init_particles();

	glGenBuffers(1, &gParticleSys.positions_vbo);
	glGenBuffers(1, &gParticleSys.colors_vbo);
	glGenBuffers(1, &gParticleSys.velocities_vbo);

	glGenVertexArrays(2, gParticleSys.vao);

	glBindVertexArray(gParticleSys.vao[0]);
	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.positions_vbo);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.positions.size()), gParticleSys.positions.data(), GL_DYNAMIC_COPY);
	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.colors_vbo);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.colors.size()), gParticleSys.colors.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(gColorLoc);
	glVertexAttribPointer(gColorLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	

	glBindVertexArray(gParticleSys.vao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.velocities_vbo);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 3 * gParticleSys.velocities.size()), gParticleSys.velocities.data(), GL_DYNAMIC_COPY);
	glEnableVertexAttribArray(gVelocityLoc);
	glVertexAttribPointer(gVelocityLoc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glGenTransformFeedbacks(3, gTransformBuffers);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformBuffers[0]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, gTransformBuffers[0], gParticleSys.velocities_vbo);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, gTransformBuffers[0], gParticleSys.positions_vbo);

	int linked;
	glGetProgramiv(gPassThroughProgram, GL_LINK_STATUS, &linked);
	if(linked != GL_TRUE) {
		int maxLength;
		glGetProgramiv(gPassThroughProgram, GL_INFO_LOG_LENGTH, &maxLength);
		maxLength = maxLength + 1;
		GLchar *pLinkInfoLog = new GLchar[maxLength];
		glGetProgramInfoLog(gPassThroughProgram, maxLength, &maxLength, pLinkInfoLog);
		cerr << *pLinkInfoLog << endl;
	}

	
	//Set up selection colors and a gFlag -- copied from example
	gSelectColorRLoc = glGetUniformLocation(gPassThroughProgram,"selectionColorR");
	gSelectColorGLoc = glGetUniformLocation(gPassThroughProgram,"selectionColorG");
	gSelectColorBLoc = glGetUniformLocation(gPassThroughProgram,"selectionColorB");
	gSelectColorALoc = glGetUniformLocation(gPassThroughProgram,"selectionColorA");
	glUniform1i(gSelectColorRLoc, gSelectionColorR);
	glUniform1i(gSelectColorGLoc, gSelectionColorG);
	glUniform1i(gSelectColorBLoc, gSelectionColorB);
	glUniform1i(gSelectColorALoc, gSelectionColorA);

	gSelectFlagLoc = glGetUniformLocation(gPassThroughProgram, "flag");
	glUniform1i(gSelectFlagLoc, gFlag);


    gModelViewLoc = glGetUniformLocation(gPassThroughProgram, "ModelView");
    gProjectionLoc = glGetUniformLocation(gPassThroughProgram, "Projection");

    point4  eye(0., 0., 2.5, 1.);
    point4  at(0., 0., 0., 1.);
    vec4    up(0., 1., 0., 0.);
	gCameraTranslate = Translate(-eye);
    gViewTransform = LookAt(eye, at, up);
	gModelView = gViewTransform * Angel::identity();
	manips[0].model_view = gViewTransform;
	manips[1].model_view = gViewTransform;
	manips[2].model_view = gViewTransform;
	for(auto obj : obj_data) {
		obj->model_view = gViewTransform;
	}

    glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);


    glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	//glutTimerFunc(2000, animate, 0);
}

void update_particles(void)
{
	// do the stuff where the particles move yo!
	glUseProgram(gParticleProgram);
	gVelocityLoc = glGetAttribLocation(gParticleProgram, "velocity");
	gTriangleCountLoc = glGetAttribLocation(gParticleProgram, "triangle_count");

	glUniform1i(gTriangleCountLoc, 0);
	static const char *varyings[] =
	{
		"position_out", "velocity_out"
	};
	glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_SEPARATE_ATTRIBS);
	glLinkProgram(gParticleProgram);

	if ((gFrameCount & 1) != 0)
	{
		glBindVertexArray(gParticleSys.vao[1]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.positions_vbo);
	}
	else
	{
		glBindVertexArray(gParticleSys.vao[0]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.velocities_vbo);
	}
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, gParticleSys.positions.size());
	glEndTransformFeedback();
	gFrameCount++;
}

void render_particles(void)
{
	glPointSize(10.);
	glUseProgram(gPassThroughProgram);
	glBindVertexArray(gParticleSys.vao[0]);
	glDrawArrays(GL_POINTS, 0, gParticleSys.positions.size());
}

void
draw(bool selection = false) {
	render_particles();
	update_particles();
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
	srand(time(0));

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
    glutMainLoop();

    return(0);
}
