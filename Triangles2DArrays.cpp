// Triangles.cpp
// A program to draw multiple colored triangles that form the letter "G" using
// GLSL and vertex buffering

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "GLSL.h"
#include <vector> 

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// 18 triangles
float points[][2] = { {.75, 0}, {.5, 0}, {.75, .5}, {.5, .75}, {.25, .25},
	{-.5, .75}, {-.25, .25}, {-.75, .5}, {-.25, -.25}, {-.75, -.5},
	{-.5, -.75}, {0, -.25}, {.5, -.75}, {.75, -.5}, {.75, -.125},
	{.375, -.125}, {.125, .125}, {0, -.125}, {-.125, .125}, {-.125, -.125} };
int triPoints[][3] = { {0, 1, 2}, {1, 2, 3}, {1, 3, 4}, {3, 4, 5}, {4, 5, 6},
	{5, 6, 7}, {6, 7, 8}, {7, 8, 9 }, {8, 9, 10}, {8, 10, 11}, {10, 11, 12},
	{11, 12, 13}, {11, 13, 14}, {11, 14, 15}, {11, 15, 17}, {15, 16, 17},
	{16, 17, 19}, {16, 18, 19} };

//std::vector <vec2> pts = { { .75, 0 },{ .5, 0 },{ .75, .5 },{ .5, .75 },{ .25, .25 },
//{ -.5, .75 },{ -.25, .25 },{ -.75, .5 },{ -.25, -.25 },{ -.75, -.5 },
//{ -.5, -.75 },{ 0, -.25 },{ .5, -.75 },{ .75, -.5 },{ .75, -.125 },
//{ .375, -.125 },{ .125, .125 },{ 0, -.125 },{ -.125, .125 },{ -.125, -.125 } };
//
//std::vector<vec3> trianglePts = { { 0, 1, 2 },{ 1, 2, 3 },{ 1, 3, 4 },{ 3, 4, 5 },{ 4, 5, 6 },
//{ 5, 6, 7 },{ 6, 7, 8 },{ 7, 8, 9 },{ 8, 9, 10 },{ 8, 10, 11 },{ 10, 11, 12 },
//{ 11, 12, 13 },{ 11, 13, 14 },{ 11, 14, 15 },{ 11, 15, 17 },{ 15, 16, 17 },
//{ 16, 17, 19 },{ 16, 18, 19 } };
//
//std::vector<vec2> triangles;

float triangles[][2] = {
	{ points[0][0], points[0][1] },{ points[1][0], points[1][1] },{ points[2][0], points[2][1] },
	{ points[1][0], points[1][1] },{ points[2][0], points[2][1] },{ points[3][0], points[3][1] },
	{ points[1][0], points[1][1] },{ points[3][0], points[3][1] },{ points[4][0], points[4][1] },
	{ points[3][0], points[3][1] },{ points[4][0], points[4][1] },{ points[5][0], points[5][1] },
	{ points[4][0], points[4][1] },{ points[5][0], points[5][1] },{ points[6][0], points[6][1] },
	{ points[5][0], points[5][1] },{ points[6][0], points[6][1] },{ points[7][0], points[7][1] },
	{ points[6][0], points[6][1] },{ points[7][0], points[7][1] },{ points[8][0], points[8][1] },
	{ points[7][0], points[7][1] },{ points[8][0], points[8][1] },{ points[9][0], points[9][1] },
	{ points[8][0], points[8][1] },{ points[9][0], points[9][1] },{ points[10][0], points[10][1] },
	{ points[8][0], points[8][1] },{ points[10][0], points[10][1] },{ points[11][0], points[11][1] },
	{ points[10][0], points[10][1] },{ points[11][0], points[11][1] },{ points[12][0], points[12][1] },
	{ points[11][0], points[11][1] },{ points[12][0], points[12][1] },{ points[13][0], points[13][1] },
	{ points[11][0], points[11][1] },{ points[13][0], points[13][1] },{ points[14][0], points[14][1] },
	{ points[11][0], points[11][1] },{ points[14][0], points[14][1] },{ points[15][0], points[15][1] },
	{ points[11][0], points[11][1] },{ points[15][0], points[15][1] },{ points[17][0], points[17][1] },
	{ points[15][0], points[15][1] },{ points[16][0], points[16][1] },{ points[17][0], points[17][1] },
	{ points[16][0], points[16][1] },{ points[17][0], points[17][1] },{ points[19][0], points[19][1] },
	{ points[16][0], points[16][1] },{ points[18][0], points[18][1] },{ points[19][0], points[19][1] } };

float  colors[][3] = {
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0},
	{1, 0, 0}, {0, 1, 0}, {0, 0, 1},
	{1, 0, 0}, {0, 0, 1}, {0, 1, 0},
	{0, 0, 1}, {0, 1, 0}, {1, 0, 0},
	{0, 1, 0} ,{1, 0, 0}, {0, 0, 1},
	{1, 0, 0}, {0, 0, 1}, {0, 1, 0},
	{0, 0, 1}, {0, 1, 0}, {1, 0, 0},
	{0, 1, 0} ,{1, 0, 0}, {0, 0, 1},
	{1, 0, 0}, {0, 0, 1}, {0, 1, 0},
	{1, 0, 0}, {0, 1, 0}, {0, 0, 1},
	{0, 1, 0}, {0, 0, 1}, {1, 0, 0},
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0},
	{0, 0, 1}, {0, 1, 0} ,{1, 0, 0},
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0},
	{0, 0, 1}, {0, 1, 0}, {1, 0, 0},
	{0, 1, 0}, {0, 0, 1}, {1, 0, 0},
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0},
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

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

//std::vector<vec2> getTriangles()
//{
//	std::vector<vec2> triangles;
//	for(int i = 0; i < trianglePts.size(); i++)
//	{
//		triangles.push_back(pts[trianglePts[i].x]);
//		triangles.push_back(pts[trianglePts[i].y]);
//		triangles.push_back(pts[trianglePts[i].z]);
//	}
//	return triangles;
//}

void InitVertexBuffer() {
//	triangles = getTriangles();
	// create a vertex buffer for the array, and make it the active vertex buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);	// to be a vertex array buffer
	// allocate buffer memory to hold vertex locations and colors
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangles) + sizeof(colors), NULL, GL_STATIC_DRAW);
	// load data to the GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(triangles), triangles);
	// start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(triangles), sizeof(colors), colors);
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
	glClearColor(.1, .1, .1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	// associate position input to shader with position array in vertex buffer 
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
	// associate color input to shader with color array in vertex buffer
	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(triangles));
	// finally, render three vertices as a triangle
	glDrawArrays(GL_TRIANGLES, 0, 54);
	glFlush();
}

void Close() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Triangles forming a \"G\"");
	glewInit();
	InitVertexBuffer();
	InitShader();
	glutDisplayFunc(Display);
	glutCloseFunc(Close);
	glutMainLoop();
}


