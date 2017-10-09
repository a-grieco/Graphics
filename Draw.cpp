/* ================================================
   DrawX.cpp - basic draw and related operations.  
   Authored by Jules Bloomenthal.
   Copyright (c) Unchained Geometry, Seattle, 2014.
   All rights reserved.
   ================================================ */

#include <gl/glew.h>
#include <gl/freeglut.h>
#include <assert.h>
#include "Draw.h"
#include "GLSL.h"

// Support

#define FormatString(buffer, maxBufferSize, format) {  \
    if (format) {                                      \
        va_list ap;                                    \
        va_start(ap, format);                          \
        _vsnprintf(buffer, maxBufferSize, format, ap); \
        va_end(ap);                                    \
    }                                                  \
    else                                               \
        (buffer)[0] = 0;                               \
}

// Errors

int Errors(char *buf) {
    int nErrors = 0;
    buf[0] = 0;
    for (;;) {
        GLenum n = glGetError();
        if (n == GL_NO_ERROR)
            break;
        sprintf(buf+strlen(buf), "%s%s", !nErrors++? "" : ", ", gluErrorString(n));
    }
    return nErrors;
}

void  CheckGL_Errors(char *msg) {
	char buf[100];
	if (msg)
		printf("%s: ", msg);
	if (Errors(buf))
		printf("GL error(s): %s\n", buf);
	else
		printf("<no GL errors>\n");
}

// Screen Mode

mat4 ScreenMode() { return ScreenMode(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT)); }

mat4 ScreenMode(int width, int height) {
	mat4 scale = Scale(2.f / (float) width, 2.f / (float) height, 1.);
	mat4 tran = Translate(-1, -1, 0);
	return tran*scale;
}

bool IsVisible(vec3 &p, mat4 &fullview, vec2 *screenA) {
	float winWidth = (float) glutGet(GLUT_WINDOW_WIDTH), winHeight = (float) glutGet(GLUT_WINDOW_HEIGHT);
	vec4 xp = fullview*vec4(p, 1);
	vec2 clip(xp.x/xp.w, xp.y/xp.w);	// clip space, +/1
	vec2 screen(((float) winWidth/2.f)*(1.f+clip.x), ((float) winHeight/2.f)*(1.f+clip.y));
	if (screenA)
		*screenA = screen;
	float z = xp.z/xp.w, zScreen;
	glReadPixels((int)screen.x, (int)screen.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &zScreen);
	zScreen = 2*zScreen-1; // seems to work (clip range +/-1 but zbuffer range 0-1)
	return z < zScreen;
}

void ScreenPoint(vec3 p, mat4 m, float &xscreen, float &yscreen, float *zscreen) {
	int width = glutGet(GLUT_WINDOW_WIDTH), height = glutGet(GLUT_WINDOW_HEIGHT);
	vec4 xp = m*vec4(p, 1);
 	xscreen = ((xp.x/xp.w)+1)*.5f*(float)width;
	yscreen = ((xp.y/xp.w)+1)*.5f*(float)height;
	if (zscreen)
		*zscreen = xp.z; // /xp.w;
}

void NDCPoint(vec3 p, mat4 m, float &xscreen, float &yscreen, float *zscreen) {
	int width = glutGet(GLUT_WINDOW_WIDTH), height = glutGet(GLUT_WINDOW_HEIGHT);
	vec4 xp = m*vec4(p, 1);
 	xscreen = ((xp.x/xp.w)+1)*.5f*(float)width;
	yscreen = ((xp.y/xp.w)+1)*.5f*(float)height;
	if (zscreen)
		*zscreen = xp.z; // /xp.w;
}

vec2 ScreenPoint(vec3 p, mat4 m, float *zscreen) {
	vec2 ret;
	ScreenPoint(p, m, ret.x, ret.y, zscreen);
	return ret;
}

double ScreenZ(vec3 &p, mat4 &m) {
	vec4 xp = m*vec4(p, 1);
	return xp.z;
}

static bool Nil(float d) { return d < FLT_EPSILON && d > -FLT_EPSILON; };

vec3 Ortho(vec3 &v) {                       // perpendicular vector of the same length
	float len = length(v);
	if (len < FLT_EPSILON)
		return vec3();
	// compute a vector that is non-colinear with (x,y,z)
	vec3 crosser = !Nil(v.x)? vec3(v.z,-v.x,v.y) : !Nil(v.y)? vec3(v.z,v.x,-v.y) : vec3(-v.z,v.y,v.x);
	vec3 ortho = cross(v, crosser);
	float olen = length(ortho);
	return (len/olen)*ortho;
}

vec3 ProjectToLine(vec3 &p, vec3 &p1, vec3 &p2) {
	// project p to line p1p2
	vec3 delta(p2-p1);
	if (Nil(delta.x) && Nil(delta.y) && Nil(delta.z))
		return p1;
	vec3 dif(p-p1);
	float alpha = dot(delta, dif)/dot(delta, delta);
	return p1+alpha*delta;
}

bool FrontFacing(vec3 &base, vec3 &vec, mat4 &view) { return ScreenZ(base, view) >= ScreenZ(base+vec, view); }

float ScreenDistSq(int x, int y, vec3 p, mat4 m, float *zscreen) {
	vec4 xp = m*vec4(p, 1);
 	float xscreen = ((xp.x/xp.w)+1)*.5f*(float) glutGet(GLUT_WINDOW_WIDTH);
	float yscreen = ((xp.y/xp.w)+1)*.5f*(float) glutGet(GLUT_WINDOW_HEIGHT);
	if (zscreen)
		*zscreen = xp.z;//xp.w;
	float dx = x-xscreen, dy = y-yscreen;
    return dx*dx+dy*dy;
}

float ScreenDistSq(vec3 &p1, vec3 &p2, mat4 &view) {
	vec2 screen1 = ScreenPoint(p1, view), screen2 = ScreenPoint(p2, view), dif(screen2-screen1);
	return dot(dif, dif);
}

void ScreenLine(float xscreen, float yscreen, mat4 &modelview, mat4 &persp, float p1[], float p2[]) {
    // compute 3D world space line, given by p1 and p2, that transforms
    // to a line perpendicular to the screen at (xscreen, yscreen)
	int vp[4];
	double tpersp[4][4], tmodelview[4][4], a[3], b[3];
	// get viewport
	glGetIntegerv(GL_VIEWPORT, vp);
	// create transposes
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++) {
			tmodelview[i][j] = modelview[j][i];
			tpersp[i][j] = persp[j][i];
		}
	if (gluUnProject(xscreen, yscreen, .25, (const double*) tmodelview, (const double*) tpersp, vp, &a[0], &a[1], &a[2]) == GL_FALSE)
        printf("UnProject false\n");
	if (gluUnProject(xscreen, yscreen, .50, (const double*) tmodelview, (const double*) tpersp, vp, &b[0], &b[1], &b[2]) == GL_FALSE)
        printf("UnProject false\n");
	for (int i = 0; i < 3; i++) {
		p1[i] = static_cast<float>(a[i]);
		p2[i] = static_cast<float>(b[i]);
	}
}

vec3 ScreenVec(float xscreen, float yscreen, mat4 &modelview, mat4 &persp) {
	float p1[3], p2[3];
	ScreenLine(xscreen, yscreen, modelview, persp, p1, p2);
	vec3 pp1(p1[0], p1[1], p1[2]), pp2(p2[0], p2[1], p2[2]);
	return normalize(pp2-pp1);
}

// Draw Shader

int drawShader = 0;

char *drawVShader = "\
	#version 400								\n\
	// layout (location = 0) in vec3 position;	\n\
	// layout (location = 1) in vec3 color;		\n\
	in vec3 position;							\n\
	in vec3 color;								\n\
	out vec3 vColor;							\n\
    uniform mat4 view; // persp*modelView       \n\
	void main()									\n\
	{											\n\
		gl_Position = view*vec4(position, 1);	\n\
		vColor = color;							\n\
	}											\n";

char *drawFShader = "\
	#version 400								\n\
	uniform float opacity = 1;					\n\
	in vec3 vColor;								\n\
	out vec4 fColor;							\n\
	void main()									\n\
	{											\n\
	    fColor = vec4(vColor, opacity);			\n\
	}											\n";

int UseDrawShader() {
	int current = GLSL::CurrentShader();
	if (!drawShader)
		drawShader = InitShader(drawVShader, drawFShader);
	if (current != drawShader)
		glUseProgram(drawShader);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	return current;
}

int UseDrawShader(mat4 viewMatrix) {
	int r = UseDrawShader();
	GLSL::SetUniform(drawShader, "view", viewMatrix);
	return r;
}

class DrawBuffer {
public:
	DrawBuffer() { vBuf = 0; }
	~DrawBuffer() { } // if (vBuf > 0) glDeleteBuffers(1, &vBuf);
	GLuint vBuf;
} dotBuffer, lineBuffer, triBuffer, quadBuffer;

// Display

void SetOpacity(float opacity) { GLSL::SetUniform(drawShader, "opacity", opacity); }

// Lines

void Box(vec3 &min, vec3 &max, vec3 &color) {
	// min corresponds with left/bottom/near, max corresponds with right/top/far
	// 4 lines from left to right
	Line(vec3(min.x, min.y, min.z), vec3(max.x, min.y, min.z), color); // bottom/near
	Line(vec3(min.x, min.y, max.z), vec3(max.x, min.y, max.z), color); // bottom/far
	Line(vec3(min.x, max.y, min.z), vec3(max.x, max.y, min.z), color); // top/near
	Line(vec3(min.x, max.y, max.z), vec3(max.x, max.y, max.z), color); // top/far
	// 4 lines from bottom to top
	Line(vec3(min.x, min.y, min.z), vec3(min.x, max.y, min.z), color); // left/near
	Line(vec3(min.x, min.y, max.z), vec3(min.x, max.y, max.z), color); // left/far
	Line(vec3(max.x, min.y, min.z), vec3(max.x, max.y, min.z), color); // right/near
	Line(vec3(max.x, min.y, max.z), vec3(max.x, max.y, max.z), color); // right/far
	// 4 lines from near to far
	Line(vec3(min.x, min.y, min.z), vec3(min.x, min.y, max.z), color); // left/bottom
	Line(vec3(min.x, max.y, min.z), vec3(min.x, max.y, max.z), color); // left/top
	Line(vec3(max.x, min.y, min.z), vec3(max.x, min.y, max.z), color); // right/bottom
	Line(vec3(max.x, max.y, min.z), vec3(max.x, max.y, max.z), color); // right/top
}

void Stipple(int factor, int a,int b,int c,int d,int e,int f,int g,int h,
                         int i,int j,int k,int l,int m,int n,int o,int p) {
    GLushort pattern = 0;
    int bits[16] = {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p};
    for (int index = 0; index < 16; index++) {
        int bit = bits[index];
        if (bit == -1)
            break;
        pattern |= 1<<bit;
    }
    glLineStipple(factor, pattern);
}

bool DashOn(int factor, int off) {
	GLboolean on = glIsEnabled(GL_LINE_STIPPLE);
	glEnable(GL_LINE_STIPPLE);
	Stipple(factor, (0+off)%16, (1+off)%16, (2+off)%16, (3+off)%16, (8+off)%16, (9+off)%16, (10+off)%16, (11+off)%16);
	return on != 0;
}

bool DotOn(int factor, int off) {
	GLboolean on = glIsEnabled(GL_LINE_STIPPLE);
	glEnable(GL_LINE_STIPPLE);
	Stipple(factor, (0+off)%16, (1+off)%16, (4+off)%16, (5+off)%16, (8+off)%16, (9+off)%16, (12+off)%16, (13+off)%16);
	return on != 0;
}

void DashOff() { glDisable(GL_LINE_STIPPLE); }

void DotOff() { glDisable(GL_LINE_STIPPLE); }

void Line(float *pnt1, float *pnt2, float *col1, float *col2, float opacity) {
///	int current = UseDrawShader();
	// vertex data
	float points[][3] = {{pnt1[0], pnt1[1], pnt1[2]}, {pnt2[0], pnt2[1], pnt2[2]}},
		  colors[][3] = {{col1[0], col1[1], col1[2]}, {col2[0], col2[1], col2[2]}};
 	// create a vertex buffer for the array
	if (lineBuffer.vBuf <= 0) {
		glGenBuffers(1, &lineBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, lineBuffer.vBuf);
		int bufferSize = sizeof(points)+sizeof(colors);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
	if (quadBuffer.vBuf <= 0) {
		glGenBuffers(1, &quadBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, quadBuffer.vBuf);
		// allocate buffer memory and load location and color data
		int bufferSize = sizeof(points)+sizeof(colors);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
    // set active vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, lineBuffer.vBuf);
    // allocate buffer memory and load location and color data
	glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
    // connect shader inputs
	GLSL::VertexAttribPointer(drawShader, "position", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    GLSL::VertexAttribPointer(drawShader, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
	GLSL::SetUniform(drawShader, "opacity", opacity);
	// draw
	glDrawArrays(GL_LINES, 0, 2);
	// cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
///	glUseProgram(current);
}

void Line(vec3 &p1, vec3 &p2, vec3 &col, float opacity, float width, bool dashed, bool dotted) {
	bool was = dashed? DashOn() : dotted? DotOn() : false;
	dashed? DashOn() : void();
	dotted? DotOn() : void();
	float w;
	glGetFloatv(GL_LINE_WIDTH, &w);
	glLineWidth(width);
	Line(p1, p2, col, col, opacity);
	if (!was && (dashed || dotted))
		DashOff();
	glLineWidth(w);
}

void Line(vec3 &p1, vec3 &p2, vec3 &col1, vec3 &col2, float opacity) {
	Line((float *) &p1.x, (float *) &p2.x, (float *) &col1.x, (float *) &col2, opacity);
}

void Line(float x1, float y1, float x2, float y2, float *col1, float *col2, float opacity) {
	float p1[] = {x1, y1, 0}, p2[] = {x2, y2, 0};
	Line(p1, p2, col1, col2, opacity);
}

void Line(int x1, int y1, int x2, int y2, float *col1, float *col2, float opacity) {
	float p1[] = {(float) x1, (float) y1, 0};
	float p2[] = {(float) x2, (float) y2, 0};
	Line(p1, p2, col1, col2, opacity);
}

// Disks

void Disk(vec3 &p, float diameter, vec3 &col, mat4 &fullview, char *text, ...) {
    char buf[500];
    FormatString(buf, 500, text);
	Text(p, fullview, vec3(0), buf);
	Disk(p, diameter, col);
}

void Disk(float *point, float diameter, float *color, float opacity) {
	UseDrawShader();
	// create a vertex buffer
	if (dotBuffer.vBuf <= 0) {
		glGenBuffers(1, &dotBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, dotBuffer.vBuf);
		int bufferSize = 6*sizeof(float);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
	// set active buffer
	glBindBuffer(GL_ARRAY_BUFFER, dotBuffer.vBuf);
    // allocate buffer memory and load location and color data
	// **** write directly via glMap?
	glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 3*sizeof(float), point);
	glBufferSubData(GL_ARRAY_BUFFER, 3*sizeof(float), 3*sizeof(float), color);
	// connect shader inputs
	GLSL::VertexAttribPointer(drawShader, "position", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::VertexAttribPointer(drawShader, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(vec3));
	// using layouts:
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);				// position
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(vec3));	// color
	// draw, cleanup
	SetOpacity(opacity);
	glPointSize(diameter);
	glDrawArrays(GL_POINTS, 0, 1);
}

void Disk(vec3 &p, float diameter, vec3 &color, float opacity) {
	Disk((float *) &p.x, diameter, color, opacity);
}

void Disk(vec2 &p, float diameter, vec3 &color, float opacity) {
	float pp[] = {p.x, p.y, 0};
	Disk(pp, diameter, color, opacity);
}

void Disk(int x, int y, float diameter, float *color, float opacity) {
	float p[] = {(float) x, (float) y, 0};
	Disk(p, diameter, color, opacity);
}

void DiskRing(vec3 &p, float outDia, vec3 &outColor, float inDia, vec3 &inColor, float opacity) {
	Disk(p, outDia, outColor, opacity);
	Disk(p, inDia, inColor, opacity);
}

void DiskRing(vec2 &p, float outDia, vec3 &outColor, float inDia, vec3 &inColor, float opacity) {
	Disk(p, outDia, outColor, opacity);
	Disk(p, inDia, inColor, opacity);
}

// Circles

#define N_CIRCLE_POINTS 12
vec2 circle[N_CIRCLE_POINTS];

static bool SetCircle() {
    for (int i = 0; i < N_CIRCLE_POINTS; i++) {
        double angle = 2*3.141592*(double)i/N_CIRCLE_POINTS;
        circle[i] = vec2((GLfloat) cos(angle), (GLfloat) sin(angle));
    }
    return true;
}
static bool circleSet = SetCircle();

void Circle(vec2 &p, float dia, vec3 &color) {
    float radius = dia/2;
	vec2 p1 = p+radius*circle[N_CIRCLE_POINTS-1];
    for (int i = 0; i < N_CIRCLE_POINTS; i++) {
		vec2 p2 = p+radius*circle[i];
	//	Line(p1, p2, color, color); fails
		Line(p1.x, p1.y, p2.x, p2.y, color, color);
		p1 = p2;
    }
}

void Circle(vec3 &p, mat4 &m, float dia, vec3 &color, char *msg) {
	vec2 center = ScreenPoint(p, m);
	mat4 screen = ScreenMode();
	UseDrawShader(screen);
	Circle(center, dia, color);
	UseDrawShader(m);
	msg? Text(p, m, color, msg) : void();
}

void Circle(vec3 &p, vec3 &n, float rad, vec3 &color, bool dots, float lineWidth, bool dashed) {
	vec3 pts[N_CIRCLE_POINTS];
	float xa = abs(n.x), ya = abs(n.y), za = abs(n.z);
	vec3 xaxis(1,0,0), yaxis(0,1,0), zaxis(0,0,1);
	vec3 crosser = xa < ya? (xa < za? xaxis : zaxis) : (ya < za? yaxis : zaxis);
    vec3 ortho = cross(n, crosser);
	vec3 v1 = normalize(ortho), v2 = normalize(cross(ortho, n));
	bool alreadyDashed = dashed? DashOn() : false;
	if (dashed)
		DashOn();
	for (int i = 0; i < N_CIRCLE_POINTS; i++) {
		vec2 &c = rad*circle[i];
		pts[i] = p+c[0]*v1+c[1]*v2;
	}
	for (int i = 0; i < N_CIRCLE_POINTS; i++) {
		Line(pts[i], pts[(i+1)%N_CIRCLE_POINTS], color, 1, lineWidth, dashed);
		if (dots)
			Disk(pts[i], 5, vec3(1,0,0));
	}
	if (!alreadyDashed && dashed)
		DashOff();
}

void Circle(vec3 &base, float diameter, mat4 &modelview, mat4 &persp, vec3 &color) {
	mat4 view = persp*modelview;
	vec2 screenBase = ScreenPoint(base, view);
	vec3 circN = ScreenVec(screenBase.x, screenBase.y, modelview, persp);
	vec3 circO = Ortho(circN), tipO = base+diameter*circO;
	float rad = sqrt(ScreenDistSq(base, tipO, view));
	DashOn();
	Circle(base, view, 2*rad, color);
}

void DrawSphere(vec3 &p, float rad, vec3 &color) {
	static float sqrt2 = (float) sqrt(2.), sqrt5 = (float) sqrt(5.);
	vec3 xyNormals[] = {vec3( 1,0,0), vec3( sqrt2, sqrt2,0), vec3(0, 1,0), vec3(-sqrt2, sqrt2,0),
						vec3(-1,0,0), vec3(-sqrt2,-sqrt2,0), vec3(0,-1,0), vec3( sqrt2,-sqrt2,0)};
	for (int i = 0; i < 8; i++)
		Circle(p, xyNormals[i], rad, color, false);
	Circle(p, vec3(0,0,1), rad, color, false);
	Circle(p+vec3(0,0,rad/3.f), vec3(0,0,1), (2/3.f)*sqrt2*rad, color, false);
	Circle(p-vec3(0,0,rad/3.f), vec3(0,0,1), (2/3.f)*sqrt2*rad, color, false);
	Circle(p+vec3(0,0,2*rad/3.f), vec3(0,0,1), (sqrt5/3.f)*rad, color, false);
	Circle(p-vec3(0,0,2*rad/3.f), vec3(0,0,1), (sqrt5/3.f)*rad, color, false);
}

// Arrows

void Arrow(vec2 &base, vec2 &head, vec3 &col, char *label, double headSize) {
	Line(base.x, base.y, head.x, head.y, col, col);
    if (headSize > 0) {
	    vec2 v1 = (float)headSize*normalize(head-base), v2(v1.y/2.f, -v1.x/2.f);
		vec2 head1(head-v1+v2), head2(head-v1-v2);
        Line(head.x, head.y, head1.x, head1.y, col, col);
        Line(head.x, head.y, head2.x, head2.y, col, col);
    }
    if (label)
        Text((int) head.x+5, (int) head.y, col, label);
}

void ArrowV(vec3 &base, vec3 &vec, mat4 &m, vec3 &col, char *label, double headSize) {
	vec2 base2 = ScreenPoint(base, m), head2 = ScreenPoint(base+vec, m);
	mat4 screen = ScreenMode();
	UseDrawShader(screen);
	Arrow(base2, head2, col, label, headSize);
	UseDrawShader(m);
}

// Triangles

vec3 TriangleShrink(vec3 &p1, vec3 &p2, vec3 &p3, float scaleAboutCenter) {
	vec3 cen = (p1+p2+p3)/3.f;
	p1 = cen+scaleAboutCenter*(p1-cen);
	p2 = cen+scaleAboutCenter*(p2-cen);
	p3 = cen+scaleAboutCenter*(p3-cen);
	return cen;
}

void TriangleLines(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &col, float opacity) {
	Line(p1, p2, col, col, opacity);
	Line(p2, p3, col, col, opacity);
	Line(p3, p1, col, col, opacity);
}

void TriangleLinesScale(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &col, float scaleAboutCenter, float opacity) {
	vec3 p[] = {p1, p2, p3};
	TriangleShrink(p[0], p[1], p[2], scaleAboutCenter);
	TriangleLines(p[0], p[1], p[2], col, opacity);
}

void TriangleShade(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &color, float opacity) {
	Triangle(p1, p2, p3, color, color, color, opacity);
}

void Triangle(vec3 &pnt1, vec3 &pnt2, vec3 &pnt3, vec3 &col1, vec3 &col2, vec3 &col3, float opacity) {
	int current = GLSL::CurrentShader();
//	if (current != drawShader)
	UseDrawShader();
	// align vertex data
	float points[][3] = {pnt1.x,  pnt1.y,  pnt1.z,  pnt2.x,  pnt2.y,  pnt2.z,  pnt3.x,  pnt3.y,  pnt3.z};
	float colors[][3] = {col1[0], col1[1], col1[2], col2[0], col2[1], col2[2], col3[0], col3[1], col3[2]};
 	// create vertex buffer
	if (lineBuffer.vBuf < 0) {
		glGenBuffers(1, &triBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, triBuffer.vBuf);
		int bufferSize = sizeof(points)+sizeof(colors);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
    glBindBuffer(GL_ARRAY_BUFFER, triBuffer.vBuf);
    // allocate buffer memory and load location and color data
	glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
    // connect shader inputs
	GLSL::VertexAttribPointer(drawShader, "position", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    GLSL::VertexAttribPointer(drawShader, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
	GLSL::SetUniform(drawShader, "opacity", opacity);
	// draw, cleanup
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	if (current != drawShader)
	glUseProgram(current);
}

// Quads

void Quad(vec3 &pnt1, vec3 &pnt2, vec3 &pnt3, vec3 &pnt4, float *col, float opacity) {
	int current = UseDrawShader();
	// align vertex data
	float points[][3] = {pnt1.x, pnt1.y, pnt1.z, pnt2.x, pnt2.y, pnt2.z, pnt3.x, pnt3.y, pnt3.z, pnt4.x, pnt4.y, pnt4.z};
	float colors[][3] = {col[0], col[1], col[2], col[0], col[1], col[2], col[0], col[1], col[2], col[0], col[1], col[2]};
 	// create vertex buffer
	if (lineBuffer.vBuf < 0) {
		glGenBuffers(1, &lineBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, lineBuffer.vBuf);
		int bufferSize = sizeof(points)+sizeof(colors);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
	if (quadBuffer.vBuf <= 0) {
		glGenBuffers(1, &quadBuffer.vBuf);
		glBindBuffer(GL_ARRAY_BUFFER, quadBuffer.vBuf);
		int bufferSize = sizeof(points)+sizeof(colors);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	}
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer.vBuf);
    // allocate buffer memory and load location and color data
	glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
    // connect shader inputs
	GLSL::VertexAttribPointer(drawShader, "position", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    GLSL::VertexAttribPointer(drawShader, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
	GLSL::SetUniform(drawShader, "opacity", opacity);
	// draw, cleanup
	glDrawArrays(GL_QUADS, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (true) return;
	glUseProgram(current);
}

void QuadLines(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, float *col, float opacity) {
	Line(p1, p2, col, col, opacity);
	Line(p2, p3, col, col, opacity);
	Line(p3, p4, col, col, opacity);
	Line(p4, p1, col, col, opacity);
}

// Text

void *font = GLUT_BITMAP_9_BY_15;
	//  GLUT_STROKE_ROMAN unsupported
	//  GLUT_STROKE_MONO_ROMAN unsupported
	//  GLUT_BITMAP_9_BY_15
	//  GLUT_BITMAP_8_BY_13
	//  GLUT_BITMAP_TIMES_ROMAN_10
	//  GLUT_BITMAP_TIMES_ROMAN_24
	//  GLUT_BITMAP_HELVETICA_10
	//  GLUT_BITMAP_HELVETICA_12
	//  GLUT_BITMAP_HELVETICA_18

void *SetFont(void *f) {
	void *save = font;
	font = f;
	return save;
}

void *GetFont() { return font; }

void *SetBold() { return SetFont(GLUT_BITMAP_HELVETICA_18); }

void PutString(int x, int y, const char *text, vec3 &color, void *f) {
	void *save = GetFont();
	if (f && FontSize(f) > 0)
		SetFont(f);
	assert(FontSize(font) >= 0);
	int program = GLSL::CurrentShader();
	int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
	glUseProgram(0); // no text support in GLSL
	float xf = (float) (2*x)/w-1, yf = (float) (2*y)/h-1;
	glColor3fv(&color.x);
	glRasterPos2f(xf, yf);
	glutBitmapString(font, (unsigned char*) text);
	glUseProgram(program);
	SetFont(save);
}

int Text(int x, int y, vec3 &color, char *format, ...) {
    // place text at screen-space pixel (x, y)
    char buf[500];
    FormatString(buf, 500, format);
    int ySpace = 12; // glmIsFontLarge()? 15 : 12;
    const char *b = buf;
    char line[500];
    const char *endLine;
    int nLine;
    for (nLine = 0; *b; nLine++, b = endLine+1) {
     // int yy = y+ySpace*nLine;
        int yy = y-ySpace*nLine;
        while (*b == '\n')
            b++;
        if (!(endLine = strchr(b, '\n'))) {
            PutString(x, yy, b, color);
            break;
        }
        int nChars = endLine-b;
        strncpy(line, b, nChars);
        line[nChars] = 0;
        PutString(x, yy, line, color);
    }
    return nLine+1;
}

void Text(vec3 &p, mat4 &m, char *text, vec3 &color) {
	Text(p, m, 0, 0, color, text);
}

void Text(vec3 &p, mat4 &m, vec3 &color, char *format, ...) {
    char buf[500];
    FormatString(buf, 500, format);
	Text(p, m, buf, color);
}

void Text(vec3 &p, mat4 &m, vec3 &color, bool bold, char *format, ...) {
	void *save = bold? SetBold() : GetFont();
    char buf[500];
    FormatString(buf, 500, format);
	Text(p, m, buf, color);
	SetFont(save);
}

void Text(vec3 &p, mat4 &m, int xoff, int yoff, vec3 &color, char *format, ...) {
    char buf[500];
	vec2 s = ScreenPoint(p, m);
    FormatString(buf, 500, format);
	Text((int) s.x+xoff, (int) s.y+yoff, color, buf);
}

int FontSize(void *font) {
	int sizes[] = {9, 8, 10, 24, 10, 12, 18};
	void *fonts[] = {GLUT_BITMAP_9_BY_15,
					 GLUT_BITMAP_8_BY_13,
					 GLUT_BITMAP_TIMES_ROMAN_10,
					 GLUT_BITMAP_TIMES_ROMAN_24,
					 GLUT_BITMAP_HELVETICA_10,
					 GLUT_BITMAP_HELVETICA_12,
					 GLUT_BITMAP_HELVETICA_18};
	for (int i = 0; i < sizeof(fonts)/sizeof(void *); i++)
		if (fonts[i] == font)
			return sizes[i];
	return -1;
}

// Miscellany

void ScreenLine(vec2 &v1, vec2 &v2, vec3 &col) {
	float c[] = {col.x, col.y, col.z};
	Line(v1.x, v1.y, v2.x, v2.y, c, c);
}

void ScreenBox(float x1, float y1, float x2, float y2, vec3 &col) {
	float c[] = {col.x, col.y, col.z};
	Line(x1, y1, x1, y2, c, c);
	Line(x1, y2, x2, y2, c, c);
	Line(x2, y2, x2, y1, c, c);
	Line(x2, y1, x1, y1, c, c);
}

void ScreenTriangle(vec2 &v1, vec2 &v2, vec2 &v3, vec3 &col) {
	ScreenLine(v1, v2, col);
	ScreenLine(v2, v3, col);
	ScreenLine(v3, v1, col);
}

void Rectangle(int x, int y, int w, int h, float *col, bool solid, float opacity) {
	float linewidth;
	glGetFloatv(GL_LINE_WIDTH, &linewidth);
	float halfw = .5f*linewidth;
	float x1 = (float) x, x2 = (float) (x+w), y1 = (float) y, y2 = (float) (y+h);
	if (solid)
		Quad(vec3(x1, y1, 0), vec3(x2, y1, 0), vec3(x2, y2, 0), vec3(x1, y2, 0), col, opacity);
	else {
		Line(x1-halfw, y1, x2+halfw, y1, col, col, opacity);
		Line(x2, y1-halfw, x2, y2+halfw, col, col, opacity);
		Line(x1-halfw, y2, x2+halfw, y2, col, col, opacity);
		Line(x1, y1-halfw, x1, y2+halfw, col, col, opacity);
	}
}

void Axes(vec2 &s, mat4 &rotate, float f, char *xLabel, char *yLabel, char *zLabel) {
	vec4 x = rotate*vec4(1,0,0,0), y = rotate*vec4(0,1,0,0), z = rotate*vec4(0,0,1,0);
	Arrow(s, s+f*vec2(x.x, x.y), vec3(0), xLabel, 6);
	Arrow(s, s+f*vec2(y.x, y.y), vec3(0), yLabel, 6);
	Arrow(s, s+f*vec2(z.x, z.y), vec3(0), zLabel, 6);
}

void Cross(vec3 &p, float s, vec3 &col) {
	vec3 p1, p2;
	for (int n = 0; n < 3; n++)
		for (int i = 0; i < 3; i++) {
			float offset = i==n? s : 0;
			p1[i] = p[i]+offset;
			p2[i] = p[i]-offset;
			Line(p1, p2, col, col);
		}
}

void Asterisk(vec3 &p, float s, vec3 &col) {
	vec3 p1, p2;
	for (int i = 0; i < 8; i++) {
		p1.x = p.x+(i<4? -s : s);
		p1.y = p.y+(i%2? -s : s);
		p1.z = p.z+((i/2)%2? -s : s);
		p2 = 2*p-p1;
		Line(p1, p2, col, col);
	}
}

static float sqrt2o2 = sqrt(2.f)/2.f;

void Asterisk(vec2 &p, float s, vec3 &col) {
	float f = sqrt2o2*s, off[][2] = {{0,s}, {f, f}, {s,0}, {f, -f}};
	for (int i = 0; i < 4; i++) {
		float *d = off[i];
		Line(p.x+d[0], p.y+d[1], p.x-d[0], p.y-d[1], col, col);
	}
}

void Crosshairs(vec2 &s, float radius, vec3 &color) {
    float innerRad = .4f*radius;
	Circle(s, .5f, color);
    Circle(s, 2*innerRad, color);
    Line(s.x-innerRad, s.y, s.x-radius, s.y, color, color);
    Line(s.x+innerRad, s.y, s.x+radius, s.y, color, color);
    Line(s.x, s.y-innerRad, s.x, s.y-radius, color, color);
    Line(s.x, s.y+innerRad, s.x, s.y+radius, color, color);
}

void Sun(vec2 &p, vec3 *flashColor) {
	vec3 yel(1, 1, 0), red(1, 0, 0), *col = flashColor? flashColor : &red;
	// wish small yellow on larger red disk regardless of z-buffer
    Disk(p, 8, yel);
    Disk(p, 12, *col);
    Disk(p, 8, yel);
	glLineWidth(1.);
    for (int r = 0, nRays = 16; r < nRays; r++) {
        float a = 2*3.141592f*(float)r/(nRays-1), dx = cos(a), dy = sin(a);
        float len = 11*(r%2? 1.8f : 2.5f);
        Line(p.x+9*dx, p.y+9*dy, p.x+len*dx, p.y+len*dy, *col, *col);
    }
}
