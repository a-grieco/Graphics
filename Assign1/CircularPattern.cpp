
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
	vec2 center = vec2(uResolution.x/2., uResolution.y/2.);					\n\
	vec2 position = gl_FragCoord.xy - center;								\n\
	int numCircles = 18;													\n\
	float maxRadius = sqrt(pow(uResolution.x/2, 2) +						\n\
							pow(uResolution.y/2, 2));						\n\
	int circleWidth = int(maxRadius / numCircles);							\n\
	vec4 orange = vec4(1.0, 0.5, 0.0, 1);									\n\
	vec4 yellow = vec4(1.0, 1.0, 0.0, 1);									\n\
	vec4 green = vec4(0.5, 0.8, 0.0, 1);									\n\
	vec4 blue = vec4(0.0, 0.4, 0.8, 1);										\n\
	vec4 getColor(int colorGroup) {											\n\
		vec4 color;															\n\
		if(colorGroup % 4 == 0) {											\n\
			color = blue;													\n\
		}																	\n\
		else if(colorGroup % 3 == 0) {										\n\
			color = green;													\n\
		}																	\n\
		else if (colorGroup % 2 == 0) {										\n\
			color = yellow;													\n\
		}																	\n\
		else {																\n\
			color = orange;													\n\
		}																	\n\
		return color;														\n\
	}																		\n\
	float getBlend(float start, float finish, float percent) {				\n\
		return (finish-start)*percent + start;								\n\
	}																		\n\
	vec4 getBlendedColor(vec4 from, vec4 to, float percent)	{				\n\
		float red = getBlend(from.r, to.r, percent);						\n\
		float green = getBlend(from.g, to.g, percent);						\n\
		float blue = getBlend(from.b, to.b, percent);						\n\
		return vec4(red, green, blue, 1);									\n\
	}																		\n\
	void main() {															\n\
		int colorGroup = int(length(position))/circleWidth;					\n\
		vec4 insideNeighbor = getColor(colorGroup - 1);						\n\
		vec4 outsideNeighbor = getColor(colorGroup + 1);					\n\
		vec4 thisCircle = getColor(colorGroup);								\n\
		vec4 insideSeam = getBlendedColor(insideNeighbor, thisCircle, 0.5);	\n\
		vec4 outsideSeam = getBlendedColor(outsideNeighbor,					\n\
											thisCircle, 0.5);				\n\
		float edgeProximity = (length(position)/circleWidth) - colorGroup;	\n\
		if(edgeProximity <= .25) {											\n\
			fColor = getBlendedColor(insideSeam, thisCircle,				\n\
										edgeProximity/0.25);				\n\
		}																	\n\
		else if(edgeProximity >= 0.75) {									\n\
			fColor = getBlendedColor(thisCircle, outsideSeam,				\n\
										(edgeProximity-0.75)/0.25);			\n\
		}																	\n\
		else {																\n\
			fColor = thisCircle;											\n\
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

void main(int argc, char **argv) {	// application entry
	glutInit(&argc, argv);			// init app toolkit
	glutInitWindowSize(400, 400);	// set window size
	glutCreateWindow("Clear");		// create named window
	glewInit();						// wrangle GL extensions
	InitVertexBuffer();				// set GPU vertex memory
	programId = GLSL::LinkProgramViaCode(vertexShader, pixelShader);

	// adjust checkerboard to initialized window size by setting 'uResolution'
	int uniform_WindowSize = glGetUniformLocation(programId, "uResolution");
	glUseProgram(programId);
	glUniform2f(uniform_WindowSize, glutGet(GLUT_WINDOW_WIDTH),
		glutGet(GLUT_WINDOW_HEIGHT));
	float width = glutGet(GLUT_WINDOW_WIDTH);
	float height = glutGet(GLUT_WINDOW_HEIGHT);

	glutDisplayFunc(Display);		// GLUT display callback
	glutMainLoop();					// enter GLUT event loop
}