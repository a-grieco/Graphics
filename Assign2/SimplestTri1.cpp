// SimplestTri1.cpp
// simplest program to draw colored triangle with GLSL and vertex buffering

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "GLSL.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// a triangle
float  points[][2] = {{-.9f, -.9f}, {0, .9f}, {.9f, -.9f}};
float  colors[][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

char *vertexShader = "\
	#version 150					\n\
	in vec4 vPosition;				\n\
	in vec3 vColor;					\n\
	out vec4 color;					\n\
	void main() {					\n\
		gl_Position = vPosition;	\n\
	    color = vec4(vColor, 1);	\n\
	}";

char *fragmentShader = "\
	#version 150					\n\
	in vec4 color;					\n\
	out vec4 fColor;				\n\
	void main() {					\n\
        fColor = color;				\n\
	}";

void InitVertexBuffer() {
    // create a vertex buffer for the array, and make it the active vertex buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);	// to be a vertex array buffer
    // allocate buffer memory to hold vertex locations and colors
    glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    // load data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
        // start at beginning of buffer, for length of points array
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
        // start at end of points array, for length of colors array
}

void InitShader() {
    // build and use shader program from inline code
	program = GLSL::LinkProgramViaCode(vertexShader, fragmentShader);
    // status
    if (program)
        GLSL::PrintProgramAttributes(program);
    else
        printf("Failed to link shader program\n");
}

void Display() {
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    // associate position input to shader with position array in vertex buffer 
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    // associate color input to shader with color array in vertex buffer
    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
	// finally, render three vertices as a triangle
	glDrawArrays(GL_TRIANGLES, 0, 3);
    glFlush();
}

void Close() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Simple Tri");
    glewInit();
    InitVertexBuffer();
    InitShader();
    glutDisplayFunc(Display);
    glutCloseFunc(Close);
    glutMainLoop();
}


