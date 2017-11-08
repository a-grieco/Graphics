// Assn3-RotateLetter.cpp: rotate letter over time

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include <time.h>

// Application Data

GLuint vBuffer = 0;     // GPU vertex buffer ID 
GLuint program = 0;		// GLSL program ID

// 10 2D vertex locations for 'B'
float points[][2] = {{-.15f, .125f}, {-.5f,  -.75f}, {-.5f,  .75f}, {.17f,  .75f}, { .38f, .575f},
	                 { .38f,  .35f}, { .23f, .125f}, {.5f, -.125f}, { .5f, -.5f},  {.25f, -.75f}};

// 10 colors
float colors[][3] = {{ 1, 1, 1}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0},  {.5, 1, 0},
	                 { 0, 1, 0}, { 0, 1, 1}, {0, 0, 1},  { 1, 0, 1}, {.5, 0, .5}};

// 9 triangles
int triangles[][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5},
                      {0, 5, 6}, {0, 6, 7}, {0, 7, 8}, {0, 8, 9}, {0, 9, 1}};

// Vertex and Pixel Shaders

char *vertexShader = "\
	#version 130								\n\
	in vec2 point;								\n\
	in vec3 color;								\n\
	out vec4 vColor;							\n\
	uniform float radAng = 0;					\n\
	vec2 Rotate2D(vec2 v) {						\n\
		float c = cos(radAng), s = sin(radAng);	\n\
		return vec2(c*v.x-s*v.y, s*v.x+c*v.y);	\n\
	}											\n\
	void main() {								\n\
		vec2 r = Rotate2D(point);				\n\
		gl_Position = vec4(r, 0, 1);			\n\
	    vColor = vec4(color, 1);				\n\
	}\n";

char *pixelShader = "\
	#version 130								\n\
	in vec4 vColor;								\n\
	out vec4 pColor;							\n\
	void main() {								\n\
        pColor = vColor;						\n\
	}\n";

// Initialization

void InitVertexBuffer() {
    // create GPU buffer to hold positions and colors, and make it the active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex positions and colors
    glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    // load data to sub-buffers
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
}

// Animation

time_t startTime = clock();
static float degPerSec = 30;

void Idle() {
	float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC; // duration since start
	GLint id = glGetUniformLocation(program, "radAng");
	if (id >= 0)
		glUniform1f(id, (3.1415f/180.f)*dt*degPerSec);
	glutPostRedisplay();
}

void Display() {
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    // set vertex feed for points and colors, then draw
    GLSL::VertexAttribPointer(program, "point", 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    GLSL::VertexAttribPointer(program, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
    glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(int), GL_UNSIGNED_INT, triangles);
    glFlush();
}

// Application

void Close() {
	// unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	// init window
    glutInit(&argc, argv);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Rotate Letter");
    glewInit();
	// build and use shader program
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	// allocate vertex memory in the GPU and link it to the vertex shader
    InitVertexBuffer();
	// GLUT callbacks and event loop
    glutDisplayFunc(Display);
    glutCloseFunc(Close);
	glutIdleFunc(Idle);
    glutMainLoop();
}
