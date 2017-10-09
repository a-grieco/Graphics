// LetterFromTriangles.cpp: draw multiple triangles to display a letter

#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include <vector>

// shaders
GLuint vBuffer = 0;	// GPU vertex buffer ID, valid if > 0
GLuint program = 0;	// GLSL program ID, valid if > 0

char *vertexShader = "\
	#version 130								\n\
	in vec2 vPoint;								\n\
	in vec3 vColor;								\n\
	out vec4 color;								\n\
	void main() {								\n\
		gl_Position = vec4(vPoint, 0, 1);		\n\
		color = vec4(vColor, 1);				\n\
	}\n";

char *pixelShader = "\
	#version 130								\n\
	in vec4 color;								\n\
	out vec4 fColor;							\n\
	void main()	{								\n\
        fColor = color;							\n\
	}\n";

class Vertex {
public:
	vec2 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) { point = vec2(p[0], p[1]); color = vec3(c[0], c[1], c[2]); }
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	// the letter B: 10 vertices, 10 colors, 9 triangles
	float points[][2] = {{-.15f, .125f}, {-.5f, -.75f}, {-.5f,   .75f}, {.17f, .75f}, {.38f, .575f},	
	                     {.38f,   .35f}, {.23f, .125f}, {.5f,  -.125f}, {.5f,  -.5f}, {.25f, -.75f}};
	float colors[][3] = {{1, 1, 1}, {1, 0, 0}, {.5f, 0, 0}, {1, 1, 0}, {.5f, 1, 0},
					     {0, 1, 0}, {0, 1, 1}, {0,   0, 1}, {1, 0, 1}, {.5f, 0, .5f}};
	int triangles[][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5},
						  {0, 5, 6}, {0, 6, 7}, {0, 7, 8}, {0, 8, 9}, {0, 9, 1}};
	// create vertex array
	int ntriangles = sizeof(triangles)/(3*sizeof(int));
	vertices.resize(3*ntriangles);
	for (int i = 0; i < ntriangles; i++)
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3*i+k] = Vertex(points[vid], colors[vid]);
		}
    // create GPU buffer, make it active, allocate memory and copy vertices
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

void Display() {
	// clear screen to grey
    glClearColor(.5f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
    // establish vertex fetch for point and for color
	GLSL::VertexAttribPointer(program, "vPoint", 2,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
	GLSL::VertexAttribPointer(program, "vColor", 3,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec2));
	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glFlush();
}

void Close() {
	// unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (vBuffer >= 0)
		glDeleteBuffers(1, &vBuffer);
}

void main(int ac, char **av) {
    // init window
    glutInit(&ac, av);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Letter from Triangles");
	glClearColor(0, 0, 0, 1);
	// obtain OpenGL extension bindings, init shaders
	glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	InitVertexBuffer();
    // GLUT callbacks
    glutDisplayFunc(Display);
    glutCloseFunc(Close);
    // begin event handler
    glutMainLoop();
}
