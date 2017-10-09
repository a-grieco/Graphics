// ClearGlslApp.cpp: minimal GLSL application to clear window

#include <gl/glew.h>
#include <gl/freeglut.h>

GLuint vBufferId = 0;	// GPU vertex buffer ID, valid if > 0
GLuint programId = 0;	// GLSL program ID, valid if > 0

const char *vertexShader = "\
	#version 130														\n\
	in vec2 point;														\n\
	void main() {														\n\
		gl_Position = vec4(point, 0, 1);								\n\
	}																	\n";

const char *chessboardPixelShader = "\
	#version 130														\n\
    bool odd(float f) {													\n\
        int i = int(f), d = i/2;										\n\
        return i != 2*d;												\n\
    }																	\n\
    void main(void) {													\n\
        bool oddCol = odd(gl_FragCoord.x/50.);							\n\
        bool oddRow = odd(gl_FragCoord.y/50.);							\n\
        gl_FragColor = oddCol == oddRow? vec4(0, 0, 0, 1) :				\n\
		                                 vec4(1, 1, 1, 1);				\n\
	}																	\n";

const char *circlePixelShader = "\
	#version 130														\n\
    void main(void) {													\n\
		float dx = gl_FragCoord.x-200, dy = gl_FragCoord.y-200;			\n\
		float dmax = sqrt(2.)*400;										\n\
		float v = 20*sqrt(dx*dx+dy*dy)/dmax;							\n\
		float r = .5+.5*sin(5*v), g = .5+.5*sin(3*v), b = .5+.5*sin(v);	\n\
		gl_FragColor = vec4(r, 0, 0, 1);								\n\
	}																	\n";


void Display() {
	// called whenever window manager displays the application
	glUseProgram(programId);
	GLint pointId = glGetAttribLocation(programId, "point");
	if (pointId >= 0)
		glEnableVertexAttribArray(pointId);
	glVertexAttribPointer(pointId,  2,  GL_FLOAT, GL_FALSE, 0, (void *) 0);
	glDrawArrays(GL_QUADS, 0, 4);							// display quadrilateral (entire window)
    glFlush();												// ensure GL ops complete, screen refresh
}

void AddShader(GLuint program, const char **source, int type) {
	GLuint shader = glCreateShader(type);					// create shader of type vertex or fragment/pixel
	glShaderSource(shader, 1, source, NULL);				// download source code
	glCompileShader(shader);								// compile source
	glAttachShader(program, shader);						// include shader in future link
}

void InitVertexBuffer() {
	float pts[][2] = {{-1,-1}, {-1,1}, {1,1}, {1,-1}};		// four window corners
    // create GPU buffer, bind to it, allocate memory and copy vertices
    glGenBuffers(1, &vBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

void main(int argc, char **argv) {							// application entry
    glutInit(&argc, argv);									// prepare GLUT for window manager
    glutInitWindowSize(400, 400);							// specify window size
    glutCreateWindow("Pie");								// create named window
    glewInit();												// wrangle GL extensions
	InitVertexBuffer();										// allocate and fill GPU memory
	programId = glCreateProgram();							// create GLSL shader program
	AddShader(programId, &vertexShader, GL_VERTEX_SHADER);	// add vertex shader to GLSL program
	AddShader(programId, &circlePixelShader, GL_FRAGMENT_SHADER);	// add pixel shader
	glLinkProgram(programId);								// link shader program
	glUseProgram(programId);								// enable shader program
	glutDisplayFunc(Display);								// set GLUT display callback
    glutMainLoop();											// enter GLUT event loop
}
