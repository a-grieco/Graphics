// TrianglesG.cpp
// A program to draw multiple colored triangles that form the letter "G" using
// GLSL and vertex buffering

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "GLSL.h"
#include <vector> 

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

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

class Vertex {
public:
	vec2 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) {
		point = vec2(p[0], p[1]);
		color = vec3(c[0], c[1], c[2]);
	}
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	// the letter G: 20 vertices, 20 colors, 18 triangles
	float points[][2] = {{.75, 0}, {.5, 0}, {.75, .5}, {.5, .75}, {.25, .25},
		{-.5, .75}, {-.25, .25}, {-.75, .5}, {-.25, -.25}, {-.75, -.5}, 
		{-.5, -.75}, {0, -.25}, {.5, -.75}, {.75, -.5}, {.75, -.125}, 
		{.375, -.125}, {.125, .125}, {0, -.125}, {-.125, .125}, 
		{-.125, -.125}};
	int triangles[][3] = {{0, 1, 2}, {1, 2, 3}, {1, 3, 4}, {3, 4, 5}, 
		{4, 5, 6}, {5, 6, 7}, {6, 7, 8}, {7, 8, 9}, {8, 9, 10}, {8, 10, 11},
		{10, 11, 12}, {11, 12, 13}, {11, 13, 14}, {11, 14, 15}, {11, 15, 17},
		{15, 16, 17}, {16, 17, 19}, {16, 18, 19}};
	float colors[][3] = {{0, .2, .8}, {0, .6, .8}, {0, .6, .6}, {0, .8, .4}, 
		{0, .6, 0}, {.6, .8, 0}, {1, .6, 0}, {1, .2, 0}, {.8, 0, .4}, 
		{.6, .2, 1}, {0, .2, .8}, {0, .6, .8}, {0, .6, .6}, {0, .8, .4}, 
		{0, .6, 0}, {.6, .8, 0}, {1, .6, 0}, {1, .2, 0}, {.8, 0, .4}, 
		{.6, .2, 1}};
	// create vertex array
	int ntriangles = sizeof(triangles) / (3 * sizeof(int));
	int nverts = 3 * ntriangles;
	vertices.resize(nverts);
	for (int i = 0; i < ntriangles; i++) {  // for each triangle
		for (int k = 0; k < 3; k++) {	// go through its 3 vertices
			int vid = triangles[i][k];
			// set vertex k of triangle i
			vertices[3 * i + k] = Vertex(points[vid], colors[vid]);
		}
	}
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(Vertex), &vertices[0],
		GL_STATIC_DRAW);
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
	glClearColor(.1, .1, .1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
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
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (vBuffer >= 0) {
		glDeleteBuffers(1, &vBuffer);
	}
}

void main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Triangles forming a \"G\"");
	glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, fragmentShader);
	InitVertexBuffer();
	glutDisplayFunc(Display);
	glutCloseFunc(Close);
	glutMainLoop();
}


