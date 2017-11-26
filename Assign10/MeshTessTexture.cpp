// MeshTess.cpp: displacement mapped mesh

#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "GLSL.h"
#include "MeshIO.h"
#include "UI.h"

typedef VertexSTL Vertex;

// mesh
vector<Vertex> vertices;

// colors
vec3	blk(0), wht(1), cyan(0,1,1);

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
Slider	scl(30, 20, 70, -1, 1, 0, true, "scl", &wht);	// height scale

// shader indices
GLuint	shaderId = 0, vBufferId = 0, textureIds[2] = {0, 0};	// valid if > 0

// vertex shader (tessellation evaluation sets unit-sphere locations)
char *vShaderCode = "\
	#version 400 core															\n\
	in vec3 point;																\n\
	in vec3 normal;																\n\
	in vec2 uv;																	\n\
	out VS_OUT { // struct needed for tess eval									\n\
		vec3 point;																\n\
		vec3 normal;															\n\
		vec2 uv;																\n\
	} vs_out;																	\n\
	void main()	{																\n\
		vs_out.point = point; 													\n\
		vs_out.normal = normal;													\n\
		vs_out.uv = uv;															\n\
	}";

// tessellation evaluation - set vertex position, normal, and st parameters
char *teShaderCode = "\
	#version 400 core															\n\
	float _PI = 3.141592;														\n\
	layout (triangles, fractional_odd_spacing, ccw) in;							\n\
	in VS_OUT {																	\n\
		vec3 point;																\n\
		vec3 normal;															\n\
		vec2 uv;																\n\
	} tes_in[];																	\n\
	out TES_OUT {																\n\
		vec3 point;																\n\
		vec3 normal;															\n\
		vec2 uv;																\n\
	} tes_out;																	\n\
	uniform sampler2D heightField;												\n\
	uniform float heightScale;													\n\
    uniform mat4 modelview;														\n\
	uniform mat4 persp;															\n\
	void main() {																\n\
		// send uv, point, normal to pixel shader								\n\
		vec2 t;																	\n\
		vec3 p, n;																\n\
		for (int i = 0; i < 3; i++) {											\n\
			float f = gl_TessCoord[i];											\n\
			p += f*tes_in[i].point;												\n\
			n += f*tes_in[i].normal;											\n\
			t += f*tes_in[i].uv;												\n\
		}																		\n\
		normalize(n);															\n\
		float height = heightScale*texture(heightField, t).z;					\n\
		p += height*n;															\n\
		vec4 hPosition = modelview*vec4(p, 1);									\n\
		gl_Position = persp*hPosition;											\n\
		tes_out.point = hPosition.xyz;											\n\
		tes_out.normal = (modelview*vec4(n, 0)).xyz;							\n\
		tes_out.uv = t;															\n\
	}";

// pixel shader
char *pShaderCode = "\
    #version 400 core															\n\
	in TES_OUT {																\n\
		vec3 point;																\n\
		vec3 normal;															\n\
		vec2 uv;																\n\
	} ps_in;																	\n\
	out vec4 pColor;															\n\
	uniform sampler2D textureImage;												\n\
	uniform vec3 light;															\n\
	uniform vec4 color = vec4(1, 1, 1, 1);			// default white			\n\
    void main() {																\n\
		// Phong shading with texture											\n\
		vec3 N = normalize(ps_in.normal);			// surface normal			\n\
        vec3 L = normalize(light-ps_in.point);		// light vector				\n\
        vec3 E = normalize(ps_in.point);			// eye vertex				\n\
        vec3 R = reflect(L, N);						// highlight vector			\n\
		float dif = abs(dot(N, L));                 // one-sided diffuse		\n\
		float spec = pow(max(0, dot(E, R)), 50);								\n\
		float amb = .15, ad = clamp(amb+dif, 0, 1);								\n\
		vec4 texColor = texture(textureImage, ps_in.uv);						\n\
		pColor = vec4(ad*texColor.rgb+spec*color.rgb, 1);						\n\
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
	mat4 rotM = RotateY(rotNew.x)*RotateX(rotNew.y);
		// rot.x is about Y-axis, rot.y is about X-axis
	modelview = Translate(0, 0, dolly)*rotM;
	float fov = 15, nearPlane = -.001f, farPlane = -500;
	float aspect = (float) glutGet(GLUT_WINDOW_WIDTH) / (float) glutGet(GLUT_WINDOW_HEIGHT);
	persp = Perspective(fov, aspect, nearPlane, farPlane);
	fullview = persp*modelview;
	screen = ScreenMode();
	// use tessellation shader
	glUseProgram(shaderId);
	// set uniforms for height map and texture map
	GLSL::SetUniform(shaderId, "heightScale", scl.GetValue());
	GLSL::SetUniform(shaderId, "heightField", (int) textureIds[1]);
	GLSL::SetUniform(shaderId, "textureImage", (int) textureIds[0]);
	// update matrices
	GLSL::SetUniform(shaderId, "modelview", modelview);
	GLSL::SetUniform(shaderId, "persp", persp);
	// transform light and send to fragment shader
	vec4 hLight = modelview*vec4(lightSource, 1);
	vec3 xlight(hLight.x, hLight.y, hLight.z);
	glUniform3fv(glGetUniformLocation(shaderId, "light"), 1, (float *) &xlight);
    // activate vertex buffer and establish shader links
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	GLSL::VertexAttribPointer(shaderId, "point",  3,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
	GLSL::VertexAttribPointer(shaderId, "normal", 3,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
	GLSL::VertexAttribPointer(shaderId, "uv",     2,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (2*sizeof(vec3)));
    //									 attrib   num type      normalize stride          offset
	// establish tessellating patch and display
	glPatchParameteri(GL_PATCH_VERTICES, 3);
	float r = 100, outerLevels[] = {r, r, r, r}, innerLevels[] = {r, r};
	glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
	glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
	glDrawArrays(GL_PATCHES, 0, vertices.size());
	// draw sliders, light in 2D screen space
	UseDrawShader(screen);
	if (IsVisible(lightSource, fullview))
		Sun(ScreenPoint(lightSource, fullview), hover == &lightSource? &cyan : NULL);
	glDisable(GL_DEPTH_TEST);
	scl.Draw();
    glFlush();
}

// Texture and Height Maps

char *ReadTarga(string filename, int &width, int &height) {
	// open targa file, read header, return pointer to pixels
	FILE *in = fopen(filename.c_str(), "rb");
	if (in) {
		short tgaHeader[9];
		fread(tgaHeader, sizeof(tgaHeader), 1, in);
		// allocate, read pixels
		int w = width = tgaHeader[6], h = height = tgaHeader[7];
		int bitsPerPixel = tgaHeader[8], bytesPerPixel = bitsPerPixel/8, bytesPerImage = w*h*bytesPerPixel;
		if (bytesPerPixel != 3) {
			printf("bytes per pixel not 3!\n");
			return NULL;
		}
		char *pixels = new char[bytesPerImage];
		fread(pixels, bytesPerImage, 1, in);
		return pixels;
	}
	printf("can't open %s\n", filename);
	return NULL;
}

void SetTexture(string filename) {
	// open targa file, read header, store as textureIds[0]
	int width, height;
	char *pixels = ReadTarga(filename.c_str(), width, height);
	// set and bind active texture corresponding with textureIds[0]
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureIds[0]);	// 1
	// allocate GPU texture buffer; copy, free pixels
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);			// accommodate width not multiple of 4
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
	delete [] pixels;
	glGenerateMipmap(GL_TEXTURE_2D);
}

void SetHeightfield(string filename) {
	// open targa file, read header, store as GL_TEXTURE2
	int width, height;
	char *pixels = ReadTarga(filename.c_str(), width, height);
	// convert to luminance
	for (int i = 0; i < width*height; i++) {
		char *p = pixels+3*i;
		p[0] = p[1] = p[2] = (int) (.21*(double)p[2]+.72*(double)p[1]+.07*(double)p[0]);
	}
	// set and bind active texture corresponding with textureIds[1]
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textureIds[1]); // 2
	// allocate GPU texture buffer; copy, free pixels
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // in case width not multiple of 4
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
	delete [] pixels;
	glGenerateMipmap(GL_TEXTURE_2D);
}

// Input

void ReadObject(char *filename) {
	vertices.resize(0);
	// read Alias/Wavefront "obj" formatted mesh file
	if (!ReadAsciiObj(filename, vertices)) {
		printf("Failed to read %s\n", filename);
		return;
	}
	// scale/move model to uniform +/-1
	int nvrts = vertices.size();
	vector<vec3> points(nvrts);
	printf("%i triangles\n", nvrts/3);
	for (int i = 0; i < nvrts; i++)
		points[i] = vertices[i].point;
	Normalize(points, .8f);
	for (int i = 0; i < nvrts; i++)
		vertices[i].point = points[i];
    // create GPU buffer, make it active, fill
    glGenBuffers(1, &vBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
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
	glDeleteBuffers(2, textureIds);
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

void main(int argc, char **argv) {
	// init window
    glutInit(&argc, argv);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Shader Example");
    glewInit();
	// build, use shaderId program
	if (!(shaderId = MakeShaderProgram())) {
		printf("Can't link shader program\n");
		getchar();
		return;
	}
	ReadObject("C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\teacup.obj");
	// init texture and height maps
	glGenTextures(2, textureIds);
	SetTexture("C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\turquoise.tga");
	SetHeightfield("C:\\Users\\amgrieco\\Dropbox\\Graphics\\Checkerboard2\\heightmap.tga");
	// GLUT callbacks, event loop
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutMouseWheelFunc(MouseWheel);
	glutPassiveMotionFunc(MouseOver);
    glutCloseFunc(Close);
    glutMainLoop();
}
