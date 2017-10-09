/* =============================================
    DrawX.h - collection of draw support routines - from /2012
    Copyright (c) Jules Bloomenthal, 2014
    All rights reserved
    ============================================= */

#ifndef DRAW_HDR
#define DRAW_HDR

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "mat.h"

// OpenGL errors
int Errors(char *buf);
	// set buf to list of errors, return # errors
void CheckGL_Errors(char *msg = NULL);
	// print OpenGL errors

// screen operations
mat4 ScreenMode(int width, int height);
	// create matrix to map pixel space, (0,0)-(width,height), to NDC (clip) space, (-1,-1)-(1,1)
mat4 ScreenMode();
	// as above (GLUT provides width, height)
bool IsVisible(vec3 &p, mat4 &fullview, vec2 *screen = NULL);
	// if the depth test is enabled, is point p visible?
	// if non-null, set screen location (in pixels) of transformed p
void ScreenPoint(vec3 p, mat4 m, float &xscreen, float &yscreen, float *zscreen = NULL);
	// transform 3D point to location (xscreen, yscreen), in pixels; if non-null, set zscreen
vec2 ScreenPoint(vec3 p, mat4 m, float *zscreen = NULL);
double ScreenZ(vec3 &p, mat4 &m);
void ScreenLine(float xscreen, float yscreen, mat4 &modelview, mat4 &persp, float p1[], float p2[]);
    // compute 3D world space line, given by p1 and p2, that transforms
    // to a line perpendicular to the screen at pixel (xscreen, yscreen)
vec3 ScreenVec(float xscreen, float yscreen, mat4 &modelview, mat4 &persp);
	// as above, but return unit-length vector perpendicular to screen
float ScreenDistSq(int x, int y, vec3 p, mat4 m, float *zscreen = NULL);
	// return distance squared, in pixels, between screen point (x, y) and point p xformed by view matrix
float ScreenDistSq(vec3 &p1, vec3 &p2, mat4 &view);
	// as above, but between two points

// misc operations
vec3 Ortho(vec3 &v);
	// perpendicular vector of the same length
vec3 ProjectToLine(vec3 &p, vec3 &p1, vec3 &p2);
bool FrontFacing(vec3 &base, vec3 &vec, mat4 &view);

// 2D/3D drawing functions
int UseDrawShader();
	// invoke shader for these draw routines
int UseDrawShader(mat4 viewMatrix);
	// as above, but update view transformation
void SetOpacity(float opacity);

// 3D line
void Line(vec3 &p1, vec3 &p2, vec3 &color, float opacity = 1, float width = 1, bool dashed = false, bool dotted = false);
	// draw line between 3D endpoints p1, p2 with given color
void Line(vec3 &p1, vec3 &p2, vec3 &color1, vec3 &color2, float opacity = 1);
	// as above but with two colors
void Line(float *p1, float *p2, float *color1, float *color2, float opacity = 1);

// 2D line
void Line(float x1, float y1, float x2, float y2, float *color1, float *color2, float opacity = 1);
void Line(int x1, int y1, int x2, int y2, float *color1, float *color2, float opacity = 1);

// disk
void Disk(vec3 &p, float diameter, vec3 &col, mat4 &fullview, char *text, ...);
void Disk(int x, int y, float diameter, float *color, float opacity = 1);
	// display a solid disk at pixel (x, y) of given radius (in pixels) and color
void Disk(vec2 &p, float diameter, vec3 &color, float opacity = 1);
void Disk(vec3 &p, float diameter, vec3 &color, float opacity = 1);
void DiskRing(vec3 &p, float outDia, vec3 &outColor, float inDia, vec3 &inColor, float opacity = 1);
void DiskRing(vec2 &p, float outDia, vec3 &outColor, float inDia, vec3 &inColor, float opacity = 1);

// circle, sphere
void Circle(vec2 &s, float dia, vec3 &color);
	// draw a circle centered at s with given screen-space diameter and color
void Circle(vec3 &p, mat4 &m, float dia, vec3 &color, char *msg = NULL);
	// as above but first transform p to screen space
void Circle(vec3 &p, vec3 &n, float rad, vec3 &color, bool dots = false, float lineWidth = 1, bool dashed = false);
	// draw a circle in 3D space at point p with normal n and 3D space radius
	// dots: place dots at circle verts
void Circle(vec3 &base, float diameter, mat4 &modelview, mat4 &persp, vec3 &color);
	// draw a screen-space circle with a world-space diameter centered at base 

void DrawSphere(vec3 &p, float rad, vec3 &color);

// arrow
void Arrow(vec2 &base, vec2 &head, vec3 &color, char *label = NULL, double headSize = 4);
	// display an arrow between base and head
void ArrowV(vec3 &base, vec3 &vector, mat4 &m, vec3 &color, char *label = NULL, double headSize = 4);
	// as above but vector and base are 3D, transformed by m

// triangle
vec3 TriangleShrink(vec3 &p1, vec3 &p2, vec3 &p3, float scaleAboutCenter);
void TriangleLines(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &color, float opacity = 1);
void TriangleLinesScale(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &color, float scaleAboutCenter, float opacity = 1);
void TriangleShade(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &color, float opacity = 1);
void Triangle(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &col1, vec3 &col2, vec3 &col3, float opacity = 1);

// quad
void Quad(vec3 &pnt1, vec3 &pnt2, vec3 &pnt3, vec3 &pnt4, float *col, float opacity);
void QuadLines(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, float *col, float opacity);

// box
void Box(vec3 &min, vec3 &max, vec3 &color);

// stipple
bool DashOn(int factor = 1, int offset = 0);
	// subsequent line-drawing is dashed; return true if already in stippled mode
bool DotOn(int factor = 1, int offset = 0);
	// as above but dotted
void DashOff();
void DotOff();
void Stipple(int factor=1, int a=-1,int b=-1,int c=-1,int d=-1,int e=-1,
						   int f=-1,int g=-1,int h=-1,int i=-1,int j=-1,
						   int k=-1,int l=-1,int m=-1,int n=-1,int o=-1,int p=-1);
						   // args a-p are bit positions: glmStipple(1, 4) means bit 4 on, others off

// text
void *SetFont(void *font = GLUT_BITMAP_9_BY_15);
void *GetFont();
void *SetBold();
void PutString(int x, int y, const char *text, vec3 &color, void *font = NULL);
int Text(int x, int y, vec3 &color, char *format, ...);
	// position null-terminated text at pixel (x, y)
void Text(vec3 &p, mat4 &m, char *text, vec3 &color = vec3(0));
	// as above but with text positioned at screen location of transformed p
void Text(vec3 &p, mat4 &m, vec3 &color, char *format, ...);
	// as above, but as formatted text (ie, like printf)
void Text(vec3 &p, mat4 &m, int xoff, int yoff, vec3 &color, char *format, ...);
int FontSize(void *font);

// screen-space draw
void ScreenLine(vec2 &v1, vec2 &v2, vec3 &col);
void ScreenBox(float x1, float y1, float x2, float y2, vec3 &col);
void ScreenTriangle(vec2 &v1, vec2 &v2, vec2 &v3, vec3 &col);
void Rectangle(int x, int y, int w, int h, float *color, bool solid = true, float opacity = 1);
	// draw an optionally filled rectangle between pixel(x,y) and (x+width-1, y+height-1)
void Crosshairs(vec2 &s, float radius, vec3 &color);
	// draw a crosshairs at s
void Sun(vec2 &s, vec3 *flashColor = NULL);
	// draw a sun at pixel s, with optionally colored sun-rays

// misc objects
void Axes(vec2 &s, mat4 &rotate, float scale = 30, char *xLabel = "X", char *yLabel = "Y", char *zLabel = "Z");
	// draw the axes at screen x, y (presumes fullview has uniform scale)
void Cross(vec3 &p, float s, vec3 &col);
	// draw a cross at 3D point p, of size s
void Asterisk(vec3 &p, float s, vec3 &col);
	// draw an asterisk at 3D point p, with scale s and color
void Asterisk(vec2 &p, float s, vec3 &col);
	// as above, but flattened

#endif
