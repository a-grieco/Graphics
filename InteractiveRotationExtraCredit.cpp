// InteractiveRotationExtraCredit.cpp

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

// drawing a tetrahedron
float s = 0.8, f = s / sqrt(2.0);
float points[][3] = { {-s, 0, -f}, {s, 0, -f}, {0, -s, f}, {0, s, f} };

// 40 colors
float colors[][3] = { {0, .2, .8}, {0, .6, .6}, {0, .6, 0}, {.6, .2, 1} };

// 76 triangles
int triangles[][3] = { {0, 1, 2}, {1, 2, 3}, {0, 2, 3}, {0, 1, 3} };

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
	glutCreateWindow("Drawing a Tetrahedron");
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