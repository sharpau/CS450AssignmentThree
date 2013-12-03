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

GLint gVertLoc, gVelocityLoc, gNormLoc, gColorLoc, gTriangleCountLoc;

// camera transforms
mat4 gCameraTranslate, gCameraRotX, gCameraRotY, gCameraRotZ;

// particles stuff
GLuint gNumParticles = 10;

// Shader programs
GLuint gParticleProgram, gPassThroughProgram, gRenderProgram;
mat4 gProjection;

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

struct Particles
{
	// interleaved position and velocity data
	vector<vec4> pos_vel_data;
	vector<vec4> colors;

	GLuint colors_vbo;
	GLuint normals_vbo;
	GLuint double_buffer_vbo[2];

	GLuint currVB;
	GLuint currTFB;

	GLuint vao;
	GLuint transformFeedbackObject[2];
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

// does all vao/vbo setup for obj_data[i]
void
setup_obj(int i) {
	// Create a vertex array object
	glGenVertexArrays(1, &obj_data[i]->vao);
	glBindVertexArray(obj_data[i]->vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);

	// set up colors for selection
	obj_data[i]->selectionR = i;
	obj_data[i]->selectionG = i;
	obj_data[i]->selectionB = i;
	obj_data[i]->selectionA = 255; // only seems to work at 255
	obj_data[i]->model_view = gViewTransform;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLsizei num_bytes_vert_data = sizeof(GLfloat)* obj_data[i]->data_soa.positions.size();
	GLsizei num_bytes_norm_data = sizeof(GLfloat)* obj_data[i]->data_soa.normals.size();
	GLvoid *vert_data = obj_data[i]->data_soa.positions.data();
	GLvoid *norm_data = obj_data[i]->data_soa.normals.data();
	glBufferData(GL_ARRAY_BUFFER, num_bytes_vert_data + num_bytes_vert_data, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_vert_data, vert_data);
	glBufferSubData(GL_ARRAY_BUFFER, num_bytes_vert_data, num_bytes_norm_data, norm_data);


	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, obj_data[i]->data_soa.positions_stride, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);


	gNormLoc = glGetAttribLocation(gRenderProgram, "vNormal");
	glEnableVertexAttribArray(gNormLoc);
	glVertexAttribPointer(gNormLoc, obj_data[i]->data_soa.normals_stride, GL_FLOAT, GL_FALSE, 0, (GLvoid *)num_bytes_vert_data);
}

void
init_grid(void) {
	// load in vertices for lines
	grid.data_soa.positions_stride = 3;
	for (int i = -10; i < 10; i++) {
		// line on x = i
		grid.data_soa.positions.push_back(i);
		grid.data_soa.positions.push_back(0);
		grid.data_soa.positions.push_back(-10);

		grid.data_soa.positions.push_back(i);
		grid.data_soa.positions.push_back(0);
		grid.data_soa.positions.push_back(10);

		// line on y = i
		grid.data_soa.positions.push_back(-10);
		grid.data_soa.positions.push_back(0);
		grid.data_soa.positions.push_back(i);

		grid.data_soa.positions.push_back(10);
		grid.data_soa.positions.push_back(0);
		grid.data_soa.positions.push_back(i);
	}
	grid.data_soa.num_vertices = grid.data_soa.positions.size() / grid.data_soa.positions_stride;

	// setup vao and two vbos for manipulators
	glGenVertexArrays(1, &grid.vao);
	glBindVertexArray(grid.vao);
	GLuint grid_buffer[2]; // 0 is vertices, 1 is colors
	glGenBuffers(2, grid_buffer);

	grid.data_soa.colors_stride = 4;
	for (int j = 0; j < grid.data_soa.num_vertices; j++) {
		grid.data_soa.colors.push_back(0.0);
		grid.data_soa.colors.push_back(0.0);
		grid.data_soa.colors.push_back(0.0);
		grid.data_soa.colors.push_back(1.0);
	}


	// vertices
	glBindBuffer(GL_ARRAY_BUFFER, grid_buffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* grid.data_soa.positions.size(), grid.data_soa.positions.data(), GL_STATIC_DRAW);
	// colors
	glBindBuffer(GL_ARRAY_BUFFER, grid_buffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* grid.data_soa.colors.size(), grid.data_soa.colors.data(), GL_STATIC_DRAW);

	// color added to shader. only displays with flag == 2
	glBindBuffer(GL_ARRAY_BUFFER, grid_buffer[0]);
	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, grid.data_soa.positions_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, grid_buffer[1]);
	glEnableVertexAttribArray(gColorLoc);
	glVertexAttribPointer(gColorLoc, grid.data_soa.colors_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
}

void
init_manips(void) {
	for (int i = 0; i < 3; i++) {
		manips[i] = Obj(DATA_DIRECTORY_PATH + "axis.obj");


		// setup vao and two vbos for manipulators
		glGenVertexArrays(1, &manips[i].vao);
		glBindVertexArray(manips[i].vao);
		GLuint manips_buffer[2]; // 0 is vertices, 1 is colors
		glGenBuffers(2, manips_buffer);

		manips[i].data_soa.colors_stride = 4;
		for (int j = 0; j < manips[i].data_soa.num_vertices; j++) {
			manips[i].data_soa.colors.push_back(i == 0 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(i == 1 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(i == 2 ? 1.0 : 0.0);
			manips[i].data_soa.colors.push_back(1.0);
		}


		// vertices
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* manips[i].data_soa.positions.size(), manips[i].data_soa.positions.data(), GL_STATIC_DRAW);
		// colors
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* manips[i].data_soa.colors.size(), manips[i].data_soa.colors.data(), GL_STATIC_DRAW);

		// color added to shader. only displays with flag == 2
		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[0]);
		glEnableVertexAttribArray(gVertLoc);
		glVertexAttribPointer(gVertLoc, manips[i].data_soa.positions_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, manips_buffer[1]);
		glEnableVertexAttribArray(gColorLoc);
		glVertexAttribPointer(gColorLoc, manips[i].data_soa.colors_stride, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	}

	// all manips start pointing +y
	// idx 0 needs to point +x
	// rotate about z +90
	manips[0].rotateZ = Angel::RotateZ(-90.0f);

	// idx 2 needs to point +z
	// rotate about x
	manips[2].rotateX = Angel::RotateX(90.0f);

}

void init_objects() {

	gRenderProgram = InitShader("./src/vshader.glsl", "./src/fshader.glsl");
	mount_shader(gRenderProgram);
	gSelectFlagLoc = glGetUniformLocation(gRenderProgram, "flag");

	// build the special objects not loaded by user
	init_grid();
	init_manips();

	for (int i = 0; i < obj_data.size(); i++)
	{
		setup_obj(i);
	}

	int linked;
	glGetProgramiv(gRenderProgram, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE)
	{
		int maxLength;
		glGetProgramiv(gRenderProgram, GL_INFO_LOG_LENGTH, &maxLength);
		maxLength = maxLength + 1;
		GLchar *pLinkInfoLog = new GLchar[maxLength];
		glGetProgramInfoLog(gRenderProgram, maxLength, &maxLength, pLinkInfoLog);
		cerr << *pLinkInfoLog << endl;
	}


	gSelectColorRLoc = glGetUniformLocation(gRenderProgram, "selectionColorR");
	gSelectColorGLoc = glGetUniformLocation(gRenderProgram, "selectionColorG");
	gSelectColorBLoc = glGetUniformLocation(gRenderProgram, "selectionColorB");
	gSelectColorALoc = glGetUniformLocation(gRenderProgram, "selectionColorA");
	glUniform1i(gSelectColorRLoc, gSelectionColorR);
	glUniform1i(gSelectColorGLoc, gSelectionColorG);
	glUniform1i(gSelectColorBLoc, gSelectionColorB);
	glUniform1i(gSelectColorALoc, gSelectionColorA);

	glUniform1i(gSelectFlagLoc, gFlag);
}

void init_particles()
{
	gParticleProgram = InitShader("./src/vParticleSystemShader.glsl", "./src/fPassThrough.glsl");
	mount_shader(gParticleProgram);
	gVelocityLoc = glGetAttribLocation(gParticleProgram, "vVelocity");
	gTriangleCountLoc = glGetUniformLocation(gParticleProgram, "triangle_count");


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

		// position
		gParticleSys.pos_vel_data.push_back(vec4(frand(-1, 1.), frand(-1, 1.), frand(-1, 1.), 1.));
		// velocity
		gParticleSys.pos_vel_data.push_back(vec4(frand(-.1, .1), frand(-.1, .1), frand(-.1, .1), 0.));
	}


	glGenVertexArrays(1, &gParticleSys.vao);
	glBindVertexArray(gParticleSys.vao);

	glGenTransformFeedbacks(2, gParticleSys.transformFeedbackObject);
	glGenBuffers(1, &gParticleSys.colors_vbo);
	glGenBuffers(2, gParticleSys.double_buffer_vbo); // these serve as vbos and tfbs

	for (int i = 0; i < 2; i++) {
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gParticleSys.transformFeedbackObject[i]);
		glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.double_buffer_vbo[i]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gParticleSys.double_buffer_vbo[i]);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.pos_vel_data.size()), gParticleSys.pos_vel_data.data(), GL_DYNAMIC_DRAW);
	}
	gParticleSys.currVB = 0;
	gParticleSys.currTFB = 1;

	glEnableVertexAttribArray(gVertLoc);
	glVertexAttribPointer(gVertLoc, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), BUFFER_OFFSET(0));

	glEnableVertexAttribArray(gVelocityLoc);
	glVertexAttribPointer(gVelocityLoc, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), BUFFER_OFFSET(4 * sizeof(GLfloat)));



	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.colors_vbo);
	glBufferData(GL_ARRAY_BUFFER, (sizeof(GLfloat)* 4 * gParticleSys.colors.size()), gParticleSys.colors.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(gColorLoc);
	glVertexAttribPointer(gColorLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
}


// OpenGL initialization
void
init(void)
{
    gCameraRotX = Angel::identity();
	gCameraRotY = Angel::identity();
	gCameraRotZ = Angel::identity();
	gCameraTranslate = Angel::identity();


	init_particles();
	init_objects();



    gModelViewLoc = glGetUniformLocation(gParticleProgram, "ModelView");
	gProjectionLoc = glGetUniformLocation(gParticleProgram, "Projection");

    point4  eye(0., 0., 2.5, 1.);
    point4  at(0., 0., 0., 1.);
    vec4    up(0., 1., 0., 0.);
	gCameraTranslate = Translate(-eye);
    gViewTransform = LookAt(eye, at, up);
	gModelView = gViewTransform * Angel::identity();

    glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
    glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);

	manips[0].model_view = gViewTransform;
	manips[1].model_view = gViewTransform;
	manips[2].model_view = gViewTransform;
	for (auto obj : obj_data) {
		obj->model_view = gViewTransform;
	}
	gModelViewLoc = glGetUniformLocation(gRenderProgram, "ModelView");
	gProjectionLoc = glGetUniformLocation(gRenderProgram, "Projection");
	glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
	glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);


    glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

void update_particles(void)
{
	static const char *varyings[] =
	{
		"position_out", "velocity_out"
	};
	glTransformFeedbackVaryings(gParticleProgram, 2, varyings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(gParticleProgram);
	int linked;
	glGetProgramiv(gParticleProgram, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		int maxLength;
		glGetProgramiv(gParticleProgram, GL_INFO_LOG_LENGTH, &maxLength);
		maxLength = maxLength + 1;
		GLchar *pLinkInfoLog = new GLchar[maxLength];
		glGetProgramInfoLog(gParticleProgram, maxLength, &maxLength, pLinkInfoLog);
		cerr << *pLinkInfoLog << endl;
	}

	glUniform1i(gTriangleCountLoc, 0);


	printf("positions and velocities\n");
	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.double_buffer_vbo[gParticleSys.currVB]);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gParticleSys.transformFeedbackObject[gParticleSys.currTFB]);

	glBeginTransformFeedback(GL_POINTS);

	glDrawArrays(GL_POINTS, 0, gParticleSys.pos_vel_data.size() / 2);
	glEndTransformFeedback();


	bool t = glIsTransformFeedback(gParticleSys.transformFeedbackObject[0]);
	int stride = 4;
	GLfloat * data;
	data = (GLfloat *)glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	for (int i = 0; i < gParticleSys.pos_vel_data.size(); i++)
	{
		int idx = stride * i;
		printf("%i: %f, %f, %f, %f\n", i, data[idx], data[idx + 1], data[idx + 2], data[idx + 3]);
	}
	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
}

void myIdle(void)
{
	mount_shader(gParticleProgram);

	update_particles();
	// swap VB and TFB
	gParticleSys.currVB = gParticleSys.currTFB;
	gParticleSys.currTFB = (gParticleSys.currTFB + 1) & 0x1;
	glutPostRedisplay();
}

void render_particles(void)
{
	glBindVertexArray(gParticleSys.vao);
	glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, gViewTransform);
	glUniformMatrix4fv(gProjectionLoc, 1, GL_TRUE, gProjection);
	glPointSize(10.);
	glBindBuffer(GL_ARRAY_BUFFER, gParticleSys.double_buffer_vbo[gParticleSys.currTFB]);
	glDrawArrays(GL_POINTS, 0, gParticleSys.pos_vel_data.size() / 2);
}

void
draw_objects(bool selection = false) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!selection) {
		mat4 rot = grid.rotateX * grid.rotateY * grid.rotateZ;
		gViewTransform = gCameraTranslate * (gCameraRotX * gCameraRotY * gCameraRotZ);
		grid.model_view = gViewTransform * (grid.translateXYZ * (grid.scaleXYZ * rot));
		glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, grid.model_view);
		// draw ground grid
		glBindVertexArray(grid.vao);
		glDrawArrays(GL_LINES, 0, grid.data_soa.num_vertices);
	}

	// draw user objects and manips
	for (auto obj : obj_data)
	{
		//Normal render so set selection Flag to 0
		gFlag = selection ? 1 : 0;
		glUniform1i(gSelectFlagLoc, gFlag);

		if (obj->selected == true) {
			// draw manipulators here
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			gFlag = 2;	// change flag to 2, for absolute coloring
			glUniform1i(gSelectFlagLoc, gFlag);
			for (int i = 0; i < 3; i++) {
				// manips: should have 1 rotation ever. also always use their object's translate.
				// and should use the gModelView as everything else does
				mat4 rot_manip = manips[i].rotateX * manips[i].rotateY * manips[i].rotateZ;
				mat4 transform = gViewTransform * obj->translateXYZ * rot_manip; // accumulation shenanigans
				glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, transform);


				glBindVertexArray(manips[i].vao);
				glDrawArrays(GL_TRIANGLES, 0, manips[i].data_soa.num_vertices);
			}

			// back to normal rendering
			gFlag = selection ? 1 : 0;
			glUniform1i(gSelectFlagLoc, gFlag);

			// wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glPolygonOffset(1.0, 2); //Try 1.0 and 2 for factor and units

		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		if (selection) {
			gSelectionColorR = obj->selectionR;
			gSelectionColorG = obj->selectionG;
			gSelectionColorB = obj->selectionB;
			gSelectionColorA = obj->selectionA;

			//sync with shader
			glUniform1i(gSelectColorRLoc, gSelectionColorR);
			glUniform1i(gSelectColorGLoc, gSelectionColorG);
			glUniform1i(gSelectColorBLoc, gSelectionColorB);
			glUniform1i(gSelectColorALoc, gSelectionColorA);
		}


		mat4 rot = obj->rotateX * obj->rotateY * obj->rotateZ;
		obj->model_view = gViewTransform * (obj->translateXYZ * (obj->scaleXYZ * rot));
		glUniformMatrix4fv(gModelViewLoc, 1, GL_TRUE, obj->model_view);

		glBindVertexArray(obj->vao);
		glDrawArrays(GL_TRIANGLES, 0, obj->data_soa.num_vertices);
	}
}


//----------------------------------------------------------------------------
void
mouse(int button, int state, int x, int y)
{
	held = NO_MANIP_HELD;
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) {
		return;
	}

	last_x = x;
	last_y = y;

	// ------------------------------- redrawing for selection

	draw_objects(true);

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
	if (ceil(pixel[0]) == 255 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 0) {
		held = X_HELD;
		return;
	}
	else if (ceil(pixel[0]) == 0 && ceil(pixel[1]) == 255 && ceil(pixel[2]) == 0) {
		held = Y_HELD;
		return;
	}
	else if (ceil(pixel[0]) == 0 && ceil(pixel[1]) == 0 && ceil(pixel[2]) == 255) {
		held = Z_HELD;
		return;
	}


	for (int i = 0; i < obj_data.size(); i++) {
		if (obj_data[i]->selectionR == ceil(pixel[0]) && obj_data[i]->selectionG == pixel[1]
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

	mount_shader(gRenderProgram);
	draw_objects();
	mount_shader(gParticleProgram);
	render_particles();

	glutSwapBuffers();
}

//----------------------------------------------------------------------------
vec4 camera_vec;
void
motion(int x, int y) {
	int delta_x = last_x - x;
	int delta_y = last_y - y;
	last_x = x;
	last_y = y;

	GLfloat delta = .01;
	GLfloat dx, dy, dz, dtheta, dscalex, dscaley, dscalez;
	GLfloat scale_down = .99;
	GLfloat scale_up = 1.01;
	dx = dy = dz = .0;
	dscalex = dscaley = dscalez = 1.;
	dtheta = 0.;

	mat4 translate_xyz = Angel::identity();
	mat4 rotate_x = Angel::identity();
	mat4 rotate_y = Angel::identity();
	mat4 rotate_z = Angel::identity();
	mat4 scale_xyz = Angel::identity();
	GLfloat v;
	vec4 view_plane;
	vec4 view_plane_perp;
	vec4 mouse_drag(delta_x, delta_y, 0., 0.);
	vec4 world_drag = normalize(gViewTransform * mouse_drag);
	printf("world drag: (%f, %f, %f)\n", world_drag.x, world_drag.y, world_drag.z);
	switch (held) {
	case NO_MANIP_HELD:
		printf("scene dragged %d px in x, %d px in y\n", delta_x, delta_y);
		switch (gCurrentCameraMode) {
		case CAMERA_ROT_X:
			//rotate_x = RotateX(gCameraThetaX);
			if (world_drag.x < 0)
			{
				dtheta = -1.;
			}
			else if (world_drag.x > 0) {
				dtheta = 1.;
			}
			rotate_x = RotateX(dtheta);
			gCameraRotX *= rotate_x;
			break;
		case CAMERA_ROT_Y:
			if (world_drag.x < 0)
			{
				dtheta = -1.;
			}
			else if (world_drag.x > 0) {
				dtheta = 1.;
			}
			rotate_y = RotateY(dtheta);
			gCameraRotY *= rotate_y;
			//rotate_y = RotateY(gCameraThetaY);
			break;
		case CAMERA_ROT_Z:
			if (world_drag.x < 0)
			{
				dtheta = -1.;
			}
			else if (world_drag.x > 0) {
				dtheta = 1.;
			}
			rotate_z = RotateZ(dtheta);
			gCameraRotZ *= rotate_z;
			//rotate_z = RotateZ(gCameraThetaZ);
			break;
		case CAMERA_TRANSLATE:
			// figure out our view plane - perpendicular to viewtransform * vec4(eye - at)
			//
			if (world_drag.x < 0)
			{
				delta = -1. * delta;
			}
			camera_vec = gViewTransform * vec4(0., 0., 0., 1.);
			camera_vec.w = 0.;
			view_plane_perp = normalize(cross(camera_vec, vec4(0., 1., 0., 0.)));
			view_plane_perp.w = 0.;
			dx = delta * dot(view_plane_perp, vec4(1., 0., 0., 0.));
			dy = delta * dot(view_plane_perp, vec4(0., 1., 0., 0.));
			dz = delta * dot(view_plane_perp, vec4(0., 0., 1., 0.));
			translate_xyz = Translate(dx, dy, dz); // inverse
			gCameraTranslate *= translate_xyz; // needs all the rotates
			break;
		case CAMERA_DOLLY:
			// take the world_Transform component perpendicular to the viewing plane, move the camera in/out that much

			if (world_drag.x < 0)
			{
				delta = -1. * delta;
			}
			camera_vec = gViewTransform * vec4(0., 0., 0., 1.);
			camera_vec.w = 0.;
			view_plane = normalize(camera_vec);
			dx = delta * dot(view_plane, vec4(1., 0., 0., 0.));
			dy = delta * dot(view_plane, vec4(0., 1., 0., 0.));
			dz = delta * dot(view_plane, vec4(0., 0., 1., 0.));
			translate_xyz = Translate(dx, dy, dz); // inverse

			gCameraTranslate *= translate_xyz;
			break;
		}
		break; // BREAK NO_MANIP_HELD
	}
	for (auto obj : obj_data) {
		if (obj->selected) {
			switch (held) {
			case X_HELD:
				printf("manipulator %d dragged %d px in x, %d px in y\n", held, delta_x, delta_y);
				// TODO: NEED TO ACCOMODATE gViewTransform for all cases
				switch (gCurrentObjMode) {
				case OBJ_TRANSLATE:
					// take the world_transform component parallel to x/y/z, move object that far
					if (delta_x < 0)
					{
						dx = delta;
					}
					else if (delta_x > 0) {
						dx = -1. * delta;
					}
					break;
				case OBJ_ROTATE:
					if (delta_x < 0)
					{
						dtheta = 1.;
					}
					else if (delta_x > 0) {
						dtheta = -1.;
					}
					obj->thetaX += dtheta;
					rotate_x = RotateX(obj->thetaX);
					obj->rotateX = rotate_x;
					break;
					// take the world_transform component perpendicular to x/y/z, turn that into degrees-to-rotate
				case OBJ_SCALE:

					if (delta_x < 0)
					{
						dscalex = scale_up;
					}
					else if (delta_x > 0) {
						dscalex = scale_down;
					}
					break;

					break; // BREAK gCurrentObjMode
				}
				break; // BREAK X_HELD

			case Y_HELD:
				// TODO: NEED TO ACCOMODATE gViewTransform for all cases
				switch (gCurrentObjMode) {
				case OBJ_TRANSLATE:
					// take the world_transform component parallel to y/y/z, move object that far

					if (delta_y < 0)
					{
						dy = -1. * delta;
					}
					else if (delta_y > 0) {
						dy = delta;
					}
					break;
				case OBJ_ROTATE:

					if (delta_y < 0)
					{
						dtheta = 1.;
					}
					else if (delta_y > 0) {
						dtheta = -1.;
					}
					obj->thetaY += dtheta;
					rotate_y = RotateY(obj->thetaY);
					obj->rotateY = rotate_y;
					break;
				case OBJ_SCALE:

					if (delta_y < 0)
					{
						dscaley = scale_down;
					}
					else if (delta_y > 0) {
						dscaley = scale_up;
					}
					break;

					break; // BREAK gCurrentObjMode
				}
				break; // BREAK Y_HELD

			case Z_HELD:
				// TODO: NEED TO ACCOMODATE gViewTransform for all cases
				switch (gCurrentObjMode) {
				case OBJ_TRANSLATE:
					// take the world_transform component parallel to z/y/z, move object that far

					if (delta_x < 0)
					{
						dz = -1. * delta;
					}
					else if (delta_x > 0) {
						dz = delta;
					}
					break;
				case OBJ_ROTATE:
					if (delta_x < 0)
					{
						dtheta = 1.;
					}
					else if (delta_x > 0) {
						dtheta = -1.;
					}
					obj->thetaZ += dtheta;
					rotate_z = RotateZ(obj->thetaZ);
					obj->rotateZ = rotate_z;
					break;
				case OBJ_SCALE:

					if (delta_x < 0)
					{
						dscalez = scale_down;
					}
					else if (delta_x > 0) {
						dscalez = scale_up;
					}
					break;

					break; // BREAK gCurrentObjMode
				}
				break; // BREAK Z_HELD
			}
			translate_xyz = Translate(obj->translateXYZ[0][3] + dx, obj->translateXYZ[1][3] + dy, obj->translateXYZ[2][3] + dz);
			scale_xyz = Scale(obj->scaleXYZ[0][0] * dscalex, obj->scaleXYZ[1][1] * dscaley, obj->scaleXYZ[2][2] * dscalez);

			obj->translateXYZ = translate_xyz;
			obj->scaleXYZ = scale_xyz;
		}
	}
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

	string application_info = "CS550-Particle-Sim";
	string *window_title = new string;
	mat4 projection;

	string usage = "Usage:\nCS550-Particle-Sim O LEFT RIGHT BOTTOM TOP NEAR FAR\nwhere LEFT RIGHT BOTTOM TOP NEAR and FAR are floating point values that specify the \
orthographic view volume.\nor\nCS550-Particle-Sim P FOV NEAR FAR\nwhere FOV is the field of view in degrees, and NEAR and FAR are floating point values that specify the view volume in perspective.\n";
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
	glutMotionFunc(motion);
    glutDisplayFunc(display);
	//glutIdleFunc(myIdle);
    glutMainLoop();

    return(0);
}
