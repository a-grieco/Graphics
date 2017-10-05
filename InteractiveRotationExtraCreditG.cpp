// InteractiveRotationExtraCreditG.cpp

#include <stdio.h>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include "GLSL.h"
#include <vector> 
#include <time.h>
#include "mat.h"

// Application Data

GLuint vBuffer = 0;
GLuint program = 0;

// Mouse Interaction
vec2 mouseDown;			// location of last mouse down
vec2 rotOld, rotNew;	// .x is rotation about Y-axis, .y about X-axis
float speed = 0.3;

// drawing a strange looking G... not sure what happened here.
// 40 2D vertex location for the letter 'G'
float points[][3] = {
	// front
	{ .75, 0, .5 },{ .5, 0, .5 },{ .75, .5, .5 },{ .5, .75, .5 },{ .25, .25, .5 },
	{ -.5, .75, .5 },{ -.25, .25, .5 },{ -.75, .5, .5 },{ -.25, -.25, .5 },
	{ -.75, -.5, .5 },{ -.5, -.75, .5 },{ 0, -.25, .5 },{ .5, -.75, .5 },
	{ .75, -.5, .5 },{ .75, -.125, .5 },{ .375, -.125, .5 },{ .125, .125, .5 },
	{ 0, -.125, .5 },{ -.125, .125, .5 },{ -.125, -.125, .5 },
	// back
	{ .75, 0, -.5 },{ .5, 0, -.5 },{ .75, .5, -.5 },{ .5, .75, -.5 },{ .25, .25, -.5 },
	{ -.5, .75, -.5 },{ -.25, .25, -.5 },{ -.75, .5, -.5 },{ -.25, -.25, -.5 },
	{ -.75, -.5, -.5 },{ -.5, -.75, -.5 },{ 0, -.25, -.5 },{ .5, -.75, -.5 },
	{ .75, -.5, -.5 },{ .75, -.125, -.5 },{ .375, -.125, -.5 },{ .125, .125, -.5 },
	{ 0, -.125, -.5 },{ -.125, .125, -.5 },{ -.125, -.125, -.5 } };

// 40 colors
float colors[][3] = {
	// front
	{ 0, .2, .8 },{ 0, .6, .8 },{ 0, .6, .6 },{ 0, .8, .4 },{ 0, .6, 0 },
	{ .6, .8, 0 },{ 1, .6, 0 },{ 1, .2, 0 },{ .8, 0, .4 },{ .6, .2, 1 },{ 0, .2, .8 },
	{ 0, .6, .8 },{ 0, .6, .6 },{ 0, .8, .4 },{ 0, .6, 0 },{ .6, .8, 0 },{ 1, .6, 0 },
	{ 1, .2, 0 },{ .8, 0, .4 },{ .6, .2, 1 },
	// back
	{ 0, .2, .8 },{ 0, .6, .8 },{ 0, .6, .6 },{ 0, .8, .4 },{ 0, .6, 0 },
	{ .6, .8, 0 },{ 1, .6, 0 },{ 1, .2, 0 },{ .8, 0, .4 },{ .6, .2, 1 },{ 0, .2, .8 },
	{ 0, .6, .8 },{ 0, .6, .6 },{ 0, .8, .4 },{ 0, .6, 0 },{ .6, .8, 0 },{ 1, .6, 0 },
	{ 1, .2, 0 },{ .8, 0, .4 },{ .6, .2, 1 } };

// 76 triangles
int triangles[][3] = {
	// front (18)
	{ 0, 1, 2 },{ 1, 2, 3 },{ 1, 3, 4 },{ 3, 4, 5 },{ 4, 5, 6 },{ 5, 6, 7 },
	{ 6, 7, 8 },{ 7, 8, 9 },{ 8, 9, 10 },{ 8, 10, 11 },{ 10, 11, 12 },{ 11, 12, 13 },
	{ 11, 13, 14 },{ 11, 14, 15 },{ 11, 15, 17 },{ 15, 16, 17 },{ 16, 17, 19 },
	{ 16, 18, 19 },
	// back (18)
	{ 19, 20, 21 },{ 20, 21, 22 },{ 20, 22, 23 },{ 22, 23, 24 },{ 23, 24, 25 },
	{ 24, 25, 26 },{ 25, 26, 27 },{ 26, 27, 28 },{ 27, 28, 29 },{ 27, 29, 30 },
	{ 29, 30, 31 },{ 30, 31, 32 },{ 30, 32, 33 },{ 30, 33, 34 },{ 30, 34, 36 },
	{ 34, 35, 36 },{ 35, 36, 38 },{ 35, 37, 38 },
	// sides (40)
	{ 0, 19, 21 },{ 0, 21, 2 },{ 2, 3, 21 },{ 3, 21, 22 },{ 3, 22, 24 },{ 3, 5, 24 },
	{ 5, 7, 24 },{ 7, 24, 26 },{ 7, 9, 26 },{ 9, 26, 28 },{ 9, 28, 29 },{ 9, 10, 29 },
	{ 10, 12, 29 },{ 12, 29, 31 },{ 12, 13, 31 },{ 13, 31, 32 },{ 13, 14, 32 },
	{ 14, 32, 33 },{ 0, 19, 20 },{ 0, 1, 20 },{ 1, 20, 23 },{ 1, 4, 23 },
	{ 4, 23, 25 },{ 4, 6, 25 },{ 6, 8, 25 },{ 8, 25, 27 },{ 8, 27, 30 },{ 8, 11, 30 },
	{ 11, 17, 30 },{ 17, 30, 36 },{ 17, 36, 38 },{ 17, 19, 38 },{ 18, 19, 38 },
	{ 18, 37, 38 },{ 18, 16, 37 },{ 16, 37, 35 },{ 16, 15, 35 },{ 15, 34, 35 },
	{ 14, 15, 34 },{ 14, 33, 34 } };


// Shaders

char *vertexShader = "\
	#version 150													\n\
	in vec4 vPosition;												\n\
	in vec3 vColor;													\n\
	out vec4 color;													\n\
	uniform mat4 view;												\n\
	void main() {													\n\
	    color = vec4(vColor, 1);									\n\
		gl_Position = view*vec4(vPosition.xyz, 1);					\n\
	}";

char *fragmentShader = "\
	#version 150													\n\
	in vec4 color;													\n\
	out vec4 fColor;												\n\
	void main() {													\n\
        fColor = color;												\n\
	}";

// Vertex Buffering

class Vertex {
public:
	vec3 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) : point(vec3(p[0], p[1], p[2])),
		color(vec3(c[0], c[1], c[2])) {}
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	// create vertex array
	int ntriangles = sizeof(triangles) / (3 * sizeof(int));
	int nverts = 3 * ntriangles;
	vertices.resize(nverts);
	for (int i = 0; i < ntriangles; i++) {
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3 * i + k] = Vertex(points[vid], colors[vid]);
		}
	}
	// create and bind GPU vertex buffer, copy vertex data
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(Vertex), &vertices[0],
		GL_STATIC_DRAW);
}

// Button Callback

void MouseButton(int butn, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		mouseDown = vec2((float)x, (float)y);	// save for continuity
	}
	if (state == GLUT_UP)
	{
		rotOld = rotNew;						// save for continuity
	}
}

// Motion (Drag) Callback

void MouseDrag(int x, int y)
{
	// called only if mouse button down
	vec2 mouse((float)x, (float)y);
	rotNew = rotOld + speed * (mouse - mouseDown);
	glutPostRedisplay();
}

// Application

void Display() {
	glClearColor(.1, .1, .1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);

	// update angle
	mat4 view = RotateY(rotNew.x) * RotateX(rotNew.y);
	GLSL::SetUniform(program, "view", view);
	// establish vertex fetch for point and for coor
	GLSL::VertexAttribPointer(program, "vPosition", 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (void *)0);
	GLSL::VertexAttribPointer(program, "vColor", 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (void *) sizeof(vec3));

	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glFlush();
}

void Close() {
	// unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (vBuffer >= 0) {
		glDeleteBuffers(1, &vBuffer);
	}
}

void main(int argc, char **argv) {
	// init window
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Drawing a Strange Looking G");
	glewInit();
	// build and use shader program
	program = GLSL::LinkProgramViaCode(vertexShader, fragmentShader);
	// allocate vertex memory in the GPU and link it to the vertex shader
	InitVertexBuffer();
	// GLUT callbacks and event loop
	glutDisplayFunc(Display);
	glutCloseFunc(Close);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutMainLoop();
}