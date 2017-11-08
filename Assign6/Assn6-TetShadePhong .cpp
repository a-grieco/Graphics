// Assn6-TetShadePhong.cpp: Phong-shade tetrahedron

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <vector>
#include "GLSL.h"

// #define PERSP        // EC-1
// #define SMOOTH_SHADE // EC-4

// Application Data

GLuint vBuffer = 0;   // GPU vertex buffer ID
GLuint program = 0;   // GLSL program ID

float s = .8f, f = s/sqrt(2.f);
float points[][3] = {{-s, 0, -f}, {s, 0, -f}, {0, -s, f}, {0, s, f}};
float colors[][3] = {{1, 1, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
int triangles[][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};

// Shaders

#ifdef PERSP
char *vertexShader = "\
	#version 130													\n\
	in vec3 point;													\n\
	in vec3 color;													\n\
	in vec3 normal;													\n\
	out vec3 vPoint;												\n\
	out vec3 vColor;												\n\
	out vec3 vNormal;												\n\
	uniform mat4 view;												\n\
	uniform mat4 persp;												\n\
	void main()	{													\n\
		vPoint = (view*vec4(point, 1)).xyz;							\n\
		vNormal = (view*vec4(normal, 0)).xyz;						\n\
		gl_Position = persp*vec4(vPoint, 1);						\n\
	    vColor = color;												\n\
	}";
#else
char *vertexShader = "\
	#version 130													\n\
	in vec3 point;													\n\
	in vec3 color;													\n\
	in vec3 normal;													\n\
	out vec3 vPoint;												\n\
	out vec3 vColor;												\n\
	out vec3 vNormal;												\n\
	uniform mat4 view;												\n\
	void main()	{													\n\
		gl_Position = view*vec4(point, 1);							\n\
		vPoint = gl_Position.xyz;									\n\
		vNormal = (view*vec4(normal, 0)).xyz;						\n\
	    vColor = color;												\n\
	}";
#endif

char *pixelShader = "\
    #version 130													\n\
	in vec3 vPoint;													\n\
	in vec3 vColor;													\n\
	in vec3 vNormal;												\n\
	out vec4 pColor;												\n\
	uniform vec3 light = vec3(.7, .4, 3);							\n\
	float PhongIntensity(vec3 pos, vec3 nrm) {						\n\
		vec3 N = normalize(nrm);           // surface normal		\n\
        vec3 L = normalize(light-pos);     // light vector			\n\
        vec3 E = normalize(pos);           // eye vertex			\n\
        vec3 R = reflect(L, N);            // highlight vector		\n\
        float d = abs(dot(N, L));          // two-sided diffuse		\n\
        float h = abs(dot(R, E));          // highlight term		\n\
		float s = pow(h, 50);			   // specular term			\n\
		return clamp(d+s, 0, 1);									\n\
	}																\n\
    void main() {													\n\
		float intensity = PhongIntensity(vPoint, vNormal);			\n\
		pColor = vec4(intensity*vColor, 1);							\n\
	}";

// Vertex Buffering

// faceted Phong shading requires distinct normals per vertex per triangle
// therefore, all vertices must be repeated (ie, glDrawArrays easier than glDrawElements)

vec3 TriangleNormal(float *a, float *b, float *c) {
	vec3 v1(b[0]-a[0], b[1]-a[1], b[2]-a[2]), v2(c[0]-b[0], c[1]-b[1], c[2]-b[2]);
    return normalize(cross(v1, v2));
}

struct Vertex {
	vec3 point, color, normal;
	Vertex() { }
	Vertex(float *p, float *c, float *n) :
	    point(vec3(p[0], p[1], p[2])),
	    color(vec3(c[0], c[1], c[2])),
		normal(normalize(vec3(n[0], n[1], n[2]))) { }
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	int nvertices = sizeof(triangles)/sizeof(int), ntriangles = nvertices/3;
#ifdef SMOOTH_SHADE
	std::vector<vec3> normals(nvertices);
	// set vertex normal as average face normals surrounding vertex
	for (int i = 0; i < ntriangles; i++) {
		int *t = triangles[i];
		vec3 n = TriangleNormal(points[t[0]], points[t[1]], points[t[2]]);
		for (int k = 0; k < 3; k++)
			normals[t[k]] += n;
	}
	for (int i = 0; i < nvertices; i++)
		normalize(normals[i]);
#endif
	// create vertex array
	vertices.resize(nvertices);
	for (int i = 0; i < ntriangles; i++) {
		int *t = triangles[i];
	#ifdef SMOOTH_SHADE
		// smooth shading
		for (int k = 0; k < 3; k++) {
			int vid = t[k];
			vertices[3*i+k] = Vertex(points[vid], colors[vid], (float *) &normals[vid]);
		}
	#else
		// faceted shading
		vec3 n = TriangleNormal(points[t[0]], points[t[1]], points[t[2]]);
		for (int k = 0; k < 3; k++) {
			int vid = t[k];
			vertices[3*i+k] = Vertex(points[vid], colors[vid], (float *) &n);
		}
	#endif
	}
    // create and bind GPU vertex buffer, copy vertex data
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

// Interaction

vec2  mouseDown;			// reference for mouse drag
vec2  rotOld, rotNew;	    // .x is rotation about Y-axis, .y about X-axis
float rotSpeed = .3f;

// translation variables
vec3	tranOld, tranNew;
float	tranSpeed = .01f;

void MouseButton(int butn, int state, int x, int y) {
	// called when mouse button pressed or released
	if (state == GLUT_DOWN)
		mouseDown = vec2((float) x, (float) y);
	if (state == GLUT_UP) {
		rotOld = rotNew;
		tranOld = tranNew;
	}
}

void MouseDrag(int x, int y) {
	vec2 mouse((float) x, (float) y), dif = mouse-mouseDown;
	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
		tranNew = tranOld+tranSpeed*vec3(dif.x, -dif.y, 0);
	else
		rotNew = rotOld+rotSpeed*dif;
	glutPostRedisplay();
}

// Application

void Display() {
	// clear screen to grey, enable z-buffer
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// update view transformation
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	mat4 view = Translate(tranNew)*RotateY(rotNew.x)*RotateX(rotNew.y);
#ifdef PERSP
	float width = (float) glutGet(GLUT_WINDOW_WIDTH), height = (float) glutGet(GLUT_WINDOW_HEIGHT);
	float aspect = width/height;
	mat4 persp = Perspective(30, aspect, -.001f, -500);
	GLSL::SetUniform(program, "persp", persp);
	view = Translate(0, 0, -3)*view;
#endif
	GLSL::SetUniform(program, "view", view);
    // establish vertex fetch for point, color, and normal
	GLSL::VertexAttribPointer(program, "point", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
	GLSL::VertexAttribPointer(program, "color", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
	GLSL::VertexAttribPointer(program, "normal", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (2*sizeof(vec3)));
 	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glFlush();
}

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(800, 800);
    glutCreateWindow("A Phong Shade Tetrahedron");
    glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
    InitVertexBuffer();
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
    glutCloseFunc(Close);
    glutMainLoop();
}
