// BezierCurve_Bonus1.cpp - interactive curve design

#include <gl/glew.h>
#include <gl/freeglut.h>
#include "mat.h"
#include <time.h>
#include "UI.h"

// Bezier class

class Bezier {
public:
	int res;				// display resolution
	vec3 p1, p2, p3, p4;	// control points
	time_t startTime;
	Bezier(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, time_t start, int res = 50) : p1(p1), p2(p2), p3(p3), p4(p4), startTime(start), res(res) { }

	// return a point on the Bezier curve given parameter t, in (0,1)
	vec3 Point(float t) {
		float t_squared = (t * t);
		float t_cubed = (t * t * t);
		vec3 first_term = p1 * (-t_cubed + (3 * t_squared) - (3 * t) + 1);
		vec3 second_term = p2 * ((3 * t_cubed) - (6 * t_squared) + (3 * t));
		vec3 third_term = p3 * ((-3 * t_cubed) + (3 * t_squared));
		vec3 fourth_term = p4 * (t_cubed);
		vec3 point = first_term + second_term + third_term + fourth_term;
		return first_term + second_term + third_term + fourth_term;	// point on curve at t
	}

	// break the curve into res number of straight pieces, render each with Line()
	void Draw(vec3 color) {
		float t1, t2;
		vec3 point1, point2;
		for (int i = 0; i < res; ++i) {
			t1 = i / (float)res;
			t2 = (i + 1) / (float)res;
			point1 = Point(t1);
			point2 = Point(t2);
			//printf("i = %d, t1 = %f, t2 = %f", i, t1, t2);
			Line(point1, point2, color, 1, 2, false);
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
		// draw the outer mesh (3 edges)
		Line(p1, p2, meshColor, opacity, width, false);
		Line(p2, p3, meshColor, opacity, width, false);
		Line(p3, p4, meshColor, opacity, width, false);

		// draw second layer of mesh (2 edges - connecting outer edge midpoints)
		vec3 mid_p1p2 = GetMidpoint(p1, p2);
		vec3 mid_p2p3 = GetMidpoint(p2, p3);
		vec3 mid_p3p4 = GetMidpoint(p3, p4);
		Line(mid_p1p2, mid_p2p3, meshColor, opacity, width, false);
		Line(mid_p2p3, mid_p3p4, meshColor, opacity, width, false);

		// draw innermost layer of mesh (1 edge - connecting second layer midpoints)
		vec3 mid_p1p2_p2p3 = GetMidpoint(mid_p1p2, mid_p2p3);
		vec3 mid_p2p3_p3p4 = GetMidpoint(mid_p2p3, mid_p3p4);
		Line(mid_p1p2_p2p3, mid_p2p3_p3p4, meshColor, opacity, width, false);

		// draw the four control points
		Disk(p1, 4, pointColor, opacity);
		Disk(p2, 4, pointColor, opacity);
		Disk(p3, 4, pointColor, opacity);
		Disk(p4, 4, pointColor, opacity);

		// draw a disk to follow the curve
		vec3 travelingPointColor = vec3(.26, .28, 1);
		time_t currTime = clock();
		static float degPerSec = 15;
		float dt = (float)(currTime - startTime) / CLOCKS_PER_SEC;
		float radAng = (3.1415f / 180.f)*dt*degPerSec;
		float s = sin(radAng);
		if (s < 0) { s *= -1; }		// pretty funny if you comment this line out... highly recommend
		vec3 currPoint = Point(s);
		Disk(currPoint, 4, travelingPointColor, opacity);
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
		// find minimum distance/closest point
		float minDist_p1_p2, minDist_p3_p4, minDist;
		vec3 *closest_p1_p2, *closest_p3_p4, *closestPoint;
		closest_p1_p2 = FirstIsCloser(distFrom_p1, distFrom_p2, minDist_p1_p2) ? &p1 : &p2;
		closest_p3_p4 = FirstIsCloser(distFrom_p3, distFrom_p4, minDist_p3_p4) ? &p3 : &p4;
		closestPoint = FirstIsCloser(minDist_p1_p2, minDist_p3_p4, minDist) ?
			closest_p1_p2 : closest_p3_p4;
		// return closest point if within 10 pixels of mouse
		return minDist <= 100 ? closestPoint : NULL;
	}
};

// Animation

time_t startTime = clock();

void Idle() {
	glutPostRedisplay();
}

Bezier	curve(vec3(.15f, .15f, 0), vec3(.25f, .75f, 0), vec3(.75f, .75f, 0), vec3(.85f, .15f, 0), startTime);

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
	glutCreateWindow("Bezier Curve with Traveling Point");
	glewInit();
	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutIdleFunc(Idle);
	glutMainLoop();
}
