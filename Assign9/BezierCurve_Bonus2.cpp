// BezierCurve_Bonus2.cpp - interactive curve design

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "mat.h"
#include <time.h>
#include "UI.h"

// Bezier class

class Bezier {
public:
	int res;				// display resolution
	vec3 p1, p2, p3, p4, p5, p6, p7;	// control points
	bool p4_isLastPointMoved = false;
	Bezier(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p5, vec3 &p6, vec3& p7, int res = 50) :
		p1(p1), p2(p2), p3(p3), p5(p5), p6(p6), p7(p7), res(res) {
		p4 = GetMidpoint(p3, p5);
	}

	void LastPointMoved(vec3& pointMoved)
	{
		p4_isLastPointMoved = (pointMoved == p4);
	}

	// return a point on the Bezier curve given parameter t, in (0,1)
	vec3 Point(float t, bool isFirstCurve) {
		vec3 point;
		float t_squared = (t * t);
		float t_cubed = (t * t * t);
		vec3 first_term = (-t_cubed + (3 * t_squared) - (3 * t) + 1);
		vec3 second_term = ((3 * t_cubed) - (6 * t_squared) + (3 * t));
		vec3 third_term = ((-3 * t_cubed) + (3 * t_squared));
		vec3 fourth_term = (t_cubed);
		if (isFirstCurve) {
			point = p1*first_term + p2*second_term + p3*third_term + p4*fourth_term;
		}
		else {
			point = p4*first_term + p5*second_term + p6*third_term + p7*fourth_term;
		}
		return point;	// point on curve at t
	}

	// break the curve into res number of straight pieces, render each with Line()
	void Draw(vec3 color) {
		float t1, t2;
		vec3 point1a, point2a, point1b, point2b;

		t1 = 0 / (float)res;
		point1a = Point(t1, true);
		point1b = Point(t1, false);

		for (int i = 0; i < res; ++i) {
			t2 = (i + 1) / (float)res;
			point2a = Point(t2, true);
			point2b = Point(t2, false);
			Line(point1a, point2a, color, 1, 2, false);
			Line(point1b, point2b, color, 1, 2, false);
			point1a = point2a;
			point1b = point2b;
		}
	}

	// returns the midpoint between two points (A and B)
	vec3 GetMidpoint(const vec3& pA, const vec3& pB) {
		float x = (pA.x + pB.x) / (float)2;
		float y = (pA.y + pB.y) / (float)2;
		float z = (pA.z + pB.z) / (float)2;
		return vec3(x, y, z);
	}

	// draw the four control points and the mesh that connects them
	void DrawControlMesh(vec3 pointColor, vec3 meshColor, float opacity, float width) {
		// keep p3 and p5 colinear with p4 as the midpoint
		vec3 sharedPoint = GetMidpoint(p3, p5);
		if (sharedPoint.x != p4.x || sharedPoint.y != p4.y || sharedPoint.z != p4.z) {
			// if p4 moved - adjust p3 and p5
			if (p4_isLastPointMoved) {
				float xAdjust = sharedPoint.x - p4.x;
				float yAdjust = sharedPoint.y - p4.y;
				float zAdjust = sharedPoint.z - p4.z;
				p3 = vec3(p3.x - xAdjust, p3.y - yAdjust, p3.z - zAdjust);
				p5 = vec3(p5.x - xAdjust, p5.y - yAdjust, p5.z - zAdjust);
			}
			// if either p3 or p5 moved, adjust p4
			else {
				p4 = sharedPoint;
			}
		}

		// draw the outer mesh (3 edges)
		Line(p1, p2, meshColor, opacity, width, false);
		Line(p2, p3, meshColor, opacity, width, false);
		Line(p3, p4, meshColor, opacity, width, false);

		Line(p4, p5, meshColor, opacity, width, false);
		Line(p5, p6, meshColor, opacity, width, false);
		Line(p6, p7, meshColor, opacity, width, false);

		// draw second layer of mesh (2 edges - connecting outer edge midpoints)
		vec3 mid_p1p2 = GetMidpoint(p1, p2);
		vec3 mid_p2p3 = GetMidpoint(p2, p3);
		vec3 mid_p3p4 = GetMidpoint(p3, p4);
		Line(mid_p1p2, mid_p2p3, meshColor, opacity, width, false);
		Line(mid_p2p3, mid_p3p4, meshColor, opacity, width, false);

		vec3 mid_p4p5 = GetMidpoint(p4, p5);
		vec3 mid_p5p6 = GetMidpoint(p5, p6);
		vec3 mid_p6p7 = GetMidpoint(p6, p7);
		Line(mid_p4p5, mid_p5p6, meshColor, opacity, width, false);
		Line(mid_p5p6, mid_p6p7, meshColor, opacity, width, false);

		// draw innermost layer of mesh (1 edge - connecting second layer midpoints)
		vec3 mid_p1p2_p2p3 = GetMidpoint(mid_p1p2, mid_p2p3);
		vec3 mid_p2p3_p3p4 = GetMidpoint(mid_p2p3, mid_p3p4);
		Line(mid_p1p2_p2p3, mid_p2p3_p3p4, meshColor, opacity, width, false);

		vec3 mid_p4p5_p5p6 = GetMidpoint(mid_p4p5, mid_p5p6);
		vec3 mid_p5p6_p6p7 = GetMidpoint(mid_p5p6, mid_p6p7);
		Line(mid_p4p5_p5p6, mid_p5p6_p6p7, meshColor, opacity, width, false);

		// draw the four control points
		Disk(p1, 4, pointColor, opacity);
		Disk(p2, 4, pointColor, opacity);
		Disk(p3, 4, pointColor, opacity);
		Disk(p4, 4, pointColor, opacity);
		Disk(p5, 4, pointColor, opacity);
		Disk(p6, 4, pointColor, opacity);
		Disk(p7, 4, pointColor, opacity);
	}

	// assigns the smaller of the two distances to minDist and returns true if the first point
	// (A) has the smallest distance, or false if the second point (B) has the smallest (or
	// distance 
	bool FirstIsCloser(float distFromA, float distFromB, float& minDist) {
		if (distFromA < distFromB) {
			minDist = distFromA;
			return true;
		}
		minDist = distFromB;
		return false;
	}

	// return pointer to nearest control point, if within 10 pixels of mouse (x,y), else NULL
	vec3 *PickPoint(int x, int y, mat4 view) {
		float distFrom_p1 = ScreenDistSq(x, y, p1, view);
		float distFrom_p2 = ScreenDistSq(x, y, p2, view);
		float distFrom_p3 = ScreenDistSq(x, y, p3, view);
		float distFrom_p4 = ScreenDistSq(x, y, p4, view);
		float distFrom_p5 = ScreenDistSq(x, y, p5, view);
		float distFrom_p6 = ScreenDistSq(x, y, p6, view);
		float distFrom_p7 = ScreenDistSq(x, y, p7, view);

		// find minimum distance/closest point
		float minDist_p1_p2, minDist_p3_p4, minDist_p5_p6, minDist_p7,
			minDist_firstCurve, minDist_secondCurve, minDist;
		vec3 *closest_p1_p2, *closest_p3_p4, *closest_p5_p6, *closest_p7,
			*closest_firstCurve, *closest_secondCurve, *closestPoint;

		closest_p1_p2 = FirstIsCloser(distFrom_p1, distFrom_p2, minDist_p1_p2) ? &p1 : &p2;
		closest_p3_p4 = FirstIsCloser(distFrom_p3, distFrom_p4, minDist_p3_p4) ? &p3 : &p4;

		closest_p5_p6 = FirstIsCloser(distFrom_p5, distFrom_p6, minDist_p5_p6) ? &p5 : &p6;
		closest_p7 = FirstIsCloser(distFrom_p7, distFrom_p7, minDist_p7) ? &p7 : &p7;

		closest_firstCurve = FirstIsCloser(minDist_p1_p2, minDist_p3_p4, minDist_firstCurve) ?
			closest_p1_p2 : closest_p3_p4;
		closest_secondCurve = FirstIsCloser(minDist_p5_p6, minDist_p7, minDist_secondCurve) ?
			closest_p5_p6 : closest_p7;

		closestPoint = FirstIsCloser(minDist_firstCurve, minDist_secondCurve, minDist) ?
			closest_firstCurve : closest_secondCurve;

		// return closest point if within 10 pixels of mouse
		if(minDist <= 100)
		{
			LastPointMoved(*closestPoint);
			return closestPoint;
		}
		// otherwise return null
		return NULL;
	}
};

Bezier	curve(vec3(.10f, .50f, 0), vec3(.25f, .80f, 0), vec3(.50f, .80f, 0), vec3(.65f, .50f, 0),
	vec3(.90f, .30f, 0), vec3(.95f, .65f, 0));

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
	// update transformations, enable UI draw shader
	mat4 ortho = Ortho(-1, 1, -1, 1, -.01f, -10);
	view = ortho*Translate(0, 0, 1)*RotateY(rotNew.x)*RotateX(rotNew.y);
	UseDrawShader(view); // no shading, so single matrix
	// curve and mesh
	curve.Draw(vec3(.7f, .2f, .5f));
	curve.DrawControlMesh(vec3(0, .4f, 0), vec3(1, 1, 0), 1, 1.5f);
	glFlush();
}

// Mouse

int		winWidth = 630, winHeight = 500;
int		xMouseDown, yMouseDown;				// for each mouse down, need start point
bool 	cameraDown = false;
Mover	ptMover;

void MouseButton(int butn, int state, int x, int y) {
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;						// invert y for upward-increasing screen space
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
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;
	if (ptMover.point)
		ptMover.Drag(x, y, view);
	else if (cameraDown)
		rotNew = rotOld + .3f*(vec2((float)(x - xMouseDown), (float)(y - yMouseDown)));
	glutPostRedisplay();
}

// Application

int main(int ac, char **av) {
	glutInit(&ac, av);
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Bezier Curves... Two = Company");
	glewInit();
	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutMainLoop();
}
