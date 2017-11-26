// UI.h - buttons, sliders, line and quad drawing
// copyright (c) Jules Bloomenthal, 2017, all rights reserved

#ifndef UI_HDR
#define UI_HDR

#include <string>
#include "mat.h"

using std::string;

// Screen

vec2 ScreenPoint(vec3 &p, mat4 &m);
	// return 2D screen location of p

float ScreenDistSq(int x, int y, vec3 p, mat4 m);
	// return distance in pixels between 2D screen point (x, y) and transformed p

void ScreenLine(float xscreen, float yscreen, mat4 &modelview, mat4 &persp, float p1[], float p2[]);
    // compute 3D world space line, given by p1 and p2, that transforms
    // to a line perpendicular to the screen at pixel (xscreen, yscreen)

mat4 ScreenMode();
	// matrix to transform to pixel space

bool IsVisible(vec3 &p, mat4 &view, vec2 *screen = NULL);
	// if the depth test is enabled, is point p visible?
	// if non-null, set screen location (in pixels) of transformed p
	// view should include modelView and persp/ortho

// Text

int Text(int x, int y, vec3 &color, char *format, ...);

void Text(vec3 &p, mat4 &m, int xoff, int yoff, vec3 &color, char *format, ...);

void Text(vec3 &p, mat4 &m, char *text, vec3 &color);

void Text(vec3 &p, mat4 &m, vec3 &color, char *format, ...);

// Keyboard

bool KeyUp(int button);
  // is the key presently up?

bool KeyDown(int c);
	// is the key presently down?

// Drawing

int UseDrawShader();

int UseDrawShader(mat4 viewMatrix);

bool DashOn();

void DashOff();

void Disk(vec3 &point, float diameter, vec3 &color, float opacity = 1);

void Line(vec3 &p1, vec3 &p2, vec3 &color, float opacity = 1, float width = 1, bool dashed = false);

void Line(int x1, int y1, int x2, int y2, vec3 &color, float opacity = 1, float width = 1);

void Line(float x1, float y1, float x2, float y2, vec3 &color, float opacity = 1);

void Quad(vec3 &pnt1, vec3 &pnt2, vec3 &pnt3, vec3 &pnt4, vec3 &color, float opacity = 1);

void Rectangle(int x, int y, int w, int h, vec3 &color, bool solid = true, float opacity = 1);

void Circle(vec2 &p, float radius, vec3 &color);

void Crosshairs(vec2 &s, float radius, vec3 &color);

void Sun(vec2 &s, vec3 *flashColor = NULL);
	// draw a sun at pixel s, with optionally colored sun-rays

// Pushbutton and Checkbox

class Button {
public:
	enum {B_Checkbox, B_Rectangle} type;	// see WidgetOld.h for B_Dot, B_Tube
    int x, y, w, h, winW, winH;				// in pixels
	vec3 backgroundColor, textColor;		// defaults are white, black
	string name;
	int textWidth;
	void *font;
	bool *value;
	Button();
	// rectangle button:
    Button(int x, int y, int w, int h, char *name, vec3 *backgroundColor = NULL, vec3 *textColor = NULL);
	// checkbox:
	Button(int x, int y, int size, char *name, bool *value = NULL, vec3 *textColor = NULL);
	void InitCheckbox(int x, int y, int size = 15, char *name = NULL, bool *value = NULL, vec3 *textColor = NULL);
	void InitRectangle(int x, int y, int w, int h, char *name, vec3 *backgroundColor = NULL, vec3 *textColor = NULL);
	void CenterText(int x, int y, char *text, int buttonWidth, vec3 &color);
	void SetTextColor(vec3 &col);
	void ShowName(char *name, vec3 &color);
	    // sets textWidth, which affects Hit
	void Draw(vec3 *statusColor = NULL);
	void Draw(char *name = NULL, vec3 *statusColor = NULL);
		// if name NULL, use button name, else override
		// if checkbox, statusColor NULL means unchecked
		// if rectangle, statusColor NULL means unpressed
	void Highlight();
    bool Hit(int x, int y);
		// does mouse(x,y) hit the button?
	bool UpHit(int x, int y, int state);
		// as Hit, but toggle *value if hit and button is checkbox and state is GLUT_UP
};

// Slider

class Slider {
public:
	bool vertical;
	int winW, winH;	// window size
    int x, y;				// lower-left location, in pixels
    int size;				// length, in pixels
	vec3 color;
    int loc;				// slider x-position, in pixels
    float min, max;
	string name;
	Slider();
    Slider(int x, int y, int size, float min, float max, float init, bool vertical = true, char *name = NULL, vec3 *color = NULL);
    void Init(int x, int y, int size, float min, float max, float init, bool vertical = true, char *name = NULL, vec3 *color = NULL);
	void SetValue(float val);
	void SetRange(float min, float max, float init);
    void Draw(char *nameOverride = NULL, vec3 *sliderColor = NULL);
    float GetValue();
    bool Mouse(int x, int y);
		// called upon mouse-down or mouse-drag; return true if call results in new slider location
    bool Hit(int x, int y);
};

// Mover

class Mover {
public:
	vec3 *point;
	Mover() {}
	Mover(vec3 *p) {point = p;}
    void Set(vec3 *p);
    void Down(int x, int y, mat4 &modelview, mat4 *persp = NULL);
    void Drag(int x, int y, mat4 &modelview, mat4 *persp = NULL);
	bool Hit(int x, int y, mat4 &view);
private:
    float plane[4];		// needn't be normalized
    void SetPlane(int x, int y, mat4 &modelview, mat4 *persp = NULL);
};

#endif
