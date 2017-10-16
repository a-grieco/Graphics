// TetShadePhong.cpp: Phong-shaded tetrahderon

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include <vector>
#include "Widget.h"
#include "Draw.h"

void *picked = NULL;	// user has(/not) picked slider control
float white[] = { 1,1,1 };
Slider fov(30, 20, 70, 5, 85, 45, Ver, "fov", white);	// vertical slider

// Application Data

GLuint vBuffer = 0;   // GPU vertex buffer ID
GLuint program = 0;   // GLSL program ID

float s = .8f, f = s / sqrt(2.f);
float points[][3] = { {-s, 0, -f}, {s, 0, -f}, {0, -s, f}, {0, s, f} };
float colors[][3] = { {1, 1, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
int triangles[][3] = { {0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3} };

// Shaders

char *vertexShader = "\
	#version 130													\n\
	in vec3 point;													\n\
	in vec3 color;													\n\
	in vec3 normal;													\n\
	out vec3 vColor;												\n\
	out vec3 vPoint;												\n\
	out vec3 vNormal;												\n\
	uniform mat4 view;												\n\
	void main()	{													\n\
		gl_Position = view*vec4(point, 1);							\n\
		vPoint = gl_Position.xyz;									\n\
		vNormal = (view*vec4(normal, 0)).xyz;						\n\
		vColor = color;												\n\
	}";

// Phong pixel shader

char *pixelShader = "\
    #version 130													\n\
	in vec3 vColor;													\n\
	in vec3 vPoint;													\n\
	in vec3 vNormal;												\n\
	out vec4 pColor;												\n\
	uniform vec3 light = vec3(.4, .3, -.5);							\n\
	float Intensity(vec3 pos, vec3 nrm) {							\n\
		vec3 N = normalize(nrm);			// surface normal		\n\
        vec3 L = normalize(light - pos);	// light vector			\n\
		vec3 R = reflect(L, N);				// reflected			\n\
		vec3 E = normalize(pos);			// eye					\n\
		float a = 0.2;						// ambient light		\n\
        float d = abs(dot(N, L));			// two-sided diffuse	\n\
		float h = abs(dot(R, E));			// hilight				\n\
		float s = pow(h, 500);				// specular component	\n\
		return clamp(a+d+s, 0, 1);									\n\
	}																\n\
    void main() {													\n\
		pColor = vec4(vColor*Intensity(vPoint, vNormal), 1);		\n\
	}";

// Vertex Buffering

struct Vertex {
	vec3 point, color, normal;
	Vertex() { }
	Vertex(float *p, float *c, float *n) :
		point(vec3(p[0], p[1], p[2])),
		color(vec3(c[0], c[1], c[2])),
		normal(vec3(n[0], n[1], n[2])) { }
};

std::vector<Vertex> vertices;

vec3 Normal(float *a, float *b, float *c) {
	vec3 v1(b[0] - a[0], b[1] - a[1], b[2] - a[2]), v2(c[0] - b[0], c[1] - b[1], c[2] - b[2]);
	return normalize(cross(v1, v2));
}

void InitVertexBuffer() {
	// create vertex array
	int nvertices = sizeof(triangles) / sizeof(int), ntriangles = nvertices / 3;
	vertices.resize(nvertices);
	for (int i = 0; i < ntriangles; i++) {
		int *tri = triangles[i];
		vec3 n = Normal(points[tri[0]], points[tri[1]], points[tri[2]]);
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3 * i + k] = Vertex(points[vid], colors[vid], (float *)&n);
		}
	}
	// create and bind GPU vertex buffer, copy vertex data
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

// Interaction

vec2	mouseDown;			// reference for mouse drag
vec2	rotOld, rotNew;	    // .x is rotation about Y-axis, .y about X-axis
float	rotSpeed = .3f;

// translation variables
vec3	tranOld, tranNew;
float	tranSpeed = .01f;

void MouseButton(int butn, int state, int x, int y) {
	// invert y for upward increasing screen
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;
	// called when mouse button pressed or released
	if (state == GLUT_DOWN) {
		if (fov.Hit(x, y)) {
			picked = &fov;	// user selected slider
		}
		else {
			mouseDown = vec2((float)x, (float)y);	// mouse drag
		}
	}
	if (state == GLUT_UP) {
		picked = NULL;
		rotOld = rotNew;
		tranOld = tranNew;
	}
}

void MouseDrag(int x, int y) {
	// invert y for upward increasing screen
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;
	if (picked == &fov) {
		fov.Mouse(x, y);
	}
	else {
		vec2 mouse((float)x, (float)y), dif = mouse - mouseDown;
		if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
			tranNew = tranOld + tranSpeed*vec3(dif.x, -dif.y, 0);  // shift move
		}
		else {
			rotNew = rotOld + rotSpeed*dif;	// rptation
		}
	}
	glutPostRedisplay();
}

// Application

void Display() {
	// clear screen to grey
	glClearColor(.1, .1, .1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// enable z-buffer (needed for tetrahedron)
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// update view transformation
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	mat4 view = Translate(tranNew.x, tranNew.y, 0)*RotateY(rotNew.x)*RotateX(rotNew.y);
	mat4 dolly = Translate(0, 0, -1);
	float width = (float)glutGet(GLUT_WINDOW_WIDTH);
	float height = (float)glutGet(GLUT_WINDOW_HEIGHT);
	float aspect = width / height;
	//	mat4 proj = Ortho(-1, 1, -1, 1, 0, 10);
	mat4 proj = Perspective(fov.GetValue(), aspect, -0.01, -500);
	GLSL::SetUniform(program, "view", proj*dolly*view);
	// establish vertex fetch for point, color, and normal
	GLSL::VertexAttribPointer(program, "point", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
	GLSL::VertexAttribPointer(program, "color", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
	GLSL::VertexAttribPointer(program, "normal", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(2 * sizeof(vec3)));
	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	// draw slider controls
	glDisable(GL_DEPTH_TEST);
	mat4 screen = Translate(-1, -1, 0) * Scale(2 / width, 2 / height, 1);
	UseDrawShader(screen);
	fov.Draw();
	glFlush();
}

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Phong-shaded tetrahderon with perspective");
	glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	InitVertexBuffer();
	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutCloseFunc(Close);
	glutMainLoop();
}
