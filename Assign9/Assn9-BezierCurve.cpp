// BezierCurveGLSL.cpp - interactive curve design

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "mat.h"
#include <time.h>
#include "UI.h"

// Bezier curve

class Bezier {
public:
	int res;				// display resolution
	vec3 p1, p2, p3, p4;	// control points
	Bezier(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, int res = 50) : p1(p1), p2(p2), p3(p3), p4(p4), res(res) { }
	vec3 Point(float t) {
		// return a point on the Bezier curve given parameter t, in (0,1)
		float t2 = t*t, t3 = t*t2;
		return (-t3+3*t2-3*t+1)*p1+(3*t3-6*t2+3*t)*p2+(3*t2-3*t3)*p3+t3*p4;
	}
	void Draw(vec3 color) {
		// break the curve into res number of straight pieces, render each with Line()
		vec3 previous = p1;
		for (int i = 1; i < res; i++) {
			float s = (float) i /(res-1);
			vec3 p = Point(s);
			Line(previous, p, color, 1, 2);
			previous = p;
		}
	}
	void DrawControlMesh(vec3 pointColor, vec3 meshColor, float opacity, float width) {
		// draw the four control points and the mesh that connects them
		for (int i = 1; i < 4; i++) {
			vec3 &a = i==3? p3 : i==2? p2 : i? p1 : p4;
			vec3 &b = i==3? p4 : i==2? p3 : i? p2 : p1;
			Line(a, b, meshColor, opacity, width, true);
		}
		for (int i = 0; i < 4; i++)
			Disk(i==3? p4 : i==2? p3 : i? p2 : p1, 8, pointColor);
	}
	vec3 *PickPoint(int x, int y, mat4 view) {
		// return pointer to nearest control point, if within 10 pixels of mouse (x,y), else NULL
		vec3 *ret = NULL;
		float dsqmin = 100; // 10 pixels
		for (int k = 0; k < 4; k++) {
			vec3 *pt = k==3? &p4 : k==2? &p3 : k? &p2 : &p1;
			float dsq = ScreenDistSq(x, y, *pt, view);
			if (dsq < dsqmin) {
				dsqmin = dsq;
				ret = pt;
			}
		}
		return ret;
	}
};

Bezier	curve(vec3(.5f,-.25f,0), vec3(.5f,.25f,0), vec3(-.5f,-.25f,0), vec3(-.5f,.25f,0));

// Animation

time_t	startTime = clock();

void Idle() { glutPostRedisplay(); }

// Display

mat4	view;
vec2	rotOld, rotNew;	// rot.x is about Y-axis, rot.y is about X-axis

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
	// update transformations, enable draw shader
	mat4 ortho = Ortho(-1, 1, -1, 1, -.01f, -10);
	view = ortho*Translate(0, 0, 1)*RotateY(rotNew.x)*RotateX(rotNew.y);
	UseDrawShader(view); // no shading, so single matrix
	// curve and moving dot
	curve.Draw(vec3(.7f, .2f, .5f));
	curve.DrawControlMesh(vec3(0, .4f, 0), vec3(1, 1, 0), 1, 1.5f);
	float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;
	Disk(curve.Point((sin(dt)+1.f)/2.f), 12, vec3(1, 0, 0));
    glFlush();
}

// Mouse

int		winWidth = 630, winHeight = 500;
int		xMouseDown, yMouseDown;				// for each mouse down, need start point
bool 	cameraDown = false;
Mover	ptMover;

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;		// invert y for upward-increasing screen space
    if (state == GLUT_UP) {
		if (cameraDown)
			rotOld = rotNew;				// reset rotation accumulation
		else if (ptMover.point)
			ptMover.Set(NULL);				// deselect control point
	}
	cameraDown = false;
	if (state == GLUT_DOWN) {
		vec3 *pp = curve.PickPoint(x, y, view);
		if (pp) {
			if (butn == GLUT_LEFT_BUTTON) { // pick control point
				ptMover.Set(pp);
				ptMover.Down(x, y, view);
			}
		}
		else {
			cameraDown = true;				// set mouse reference
			xMouseDown = x;
			yMouseDown = y;
		}
	}
    glutPostRedisplay();
}

void MouseDrag(int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
    if (ptMover.point)
        ptMover.Drag(x, y, view);
	else if (cameraDown)
		rotNew = rotOld+.3f*(vec2((float)(x-xMouseDown), (float)(y-yMouseDown)));
    glutPostRedisplay();
}

// Application

int main(int ac, char **av) {
    glutInit(&ac, av);
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bezier Curve");
	glewInit();
    glutDisplayFunc(Display);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
	glutIdleFunc(Idle);
    glutMainLoop();
}
