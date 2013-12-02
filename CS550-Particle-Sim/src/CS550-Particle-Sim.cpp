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

void animate(int);

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
GLuint gNumParticles = 10;

GLint const WORLD_TRIANGLE_BUFF_IDX = 0;
GLint const POSITIONS_VELOCITIES_BUFF_IDX = 1;
GLuint gTransformBuffers[3]; // 0 = world triangle locations, 1 = positions & velocities

// Shader programs
GLuint gParticleProgram, gPassThroughProgram, gRenderProgram;
mat4 gProjection;
int gFrameCount = 0;

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

// which manipulator is being dragged
enum manip {
	NO_MANIP_HELD,
	X_HELD,
	Y_HELD,
	Z_HELD
};
manip held;
int last_x, last_y;

enum obj_mode {
	OBJ_TRANSLATE,
	OBJ_ROTATE,
	OBJ_SCALE
};
enum camera_mode {
	CAMERA_ROT_X,
	CAMERA_ROT_Y,
	CAMERA_ROT_Z,
	CAMERA_TRANSLATE,
	CAMERA_TRANSLATE_X,
	CAMERA_TRANSLATE_Y,
	CAMERA_TRANSLATE_Z,
	CAMERA_DOLLY
};
obj_mode gCurrentObjMode = OBJ_TRANSLATE;
camera_mode gCurrentCameraMode = CAMERA_TRANSLATE;

enum menu_val {
	ITEM_RESET,
	ITEM_NEW_OBJ,
	ITEM_OBJ_TRANSLATION,
	ITEM_OBJ_ROTATION,
	ITEM_SCALE,
	ITEM_CAMERA_ROT_X,
	ITEM_CAMERA_ROT_Y,
	ITEM_CAMERA_ROT_Z,
	ITEM_CAMERA_TRANSLATION,
	ITEM_CAMERA_TRANSLATION_X,
	ITEM_CAMERA_TRANSLATION_Y,
	ITEM_CAMERA_TRANSLATION_Z,
	ITEM_DOLLY
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
	for (GLuint idx = 0; idx < gNumParticles; idx++)
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
GLuint gTransformFeedback[2];
GLuint gParticleBuffer[3];
GLint gVelocityLoc;
// OpenGL initialization
void
init(void)
{
	srand(100000);

	gPassThroughProgram = InitShader("./src/vPassThrough.glsl", "./src/fPassThrough.glsl");
	gParticleProgram = InitShader("./src/vParticleSystemShader.glsl", "./src/fPassThrough.glsl");

	GLuint vPositionLoc, vColorLoc, ModelViewLoc, ProjectionLoc, triangleCountLoc, geometryTboLoc, timeStepLoc, velocityLoc;
	GLuint vao;

	glGenVertexArrays(1, &vao);
	glGenBuffers(3, gParticleBuffer);
	glGenTransformFeedbacks(2, gTransformFeedback);

	glBindVertexArray(vao);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformFeedback[0]);
	glBindBuffer(GL_ARRAY_BUFFER, gParticleBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 4 * gParticleSys.positions.size(), gParticleSys.positions.data(), GL_DYNAMIC_DRAW);
	//glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleBuffer[0]);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformFeedback[1]);
	glBindBuffer(GL_ARRAY_BUFFER, gParticleBuffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 3 * gParticleSys.velocities.size(), gParticleSys.velocities.data(), GL_DYNAMIC_DRAW);
	//glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleBuffer[1]);

	glBindBuffer(GL_ARRAY_BUFFER, gParticleBuffer[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 4 * gParticleSys.colors.size(), gParticleSys.colors.data(), GL_STATIC_DRAW);

	vPositionLoc = glGetAttribLocation(gPassThroughProgram, "vPosition");
	vColorLoc = glGetAttribLocation(gPassThroughProgram, "vColor");
	ModelViewLoc = glGetAttribLocation(gPassThroughProgram, "ModelView");
	ProjectionLoc = glGetAttribLocation(gPassThroughProgram, "Projection");

	glEnableVertexAttribArray(vPositionLoc);
	glVertexAttribPointer(vPositionLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	glEnableVertexAttribArray(vColorLoc);
	glVertexAttribPointer(vPositionLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

	point4  eye(0., 0., 1., 1.);
	point4  at(0., 0., 0., 1.);
	vec4    up(0., 1., 0., 0.);
	gCameraTranslate = Translate(-eye);
	gViewTransform = LookAt(eye, at, up);
	gModelView = gViewTransform * Angel::identity();

	glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);

    glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	//glutTimerFunc(2000, animate, 0);
}

void update_particles(void)
{
	// do the stuff where the particles move yo!
	mount_shader(gParticleProgram);
	gVelocityLoc = glGetAttribLocation(gParticleProgram, "velocity");
	gTriangleCountLoc = glGetAttribLocation(gParticleProgram, "triangle_count");

	static const char *varyings[] =
	{
		"position_out", "velocity_out"
	};
	glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_SEPARATE_ATTRIBS);
	glLinkProgram(gParticleProgram);

	glBindVertexArray(gParticleSys.vao[1]);

	glBeginTransformFeedback(GL_POINTS);
	if ((gFrameCount & 1) != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.positions_vbo);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformBuffers[0]);
		glDrawTransformFeedback(GL_POINTS, gTransformBuffers[1]);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.velocities_vbo);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformBuffers[1]);
		glDrawTransformFeedback(GL_POINTS, gTransformBuffers[0]);
	}
	glEndTransformFeedback();

	gFrameCount++;
}

void render_particles(void)
{
	glPointSize(10.);
	glUseProgram(gPassThroughProgram);
	glDrawArrays(GL_POINTS, 0, gParticleSys.positions.size());
}

void
draw(bool selection = false) {
	render_particles();
	/*update_particles();
	update_particles();*/
}

//----------------------------------------------------------------------------

void
mouse( int button, int state, int x, int y )
{
	held = NO_MANIP_HELD;
	if(button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) {
		return;
	}

	last_x = x;
	last_y = y;

	// ------------------------------- redrawing for selection

	draw(true);

	// -------------------------------- end redrawing


	glutPostRedisplay();  //MUST REMEMBER TO CALL POST REDISPLAY OR IT WON'T RENDER!

	//Now check the pixel location to see what color is found!
	GLubyte pixel[4];
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	//Read as unsigned byte.
	glReadPixels(x, viewport[3] - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	gPicked = -1;

	// check first if a manipulator was selected. color of (255,0,0) or (0,255,0) or (0,0,255)
	if(ceil(pixel[0]) == 255 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 0) {
		held = X_HELD;
		return;
	}
	else if(ceil(pixel[0]) == 0 && ceil(pixel[1]) == 255 && ceil(pixel[2]) == 0) {
		held = Y_HELD;
		return;
	}
	else if(ceil(pixel[0]) == 0 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 255) {
		held = Z_HELD;
		return;
	}


	for(GLuint i=0; i < obj_data.size(); i++) {
		if(obj_data[i]->selectionR == ceil(pixel[0]) && obj_data[i]->selectionG == pixel[1]
			&& obj_data[i]->selectionB == pixel[2] /*&& obj_data[i]->selectionA == pixel[3]*/) {
			gPicked = i;
			obj_data[i]->selected = true;
		}
		else {
			gPicked = -1;
			obj_data[i]->selected = false;
		}
	}

	//printf("Picked  == %d\n", gPicked);
	// uncomment below to see the color render
	// Swap buffers makes the back buffer actually show...in this case, we don't want it to show so we comment out.
	// For debugging, you can uncomment it to see the render of the back buffer which will hold your 'fake color render'

	/*glutSwapBuffers();
	cin.get();*/
}

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
	glutMouseFunc(mouse);
    glutDisplayFunc(display);
    glutMainLoop();

    return(0);
}
