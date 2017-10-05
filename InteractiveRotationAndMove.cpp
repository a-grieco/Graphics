// InteractiveRotationAndMove.cpp

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
float rotSpeed = 0.3;
vec2 tranOld, tranNew;
float tranSpeed = .01;

// 20 2D vertex location for the letter 'G'
float points[][2] = { {.75, 0}, {.5, 0}, {.75, .5}, {.5, .75}, {.25, .25},
	{-.5, .75}, {-.25, .25}, {-.75, .5}, {-.25, -.25}, {-.75, -.5},
	{-.5, -.75}, {0, -.25}, {.5, -.75}, {.75, -.5}, {.75, -.125},
	{.375, -.125}, {.125, .125}, {0, -.125}, {-.125, .125}, {-.125, -.125} };

// 20 colors
float colors[][3] = { {0, .2, .8}, {0, .6, .8}, {0, .6, .6}, {0, .8, .4},
	{0, .6, 0}, {.6, .8, 0}, {1, .6, 0}, {1, .2, 0}, {.8, 0, .4}, {.6, .2, 1},
	{0, .2, .8}, {0, .6, .8}, {0, .6, .6}, {0, .8, .4}, {0, .6, 0},
	{.6, .8, 0}, {1, .6, 0}, {1, .2, 0}, {.8, 0, .4}, {.6, .2, 1} };

// 18 triangles
int triangles[][3] = { {0, 1, 2}, {1, 2, 3}, {1, 3, 4}, {3, 4, 5}, {4, 5, 6},
	{5, 6, 7}, {6, 7, 8}, {7, 8, 9}, {8, 9, 10}, {8, 10, 11}, {10, 11, 12},
	{11, 12, 13}, {11, 13, 14}, {11, 14, 15}, {11, 15, 17}, {15, 16, 17},
	{16, 17, 19}, {16, 18, 19} };

// Shaders

char *vertexShader = "\
	#version 150													\n\
	in vec4 vPosition;												\n\
	in vec3 vColor;													\n\
	out vec4 color;													\n\
	uniform mat4 view;												\n\
	void main() {													\n\
	    color = vec4(vColor, 1);									\n\
		gl_Position = view*vec4(vPosition.xy, 0, 1);				\n\
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
	vec2 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) : point(vec2(p[0], p[1])),
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
		mouseDown = vec2((float)x, (float)y);
	}
	if (state == GLUT_UP)
	{
		rotOld = rotNew;
		tranOld = tranNew;
	}
}

// Motion (Drag) Callback

void MouseDrag(int x, int y)
{
	// called only if mouse button down
	vec2 mouse((float)x, (float)y), dif = mouse - mouseDown;
	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	{
		tranNew = tranOld + tranSpeed * vec2(dif.x, -dif.y);
	}
	else
	{
		rotNew = rotOld + rotSpeed * (dif);
	}
	glutPostRedisplay();
}

// Application

void Display() {
	glClearColor(.1, .1, .1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	// update angle
	mat4 view = Translate(tranNew.x, tranNew.y, 0) * RotateY(rotNew.x) *
		RotateX(rotNew.y);
	GLSL::SetUniform(program, "view", view);
	// establish vertex fetch for point and for coor
	GLSL::VertexAttribPointer(program, "vPosition", 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (void *)0);
	GLSL::VertexAttribPointer(program, "vColor", 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (void *) sizeof(vec2));
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
	glutCreateWindow("Interactive Rotation and Move");
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