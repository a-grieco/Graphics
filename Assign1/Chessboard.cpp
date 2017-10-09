
#include "stdafx.h"
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"

GLuint vBufferId = 0;				// GPU vert buffer, valid > 0
GLuint programId = 0;				// GLSL program, valid if > 0

const char *vertexShader = "\
	#version 130															\n\
	in vec2 point;															\n\
	void main() {															\n\
		gl_Position = vec4(point, 0, 1);									\n\
	}																		\n";

const char *pixelShader = "\
	#version 130															\n\
	out vec4 fColor;														\n\
	uniform vec2 uResolution;												\n\
	int numRowsCols = 8;													\n\
	bool odd(int row, int col) {											\n\
		return (row + col) % 2 != 0;										\n\
	}																		\n\
	int getRowOrColNumber(int coord, int windowSize) {						\n\
		int squareSize = windowSize / numRowsCols;							\n\
		return coord / squareSize;											\n\
	}																		\n\
	bool isBlackSquare(int xCoord, int yCoord) {							\n\
		int row = getRowOrColNumber(yCoord, int(uResolution.y));			\n\
		int col = getRowOrColNumber(xCoord, int(uResolution.x));			\n\
		return !odd(row, col);												\n\
	}																		\n\
	void main() {															\n\
		if(gl_FragCoord.x > uResolution.x ||								\n\
				gl_FragCoord.y >= uResolution.y) {							\n\
			fColor = vec4(0, 244, 0, 1);									\n\
		}																	\n\
		else if (isBlackSquare(int(gl_FragCoord.x), int(gl_FragCoord.y))) {	\n\
			fColor = vec4(0, 0, 0, 1);										\n\
		}																	\n\
		else {																\n\
			fColor = vec4(244, 244, 244, 1);								\n\
		}																	\n\
	}																		\n";

void Display() {
	// called whenever application displayed
	glUseProgram(programId);
	GLSL::VertexAttribPointer(programId, "point", 2, GL_FLOAT, GL_FALSE, 0,
		(void *)0);
	glDrawArrays(GL_QUADS, 0, 4);	// display entire window
	glFlush();						// flush GL ops complete
}

void InitVertexBuffer() {
	float pts[][2] = { { -1,-1 },{ -1,1 },{ 1,1 },{ 1,-1 } };
	// create GPU buffer for 4 verts, bind, allocate/copy
	glGenBuffers(1, &vBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

/* code from Professor Bloomenthal */
void Reshape(int w, int h) {
	// called by GLUT whenever application is resized
	// adjust checkerboard to window size by setting 'uResolution'
	glUseProgram(programId);
	int uniform_WindowSize = glGetUniformLocation(programId, "uResolution");
	glUniform2f(uniform_WindowSize, (float)w, (float)h);
}

void main(int argc, char **argv) {	// application entry
	glutInit(&argc, argv);			// init app toolkit
	glutInitWindowSize(400, 400);	// set window size
	glutCreateWindow("Clear");		// create named window
	glewInit();						// wrangle GL extensions
	InitVertexBuffer();				// set GPU vertex memory
	programId = GLSL::LinkProgramViaCode(vertexShader, pixelShader);

	/*
	// adjust checkerboard to initialized window size by setting 'uResolution'
	int uniform_WindowSize = glGetUniformLocation(programId, "uResolution");
	glUseProgram(programId);
	glUniform2f(uniform_WindowSize, glutGet(GLUT_WINDOW_WIDTH),
		glutGet(GLUT_WINDOW_HEIGHT));
	float width = glutGet(GLUT_WINDOW_WIDTH);
	float height = glutGet(GLUT_WINDOW_HEIGHT);
	*/

	/* code added with Reshape function above*/
	glutReshapeFunc(Reshape);
	Reshape(400, 400);

	glutDisplayFunc(Display);		// GLUT display callback
	glutMainLoop();					// enter GLUT event loop
}