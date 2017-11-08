// TextureMesh.cpp: Phong shade .obj mesh with .tga texture

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include "MeshIO.h"

// Application Data

char        *objFilename = "C:/Users/jules/SeattleUniversity/Web/Models/Teacup.obj";
char		*texFilename = "C:/Users/jules/SeattleUniversity/Exe/Soccer.tga";

vector<vec3> points;
vector<vec3> normals;
vector<vec2> textures;
vector<int3> triangles;

vec3         lightSource(1, 1, 0);
GLuint		 programId = 0, vBufferId = 0, textureId = 0;

// Shaders

char *vertexShader = "\
	#version 130													\n\
	in vec3 point;													\n\
	in vec3 normal;													\n\
	in vec2 uv;														\n\
	out vec3 vPoint;												\n\
	out vec3 vNormal;												\n\
	out vec2 vUv;													\n\
    uniform mat4 view;												\n\
	uniform mat4 persp;												\n\
	void main() {													\n\
		vPoint = (view*vec4(point, 1)).xyz;							\n\
		vNormal = (view*vec4(normal, 0)).xyz;						\n\
		gl_Position = persp*vec4(vPoint, 1);						\n\
		vUv = uv;													\n\
	}";

char *pixelShader = "\
    #version 130													\n\
	in vec3 vPoint;													\n\
	in vec3 vNormal;												\n\
	in vec2 vUv;													\n\
	out vec4 pColor;												\n\
	uniform vec3 light;												\n\
	uniform sampler2D textureImage;									\n\
	float PhongIntensity(vec3 pos, vec3 nrm) {						\n\
		vec3 N = normalize(nrm);           // surface normal		\n\
        vec3 L = normalize(light-pos);     // light vector			\n\
        vec3 E = normalize(pos);           // eye vector			\n\
        vec3 R = reflect(L, N);            // highlight vector		\n\
        float d = abs(dot(N, L));          // two-sided diffuse		\n\
        float s = abs(dot(R, E));          // two-sided specular	\n\
		return clamp(d+pow(s, 50), 0, 1);							\n\
	}																\n\
    void main() {													\n\
		float intensity = PhongIntensity(vPoint, vNormal);			\n\
		vec3 color = texture(textureImage, vUv).rgb;				\n\
		pColor = vec4(intensity*color, 1);							\n\
	}";

// Vertex Buffering

void InitVertexBuffer() {
    // create GPU buffer, make it the active buffer
    glGenBuffers(1, &vBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	// allocate and fill vertex buffer
	int nPts = points.size(), nNrms = normals.size(), nTex = textures.size();
	int sizePts = nPts*sizeof(vec3), sizeNrms = nNrms*sizeof(vec3), sizeTex = nTex*sizeof(vec2);
    glBufferData(GL_ARRAY_BUFFER, sizePts+sizeNrms+sizeTex, NULL, GL_STATIC_DRAW);
    // load data to sub-buffers
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &points[0]);
	if (sizeNrms)
		glBufferSubData(GL_ARRAY_BUFFER, sizePts, sizeNrms, &normals[0]);
	if (sizeTex)
		glBufferSubData(GL_ARRAY_BUFFER, sizePts+sizeNrms, sizeTex, &textures[0]);
}

// Texture

void InitTexture(const char *filename) {
	int width, height;
	char *pixels = ReadTexture(filename, width, height);
	if (pixels) {
		// allocate GPU texture buffer; copy, free pixels
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		// for multiple textures, see glActiveTexture
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // in case width not multiple of 4
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
		delete [] pixels;
		glGenerateMipmap(GL_TEXTURE_2D);
		// refer sampler uniform to texture 0 (default)
		GLSL::SetUniform(programId, "textureImage", 0);
	}
}

// Interactive Rotation

vec2 mouseDown;				// for each mouse down, need start point
vec2 rotOld, rotNew;	    // previous, current rotations

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	if (state == GLUT_DOWN)
		mouseDown = vec2((float) x, (float) y);
	if (state == GLUT_UP)
		rotOld = rotNew;
	glutPostRedisplay();
}

void MouseDrag(int x, int y) {
	y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	vec2 mouse((float) x, (float) y);
	rotNew = rotOld+.3f*(mouse-mouseDown); // old plus mouse distance from mouseDown
	glutPostRedisplay();
}

// Display

void Display() {
	// activate shader, vertex buffer
    glUseProgram(programId);
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	// update and send matrices to vertex shader
	mat4 view = Translate(0, 0, -5)*RotateY(rotNew.x)*RotateX(rotNew.y);
	GLSL::SetUniform(programId, "view", view);
	float fov = 15, nearPlane = -.001f, farPlane = -500;
	float aspect = (float) glutGet(GLUT_WINDOW_WIDTH) / (float) glutGet(GLUT_WINDOW_HEIGHT);
	mat4 persp = Perspective(fov, aspect, nearPlane, farPlane);
	GLSL::SetUniform(programId, "persp", persp);
	// transform light and send to pixel shader
	vec4 lite = view*vec4(lightSource, 1);
	GLSL::SetUniform(programId, "light", vec3(lite.x, lite.y, lite.z));
	// clear screen, enable transparency, use z-buffer
    glClearColor(.5f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
    // establish shader links
	int sizePts = points.size()*sizeof(vec3);
	GLSL::VertexAttribPointer(programId, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::VertexAttribPointer(programId, "normal", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizePts);
	GLSL::VertexAttribPointer(programId, "uv", 2, GL_FLOAT, GL_FALSE, 0, (void *) (2*sizePts));
	// draw triangles
	glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    glFlush();
}

// Application

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBufferId);
}

void main(int argc, char **argv) {
	// init window
    glutInit(&argc, argv);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Texture Example");
    glewInit();
	// build, use shaderId program
	programId = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	if (!programId)
		printf("can't link shader program\n");
	// read Alias/Wavefront "obj" formatted mesh file
	bool readOk = ReadAsciiObj(objFilename, points, triangles, &normals, &textures, NULL);
	if (!readOk)
		printf("failed to read %s\n", objFilename);
	if (!programId || !readOk) {
		getchar();
		return;
	}
	int nvrts = points.size(), nnrms = normals.size(), ntxts = textures.size();
	if (nvrts != nnrms || nvrts != ntxts)
		printf("error: %i vrts, %i nrms, %i txts\n", nvrts, nnrms, ntxts);
	printf("%i triangles\n", triangles.size());
	Normalize(points, .8f);
	// allocate vertex memory in the GPU, link it to the vertex shader
    InitVertexBuffer();
	// read texture image, create mipmap, link it to pixel shader
	InitTexture(texFilename);
	// GLUT callbacks, event loop
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
    glutCloseFunc(Close);
    glutMainLoop();
}
