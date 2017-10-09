// WidgetX.h - buttons, sliders, etc (optimized) - from /2012
// copyright (c) Jules Bloomenthal, 2012-2016, all rights reserved

#ifndef WIDGET_HDR
#define WIDGET_HDR

#include <string>
#include <vector>
#include "mat.h"

using std::string;
using std::vector;

#pragma warning(disable:4786) // kill MS warning re long names, nested templates

// Keyboard

bool KeyUp(int button);

bool KeyDown(int c);
//bool KeyDown(char c);


// Support

void DrawDepressedBox(int x, int y, int w, int h, float *color = NULL);

void DrawUnpressedBox(int x, int y, int w, int h, float *color = NULL);

void DrawColoredBox(int x, int y, int w, int h, int id, vec3 &col);

void DrawBlankBox(int x, int y, int w, int h);

// Headers

class Header {
public:
	int x1, y1, x2, y2;
	char title[100];
	float textColor[3];
	Header();
	Header(int x1, int y1, int x2, int y2, char *title, float *textColor = NULL);
	void Init(int x1, int y1, int x2, int y2, char *title, float *textColor = NULL);
	bool Within(int x, int y);
	void Draw();
};

// Buttons

class Button {
public:
	enum {B_Checkbox, B_Rectangle} type;	// see WidgetOld.h for B_Dot, B_Tube
    int x, y, w, h, winW, winH;				// in pixels
	float backgroundColor[3], textColor[3]; // defaults are white, black
	string name;
	int textWidth;
	void *font;
	bool *value;
	Button();
	// rectangle button:
    Button(int x, int y, int w, int h, char *name, float *backgroundColor = NULL, float *textColor = NULL);
	// checkbox:
	Button(int x, int y, int size, char *name, bool *value, float *textColor = NULL);
	void InitCheckbox(int x, int y, int size = 15, char *name = NULL, bool *value = NULL, float *textColor = NULL);
	void InitRectangle(int x, int y, int w, int h, char *name, float *backgroundColor = NULL, float *textColor = NULL);
	void CenterText(int x, int y, char *text, int buttonWidth, float *c);
	void SetTextColor(float *col);
	void ShowName(char *name, float *color);
	    // sets textWidth, which affects Hit
	void Draw(float *statusColor = NULL);
	void Draw(char *name = NULL, float *statusColor = NULL);
		// if name NULL, use button name, else override
		// if checkbox, statusColor NULL means unchecked
		// if rectangle, statusColor NULL means unpressed
	void Highlight();
    bool Hit(int x, int y);
		// does mouse(x,y) hit the button?
	bool UpHit(int x, int y);
		// as Hit, but toggle *value if hit and button is checkbox
};

// Sliders

enum Orientation {Hor, Ver};

class Slider {
public:
	Orientation orientation;
	int winW, winH;	// window size
    int x, y;				// lower-left location, in pixels
    int size;				// length, in pixels
	float color[3];
    int loc;				// slider x-position, in pixels
    float min, max;
	string name;
	Slider();
    Slider(int x, int y, int size, float min, float max, float init, Orientation o = Hor, char *name = NULL, float *color = NULL);
    void Init(int x, int y, int size, float min, float max, float init, Orientation o = Hor, char *name = NULL, float *color = NULL);
	void SetValue(float val);
	void SetRange(float min, float max, float init);
    void Draw(char *nameOverride = NULL, float *sliderColor = NULL);
    float GetValue();
    bool Mouse(int x, int y);
		// called upon mouse-down or mouse-drag; return true if call results in new slider location
    bool Hit(int x, int y);
};

// Mover, Aimer

class Mover {
public:
	vec3 *point;
	Mover() {}
	Mover(vec3 *p) {point = p;}
    void Set(vec3 *p);
    void Down(int x, int y, mat4 &modelview, mat4 &persp);
    void Drag(int x, int y, mat4 &modelview, mat4 &persp);
	bool Hit(int x, int y, mat4 &view);
private:
    float plane[4];		// needn't be normalized
    void SetPlane(int x, int y, mat4 &modelview, mat4 &persp);
};

class Aimer {
public:
	// see Widget:Mover, Widgets:WidgetPlane
	Aimer() : size(1), mode(A_None) { };
	void Set(vec3 b, vec3 v, mat4 modelview, mat4 persp, float s = 1);
	bool Hit(int x, int y);
    void Down(int x, int y);
    void Drag(int x, int y);
	void Draw(vec3 &color, float baseQuadScale = 0);
private:
	vec3 base, vector, tip;								// vector presumed unit length
    float plane[4], size;
	mat4 modelview, persp;
	bool fwdFace;
	enum {A_None, A_Base, A_Tip} mode;
};

// Typescripts

const int maxTypescript = 200;

class Typescript {
public:
    int x, y, w, h;
    char text[maxTypescript], save[maxTypescript];
    Typescript();
    Typescript(int x, int y, int w, int h, char *text = NULL);
    void Clear();
	void Restore();
    void AddChar(char c);
	void SetText(const char *txt);
    bool Hit(int _x, int _y);
    void Draw(float *color = NULL, float *bgrnd = NULL);
};

// Miscellany

string GetDirectory();
void GetFilenames(char *search, vector<string> &names);
void NewestFilename(char *search, string &name);
bool ChooseFile(bool read, string startDir, char *wbuf, string &filename, string title);
	// create dialog box with given title
	// dialog box will contain files from startDir that match a file extension
	// if write (!read), filename can contain suggested name
	// if no file selected, return false; else, set filename, return true
bool ChooseFile(bool read, string startDir, string &filename, string title);

#endif
