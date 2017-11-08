// ShadeMeshOBJ_Texture.cpp: .obj mesh with texture

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include "MeshIO.h"

// Application Data

bool bonus3 = true;

char        *objFilename = "C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\teacup.obj";
char		*txtrFilemame = "C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\vg_almond_blossoms.tga";
char		*txtrWrapFilename = "C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\vg_wrap.tga";

vector<vec3> points;				// 3D mesh vertices
vector<vec3> normals;				// vertex normals
vector<int3> triangles;				// triplets of vertex indices
vector<vec2> textures;				// for texture mapping

vec3         lightSource(1, 1, 0);	// for Phong shading
GLuint		 vBuffer = 0;			// GPU vertex buffer ID
GLuint		 program = 0;			// GLSL program ID
GLuint		 textureId = 0;			// GLSL texture ID

// Shaders

char *vertexShader = "\
	#version 130														\n\
	in vec3 point;														\n\
	in vec3 normal;														\n\
	in vec2 uv;															\n\
	out vec3 vPoint;													\n\
	out vec3 vNormal;													\n\
	out vec2 vUv;														\n\
    uniform mat4 view;													\n\
	uniform mat4 persp;													\n\
	void main() {														\n\
		vPoint = (view*vec4(point, 1)).xyz;								\n\
		vNormal = (view*vec4(normal, 0)).xyz;							\n\
		vUv = uv;														\n\
		gl_Position = persp*vec4(vPoint, 1);							\n\
	}";

char *pixelShader = "\
    #version 130														\n\
	in vec3 vPoint;														\n\
	in vec3 vNormal;													\n\
	in vec2 vUv;														\n\
	out vec4 pColor;													\n\
	uniform vec3 light = vec3(-.2, .1, -3);								\n\
	uniform sampler2D textureImage;										\n\
	vec3 color = texture(textureImage, vUv).rgb;						\n\
    void main() {														\n\
		vec3 N = normalize(vNormal);       // surface normal			\n\
        vec3 L = normalize(light-vPoint);  // light vector				\n\
        vec3 E = normalize(vPoint);        // eye vertex				\n\
        vec3 R = reflect(L, N);            // highlight vector			\n\
        float d = abs(dot(N, L));          // two-sided diffuse			\n\
        float s = abs(dot(R, E));          // two-sided specular		\n\
		float intensity = clamp(d+pow(s, 50), 0, 1);					\n\
		pColor = vec4(intensity*color, 1);								\n\
	}";

// Initialization

void InitVertexBuffer() {
	// create GPU buffer, make it the active buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate memory for vertex positions and normals
	int sizePts = points.size() * sizeof(vec3);
	int sizeNrms = normals.size() * sizeof(vec3);
	int sizeTxtr = textures.size() * sizeof(vec2);
	glBufferData(GL_ARRAY_BUFFER, sizePts + sizeNrms + sizeTxtr, NULL, GL_STATIC_DRAW);
	// copy data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &points[0]);
	if (sizeNrms)
		glBufferSubData(GL_ARRAY_BUFFER, sizePts, sizeNrms, &normals[0]);
	if (sizeTxtr)
		glBufferSubData(GL_ARRAY_BUFFER, sizePts + sizeNrms, sizeTxtr, &textures[0]);
}

// Interactive Rotation

vec2 mouseDown;				// for each mouse down, need start point
vec2 rotOld, rotNew;	    // previous, current rotations

void MouseButton(int butn, int state, int x, int y) {
	if (state == GLUT_DOWN)
		mouseDown = vec2((float)x, (float)y);
	if (state == GLUT_UP)
		rotOld = rotNew;
}

void MouseDrag(int x, int y) {
	vec2 mouse((float)x, (float)y);
	rotNew = rotOld + .3f*(mouse - mouseDown);
	glutPostRedisplay();
}

// Application

void Display() {
	glUseProgram(program);
	// update view matrix
	mat4 view = Translate(0, 0, -6)*RotateY(rotNew.x)*RotateX(rotNew.y);
	GLSL::SetUniform(program, "view", view);
	// update persp matrix
	static float fov = 15, nearPlane = -.001f, farPlane = -500;
	static float aspect = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);
	mat4 persp = Perspective(fov, aspect, nearPlane, farPlane);
	GLSL::SetUniform(program, "persp", persp);
	// transform light and send to fragment shader
	vec4 hLight = view*vec4(lightSource, 1);
	GLSL::SetUniform(program, "light", vec3(hLight.x, hLight.y, hLight.z));
	// clear screen to grey, enable transparency, use z-buffer
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// setup vertex feeder
	int sizePts = points.size() * sizeof(vec3);
	int sizeNrms = normals.size() * sizeof(vec3);
	GLSL::VertexAttribPointer(program, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	GLSL::VertexAttribPointer(program, "normal", 3, GL_FLOAT, GL_FALSE, 0, (void *)sizePts);
	GLSL::VertexAttribPointer(program, "uv", 2, GL_FLOAT, GL_FALSE, 0, (void *)(sizePts + sizeNrms));
	// draw triangles, finish
	glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
	glFlush();
}

void InitTexture(const char *filename) {
	int width, height;
	char *pixels = ReadTexture(filename, width, height);
	printf("width: %i, height: %i", width, height);
	if (pixels) {
		// allocate GPU texture buffer; copy, free pixels
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // in case width not multiple of 4
											   // transfer pixel data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
		delete[] pixels;
		glGenerateMipmap(GL_TEXTURE_2D);
		// refer sampler uniform to texture 0 (default texture)
		// for multiple textures, see glActiveTexture
		GLSL::SetUniform(program, "textureImage", 0);
	}
}


void Close() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Texture Example");
	glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	if (!ReadAsciiObj(objFilename, points, triangles, &normals, &textures)) {
		printf("failed to read obj file\n");
		getchar();
		return;
	}
	printf("%i vertices, %i triangles, %i normals, %i textures\n",
		points.size(), triangles.size(), normals.size(), textures.size());
	Normalize(points, .8f); // scale/move model to uniform +/-1, approximate normals if none from file
	if(bonus3) { InitTexture(txtrWrapFilename); }
	else { InitTexture(txtrFilemame); }
	InitVertexBuffer();
	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutCloseFunc(Close);
	glutMainLoop();
}
