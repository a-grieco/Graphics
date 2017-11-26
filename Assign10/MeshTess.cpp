// MeshTess-Stub.cpp: displacement-mapped mesh

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include "MeshIO.h"
#include "UI.h"

bool USE_TEXTURE = false;	// true: uses texture
							// false: adjusts color based on height

// mesh
vector<int3> triangles;
vector<vec3> points;
vector<vec3> normals;
vector<vec2> uvs;

// colors
vec3	 blk(0), wht(1), cyan(0,1,1);

// interactive view
vec2	 mouseDown, rotOld, rotNew;								// previous, current rotations
float	 dolly = -10;
mat4	 modelview, persp, fullview, screen;					// camera matrices

// selection
void   *picked = NULL, *hover = NULL;

// movable light
vec3	lightSource(-.2f, .4f, .8f);
Mover	lightMover(&lightSource);

// sliders
Slider	scl(30, 20, 70, -1, 1, 0, true, "scl", &wht);			// height scale

// shader indices
GLuint	shaderId = 0, vBufferId = 0, textureId = 0;				// valid if > 0

// vertex shader
char *vShaderCode = "\
	#version 400 core															\n\
	in vec3 point;																\n\
	in vec3 normal;																\n\
	in vec2 uv;																	\n\
	out vec3 vPoint;															\n\
	out vec3 vNormal;															\n\
	out vec2 vUv;																\n\
	void main()	{																\n\
		vPoint = point; 														\n\
		vNormal = normal;														\n\
		vUv = uv;																\n\
	}";

// tessellation evaluation - set vertex position and normal given triangle uvs
char *teShaderCode = "\
	#version 400 core															\n\
	float _PI = 3.141592;														\n\
	layout (triangles, fractional_odd_spacing, ccw) in;							\n\
	in vec3 vPoint[];															\n\
	in vec3 vNormal[];															\n\
	in vec2 vUv[];																\n\
	out vec3 tePoint;															\n\
	out vec3 teNormal;															\n\
	out vec2 teUv;																\n\
	out float teHeight;															\n\
	uniform float heightScale;													\n\
	uniform sampler2D heightField;												\n\
    uniform mat4 modelview;														\n\
	uniform mat4 persp;															\n\
	void main() {																\n\
		// send uv, point, normal to pixel shader								\n\
		vec2 t;																	\n\
		vec3 n, p;																\n\
		for (int i = 0; i < 3; i++) {											\n\
			float f = gl_TessCoord[i];											\n\
			p += f*vPoint[i];													\n\
			n += f*vNormal[i];													\n\
			t += f*vUv[i];														\n\
		}																		\n\
		teHeight = texture(heightField, t).r * heightScale;						\n\
		p += teHeight*normalize(n);												\n\
		tePoint = (modelview*vec4(p, 1)).xyz;									\n\
		gl_Position = persp*vec4(tePoint, 1);									\n\
		teNormal = (modelview*vec4(n, 0)).xyz;									\n\
		teUv = (modelview*vec4(t, 1, 1)).xy;									\n\
	}";

// pixel shader
char *pShaderCode = "\
    #version 400 core															\n\
	in vec3 tePoint;															\n\
	in vec3 teNormal;															\n\
	in vec2 teUv;																\n\
	in float teHeight;															\n\
	out vec4 pColor;															\n\
	uniform vec3 light;															\n\
	uniform bool useTexture;													\n\
	uniform sampler2D textureImage;												\n\
	vec3 color;																	\n\
	vec3 getColor(float height) {												\n\
		float r = 0, g = 0, b = 0;												\n\
		if(height < 0.0f) { height *= -1.0f; }									\n\
		height = mod(height, 1) * 10.0f;										\n\
		if(height > 3.0f) {														\n\
			float whiteness = height - 3.0;										\n\
			color = vec3(whiteness, whiteness, 1);								\n\
		} else if(height > 2.0f) {												\n\
			b += height/3.0f, g += 1 - b;										\n\
			color = vec3(r, g, b);												\n\
		} else if (height > 1.0f) {												\n\
			g += height/2.0f, r += 1 - g;										\n\
			color = vec3(r, g, b);												\n\
		} else {																\n\
			r += height;														\n\
			color = vec3(r, g, b);	// fades to black							\n\
		}																		\n\
		return color;															\n\
	}																			\n\
	void main() {																\n\
		// Phong shading														\n\
		vec3 N = normalize(teNormal);				// surface normal			\n\
        vec3 L = normalize(light-tePoint);			// light vector				\n\
        vec3 E = normalize(tePoint);				// eye vertex				\n\
        vec3 R = reflect(L, N);						// highlight vector			\n\
		if(useTexture) {														\n\
			color = texture(textureImage, teUv).rgb; // texture					\n\
		} else {																\n\
			color = getColor(teHeight);											\n\
		}																		\n\
		float dif = abs(dot(N, L));                 // one-sided diffuse		\n\
		float spec = pow(max(0, dot(E, R)), 50);								\n\
		float amb = .15, ad = clamp(amb+dif, 0, 1);								\n\
		pColor = vec4((ad+spec)*color, 1);										\n\
	}";

// Display

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
	// compute transformation matrices
	modelview = Translate(0, 0, dolly)*RotateY(rotNew.x)*RotateX(rotNew.y);
	float fov = 15, nearPlane = -.001f, farPlane = -500;
	float aspect = (float) glutGet(GLUT_WINDOW_WIDTH) / (float) glutGet(GLUT_WINDOW_HEIGHT);
	persp = Perspective(fov, aspect, nearPlane, farPlane);
	fullview = persp*modelview;
	screen = ScreenMode();
	// use tessellation shader
	glUseProgram(shaderId);
	GLSL::SetUniform(shaderId, "useTexture", USE_TEXTURE);	// sets whether to use texture (USE_TEXTURE = true) or
															// set color based on displacement on height (false)
	// set uniforms for height map and texture map
	GLSL::SetUniform(shaderId, "heightScale", scl.GetValue());
	GLSL::SetUniform(shaderId, "heightField", (int) textureId);
	// GLSL::SetUniform(shaderId, "textureImage", 0);	// replace white with texture
	// update matrices
	GLSL::SetUniform(shaderId, "modelview", modelview);
	GLSL::SetUniform(shaderId, "persp", persp);
	// transform light and send to fragment shader
	vec4 hLight = modelview*vec4(lightSource, 1);
	vec3 xlight(hLight.x, hLight.y, hLight.z);
	glUniform3fv(glGetUniformLocation(shaderId, "light"), 1, (float *) &xlight);
    // activate vertex buffer and establish shader links
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	int sizePts = points.size()*sizeof(vec3);
	GLSL::VertexAttribPointer(shaderId, "point",  3,  GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::VertexAttribPointer(shaderId, "normal", 3,  GL_FLOAT, GL_FALSE, 0, (void *) sizePts);
	GLSL::VertexAttribPointer(shaderId, "uv",     2,  GL_FLOAT, GL_FALSE, 0, (void *) (2*sizePts));
	// establish tessellating patch and display
	float r = 100, outerLevels[] = {r, r, r, r}, innerLevels[] = {r, r};
	glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
	glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
	glPatchParameteri(GL_PATCH_VERTICES, 3);
	glDrawElements(GL_PATCHES, 3*triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
	// draw sliders, light in 2D screen space
	UseDrawShader(screen);
	if (IsVisible(lightSource, fullview))
		Sun(ScreenPoint(lightSource, fullview), hover == &lightSource? &cyan : NULL);
	glDisable(GL_DEPTH_TEST);
	scl.Draw();
    glFlush();
}

// Input

void ReadObject(char *filename) {
	// read Alias/Wavefront "obj" formatted mesh file
	if (!ReadAsciiObj(filename, points, triangles, &normals, &uvs)) {
		printf("Failed to read %s\n", filename);
		return;
	}
	// scale/move model to uniform +/-1
	int npoints = points.size();
	int sizepts = npoints*sizeof(vec3), sizenrms = sizepts, sizeuvs = npoints*sizeof(vec2);
	printf("%i triangles\n", npoints/3);
	Normalize(points, .8f);
    // create GPU buffer, make it active, fill
    glGenBuffers(1, &vBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizepts+sizenrms+sizeuvs, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizepts, &points[0]);
	glBufferSubData(GL_ARRAY_BUFFER, sizepts, sizenrms, &normals[0]);
	glBufferSubData(GL_ARRAY_BUFFER, sizepts+sizenrms, sizeuvs, &uvs[0]);
}

// Interactive Rotation

void MouseOver(int x, int y) {
	int width = glutGet(GLUT_WINDOW_WIDTH), height = glutGet(GLUT_WINDOW_HEIGHT);
	void *wasHover = hover;
	hover = NULL;
	if (ScreenDistSq(x, height-y, lightSource, fullview) < 100)
		hover = (void *) lightSource;
	if (hover != wasHover)
		glutPostRedisplay();
}

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y; // invert y for upward-increasing screen space
    if (state == GLUT_UP) {
		if (picked == &rotOld)
			rotOld = rotNew;
	}
	picked = NULL;
	if (state == GLUT_DOWN) {
		if (ScreenDistSq(x, y, lightSource, fullview) < 100) {
			picked = &lightSource;
			lightMover.Down(x, y, modelview, &persp);
		}
		else if (scl.Hit(x, y))
			picked = &scl;
		else {
			picked = &rotOld;
			mouseDown = vec2((float) x, (float) y);
		}
	}
    glutPostRedisplay();
}

void MouseDrag(int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	if (picked == &lightSource)
		lightMover.Drag(x, y, modelview, &persp);
	else if (picked == &scl)
		scl.Mouse(x, y);
	else if (picked == &rotOld) {
		rotNew = rotOld+.3f*(vec2((float) x, (float) y)-mouseDown);
			// new rotations depend on old plus mouse distance from mouseDown
	}
    glutPostRedisplay();
}

void MouseWheel(int wheel, int direction, int x, int y) {
	dolly += (direction > 0? -.1f : .1f);
	glutPostRedisplay();
}

// Application

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBufferId);
	glDeleteBuffers(1, &textureId);
}

int MakeShaderProgram() {
	int vShader = GLSL::CompileShaderViaCode(vShaderCode, GL_VERTEX_SHADER);
	int pShader = GLSL::CompileShaderViaCode(pShaderCode, GL_FRAGMENT_SHADER);
	int teShader = GLSL::CompileShaderViaCode(teShaderCode, GL_TESS_EVALUATION_SHADER);
	int program = vShader && pShader && teShader? glCreateProgram() : 0, status;
	if (vShader && pShader && teShader) {
        glAttachShader(program, vShader);
        glAttachShader(program, pShader);
		glAttachShader(program, teShader);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
            GLSL::PrintProgramLog(program);
	}
	return program;
}

int Error(char *msg) {
	printf(msg);
	getchar();
	return 0;
}

int main(int argc, char **argv) {
	// init window
    glutInit(&argc, argv);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Shader Example");
    glewInit();
	// build, use shaderId program
	if (!(shaderId = MakeShaderProgram()))
		return Error("Can't link shader program\n");
	// read object and height map
	ReadObject("C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\Chair.obj");
	textureId = SetHeightfield("C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\metalcurves.tga");
	if (!triangles.size() || !textureId)
		return Error("Can't open file(s)\n");
	GLSL::SetUniform(shaderId, "textureImage", 0);	// replace white with texture
	// GLUT callbacks, event loop
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutMouseWheelFunc(MouseWheel);
	glutPassiveMotionFunc(MouseOver);
    glutCloseFunc(Close);
    glutMainLoop();
	return 0;
}
