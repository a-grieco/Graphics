// UI.cpp - Buttons, Sliders, Movers
// copyright (c) Jules Bloomenthal, 2017, all rights reserved

#include "UI.h"
#include "GLSL.h"

// Misc

vec2 ScreenPoint(vec3 &p, mat4 &m) {
	vec4 xp = m*vec4(p, 1);
 	return vec2(
		((xp.x/xp.w)+1)*.5f*(float) glutGet(GLUT_WINDOW_WIDTH),
		((xp.y/xp.w)+1)*.5f*(float) glutGet(GLUT_WINDOW_HEIGHT));
}

float ScreenDistSq(int x, int y, vec3 p, mat4 m) {
	vec2 screen = ScreenPoint(p, m);
	float dx = x-screen.x, dy = y-screen.y;
    return dx*dx+dy*dy;
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
		p1[i] = (float) a[i];
		p2[i] = (float) b[i];
	}
}

mat4 ScreenMode() {
	int width = glutGet(GLUT_WINDOW_WIDTH), height = glutGet(GLUT_WINDOW_HEIGHT);
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

// Text

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

void *font = GLUT_BITMAP_9_BY_15;

void PutString(int x, int y, const char *text, vec3 &color) {
	int program = GLSL::CurrentShader();
	int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
	glUseProgram(0); // no text support in GLSL
	float xf = (float) (2*x)/w-1, yf = (float) (2*y)/h-1;
	glColor3fv(&color.x);
	glRasterPos2f(xf, yf);
	glutBitmapString(font, (unsigned char*) text);
	glUseProgram(program);
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

void Text(vec3 &p, mat4 &m, int xoff, int yoff, vec3 &color, char *format, ...) {
    char buf[500];
	vec2 s = ScreenPoint(p, m);
    FormatString(buf, 500, format);
	Text((int) s.x+xoff, (int) s.y+yoff, color, buf);
}

void Text(vec3 &p, mat4 &m, char *text, vec3 &color) {
	Text(p, m, 0, 0, color, text);
}

void Text(vec3 &p, mat4 &m, vec3 &color, char *format, ...) {
    char buf[500];
    FormatString(buf, 500, format);
	Text(p, m, buf, color);
}

// Keyboard

static int nShortBits = 8*sizeof(SHORT);
static SHORT shortMSB = 1 << (nShortBits-1);

bool KeyUp(int button) { return (GetKeyState(button) & shortMSB) == 0; }

bool KeyDown(int c) { return (GetKeyState(c) & shortMSB) != 0; }

// Draw

static char *vertexShader = "\
	#version 130								\n\
	in vec3 point;								\n\
    uniform mat4 view;							\n\
	void main()									\n\
	{											\n\
		gl_Position = view*vec4(point, 1);		\n\
	}											\n";

static char *pixelShader = "\
	#version 130								\n\
	uniform float opacity = 1;					\n\
	uniform vec3 color = vec3(1);				\n\
	out vec4 pColor;							\n\
	void main()									\n\
	{											\n\
	    pColor = vec4(color, opacity);			\n\
	}											\n";

GLuint drawShader = 0, drawBuffer = 0;
static vec3 blk(0), wht(1), offWht(.95f), ltGry(.9f), mdGry(.6f), dkGry(.4f);

void CheckDrawBuffer() {
	if (!drawBuffer) {
		glGenBuffers(1, &drawBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, drawBuffer);
		glBufferData(GL_ARRAY_BUFFER, 4*sizeof(vec3), NULL, GL_STATIC_DRAW);
	}
}

int UseDrawShader() {
	int current = GLSL::CurrentShader();
	if (!drawShader)
		drawShader = InitShader(vertexShader, pixelShader);
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

bool DashOn() {
	GLboolean on = glIsEnabled(GL_LINE_STIPPLE);
	glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 3855); // on 4 bits / off 4 bits / on 4 bits / off 4 bits
	return on != 0;
}

void DashOff() { glDisable(GL_LINE_STIPPLE); }

// Disk

void Disk(vec3 &point, float diameter, vec3 &color, float opacity) {
	CheckDrawBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, drawBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3), &point.x);
	GLSL::VertexAttribPointer(drawShader, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::SetUniform(drawShader, "opacity", opacity);
	GLSL::SetUniform(drawShader, "color", color);
	glPointSize(diameter);
	glDrawArrays(GL_POINTS, 0, 1);
}

// Line

void Line(vec3 &p1, vec3 &p2, vec3 &color, float opacity, float width, bool dashed) {
	bool was = dashed? DashOn() : false;
	dashed? DashOn() : void();
	float w;
	glGetFloatv(GL_LINE_WIDTH, &w);
	glLineWidth(width);
	int current = UseDrawShader();
	CheckDrawBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, drawBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3), &p1.x);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3), sizeof(vec3), &p2.x);
	GLSL::VertexAttribPointer(drawShader, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::SetUniform(drawShader, "color", color);
	GLSL::SetUniform(drawShader, "opacity", opacity);
	glDrawArrays(GL_LINES, 0, 2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(current);
	if (!was && dashed)
		DashOff();
	glLineWidth(w);
}

void Line(int x1, int y1, int x2, int y2, vec3 &color, float opacity) {
	Line(vec3((float) x1, (float) y1, 0), vec3((float) x2, (float) y2, 0), color, opacity);
}

// Misc

void Line(float x1, float y1, float x2, float y2, vec3 &color, float opacity) {
	Line(vec3(x1, y1, 0), vec3(x2, y2, 0), color, opacity);
}

vec2 UnitPoint(float radians) { return vec2(cos(radians), sin(radians)); }

void Circle(vec2 &p, float radius, vec3 &color) {
	vec2 p1 = p+radius*UnitPoint(11.f/12.f);
    for (int i = 0; i < 12; i++) {
		float a1 = (float) i / 12, a2 = (float) (i+1) / 12;
		vec2 p1 = p+radius*UnitPoint(2*3.1415f*a1);
		vec2 p2 = p+radius*UnitPoint(2*3.1415f*a2);
		Line(p1.x, p1.y, p2.x, p2.y, color);
		p1 = p2;
    }
}

void Crosshairs(vec2 &s, float radius, vec3 &color) {
    float innerRad = .4f*radius;
	Circle(s, .5f, color);
    Circle(s, 2*innerRad, color);
    Line(s.x-innerRad, s.y, s.x-radius, s.y, color);
    Line(s.x+innerRad, s.y, s.x+radius, s.y, color);
    Line(s.x, s.y-innerRad, s.x, s.y-radius, color);
    Line(s.x, s.y+innerRad, s.x, s.y+radius, color);
}

void Sun(vec2 &p, vec3 *flashColor) {
	vec3 c(p.x, p.y, 0);
	vec3 yel(1, 1, 0), red(1, 0, 0), *col = flashColor? flashColor : &red;
	// wish small yellow on larger red disk regardless of z-buffer
    Disk(c, 8, yel);
    Disk(c, 12, *col);
    Disk(c, 8, yel);
	glLineWidth(1.);
    for (int r = 0, nRays = 16; r < nRays; r++) {
        float a = 2*3.141592f*(float)r/(nRays-1), dx = cos(a), dy = sin(a);
        float len = 11*(r%2? 1.8f : 2.5f);
        Line(vec3(p.x+9*dx, p.y+9*dy, 0), vec3(p.x+len*dx, p.y+len*dy, 0), *col);
    }
}

// Quad

void Quad(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, vec3 &color, float opacity) {
	int current = UseDrawShader();
	vec3 points[] = {p1, p2, p3, p4};
	CheckDrawBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, drawBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4*sizeof(vec3), points);
	GLSL::VertexAttribPointer(drawShader, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::SetUniform(drawShader, "color", color);
	GLSL::SetUniform(drawShader, "opacity", opacity);
	glDrawArrays(GL_QUADS, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(current);
}

void Rectangle(int x, int y, int w, int h, vec3 &color, bool solid, float opacity) {
	float x1 = (float) x, x2 = (float) (x+w), y1 = (float) y, y2 = (float) (y+h);
	if (solid)
		Quad(vec3(x1, y1, 0), vec3(x2, y1, 0), vec3(x2, y2, 0), vec3(x1, y2, 0), color, opacity);
	else {
		float linewidth;
		glGetFloatv(GL_LINE_WIDTH, &linewidth);
		float halfw = .5f*linewidth;
		Line(vec3(x1-halfw, y1, 0), vec3(x2+halfw, y1, 0), color, opacity);
		Line(vec3(x2, y1-halfw, 0), vec3(x2, y2+halfw, 0), color, opacity);
		Line(vec3(x1-halfw, y2, 0), vec3(x2+halfw, y2, 0), color, opacity);
		Line(vec3(x1, y1-halfw, 0), vec3(x1, y2+halfw, 0), color, opacity);
	}
}

// Buttons

Button::Button() { InitRectangle(0, 0, 0, 0, NULL); }

Button::Button(int x, int y, int w, int h, char *name, vec3 *bgrndCol, vec3 *txtCol) {
	InitRectangle(x, y, w, h, name, bgrndCol, txtCol);
}

Button::Button(int x, int y, int size, char *name, bool *value, vec3 *txtCol) {
	InitCheckbox(x, y, size, name, value, txtCol);
}

void InitButton(Button *b, char *name, vec3 *bgrndCol, vec3 *txtCol) {
	b->backgroundColor = bgrndCol? *bgrndCol : vec3(1);
	b->textColor = txtCol? *txtCol : vec3(0);
	b->textWidth = -1;
	b->font = GLUT_BITMAP_9_BY_15;
	name? b->name = string(name) : void();
}

void Button::InitCheckbox(int x, int y, int size, char *name, bool *valueA, vec3 *txtCol) {
	this->x = x;
	this->y = y;
	w = h = size;
	winW = -1;
	type = B_Checkbox;
	value = valueA;
	InitButton(this, name, NULL, txtCol);
}

void Button::InitRectangle(int ax, int ay, int aw, int ah, char *name, vec3 *bgrndCol, vec3 *txtCol) {
	textWidth = -1;
	font = GLUT_BITMAP_9_BY_15;
	x = ax;
	y = ay;
	w = aw;
	h = ah;
	type = B_Rectangle;
	winW = -1;
	InitButton(this, name, bgrndCol, txtCol);
}

void Button::CenterText(int x, int y, char *text, int buttonWidth, vec3 &color) {
	int size = 9; // FontSize(GLUT_BITMAP_9_BY_15);
	int xoff = (buttonWidth-size*strlen(text))/2;
	PutString(x+xoff, y, text, color);
}

void Button::SetTextColor(vec3 &col) { textColor = col; } // memcpy(textColor, col, 3*sizeof(float)); }

void Button::ShowName(char *name, vec3 &col) {
	float yDotOffset = font == GLUT_BITMAP_TIMES_ROMAN_24? 6.5f : 4.f;
	float yRectOffset = font == GLUT_BITMAP_TIMES_ROMAN_24? 4.f : 6.f;
	int xpos = type == B_Rectangle? x+5 : x+w;
	int ypos = type == B_Checkbox? y-(int)yDotOffset : y+(int)yRectOffset;
	int nchars = strlen(name);
	int size = 9; // FontSize(font);
	bool singleLine = size*nchars <= w;
	if (singleLine && h > 30)
		ypos += (h-20)/2;
	if (type == B_Checkbox) {
		char *cr = strchr(name, '\n'), buf[100];
		int len = cr? cr-name : 0;
		if (len) {
			strncpy(buf, name, len);
			buf[len] = 0;
			PutString(xpos, ypos+9, buf, col);
			PutString(xpos, ypos-5, cr+1, col);
			if (textWidth < 0) {
				int line1 = len, line2 = nchars-len-1;
				textWidth = 9*(line1 > line2? line1 : line2);
			}
		}
		else
			PutString(xpos, ypos, name, col);
	}
	else if (type == B_Rectangle && singleLine)
	    CenterText(x, ypos, name, w, col);
	else {
		char *sp = strchr(name, ' '), buf[100];
		int len = sp? sp-name : w/size;
		strncpy(buf, name, len);
		buf[len] = 0;
		if (sp != NULL) {
			// multi-line
			CenterText(x, ypos+15, buf, w, col);
			if (this->h >= 30) {
				char *s = name+len;
				while (*s == ' ')
					s++;
				CenterText(x, ypos, s, w, col);
			}
		}
		else
			CenterText(x, ypos+45, name, w, col);
	}
	if (textWidth < 0)
		textWidth = size*strlen(name);
}

void Button::Draw(vec3 *statusColor) { Draw(NULL, statusColor); }

void Button::Draw(char *nameOverride, vec3 *statusColor) {
	if (winW < 0) {
		winW = glutGet(GLUT_WINDOW_WIDTH);
		winH = glutGet(GLUT_WINDOW_HEIGHT);
	}
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	if (type == B_Rectangle) {
		float c[] = {1,0,0};
		Rectangle(x, y, w, h, offWht, true);
		statusColor? 
			Rectangle(x, y, w, h, mdGry) : 
			Rectangle(x, y, w, h, ltGry);
	}
	if (type == B_Checkbox) {
		int xx = x-5, yy = y-7, xEnd = xx+w, yEnd = yy+h;
		Rectangle(xx, yy, w+1, h+1, wht);
		Rectangle(xx, yy+1, 1, h-1, mdGry);
		Rectangle(xx, yEnd, w, 1, mdGry);
		Rectangle(xx+1, yy+2, 1, h-3, dkGry);
		Rectangle(xx+1, yEnd-1, w-2, 1, dkGry);
		Rectangle(xx+1, yy+1, w-2, 1, ltGry);
		Rectangle(xEnd-1, yy+1, 1, h-1, ltGry);
		if (statusColor) {
			int checkMark[7][7] = {0,0,1,0,0,0,0,
								   0,1,1,1,0,0,0,
								   1,1,1,1,1,0,0,
								   1,1,0,1,1,1,0,
								   1,0,0,0,1,1,1,
								   0,0,0,0,0,1,1,
								   0,0,0,0,0,0,1};
			for (int row = 0; row < 7; row++)
				for (int col = 0; col < 7; col++)
					if (checkMark[row][col] == 1) {
						int ix = xx+4+col, iy = yy+5+row;
						Rectangle(ix, iy, 1, 1, blk);
					}
		}
	}
	ShowName(nameOverride? nameOverride : (char *) name.c_str(), textColor);
}

void Button::Highlight() {
	Rectangle(x, y, w, h, wht, true, .5f);
	glLineWidth(1.f);
	float x1 = (float)x, x2 = x1+(float)w, y1 = (float)y, y2 = y1+(float)h;
	Line(vec3(x1+1, y1+1.5f, 0), vec3(x2-1, y1+1.5f, 0), blk, 1);
	Line(vec3(x2-1.5f, y2-1, 0), vec3(x2-1.5f, y1+1, 0), blk, 1);
}

bool Button::Hit(int ax, int ay) {
	if (type == B_Rectangle)
	    return ax >= x && ax <= x+w && ay >= y && ay <= y+h;
	if (type == B_Checkbox)
		return  ax >= x-5 && ax <= (textWidth < 0? x-5+w : x-5+w+textWidth+7) && ay >= y-7 && ay <= y-7+w;
	return false;
}

bool Button::UpHit(int ax, int ay, int state) {
	bool hit = Hit(ax, ay);
	if (hit && type == B_Checkbox && state == GLUT_UP && value != NULL)
		*value = !*value;
	return hit;
}

// Sliders

Slider::Slider() {
	x = y = size = 0;
	color[0] = color[1] = color[2] = 0;
	winW = -1;
	Init(0, 0, 80, 0, 1, .5f, true, NULL, &color);
}

Slider::Slider(int x, int y, int size, float min, float max, float init, bool v, char *nameA, vec3 *col) {
	Init(x, y, size, min, max, init, v, nameA, col);
}

int Round(float f)  { return (int)(f < 0.? ceil(f-.5f) : floor(f+.5f)); }

void Slider::Init(int x, int y, int size, float min, float max, float init, bool v, char *nameA, vec3 *col) {
		this->x = x;
		this->y = y;
		this->size = size;
		if (nameA)
			name = string(nameA);
		vertical = v;
		SetRange(min, max, init);
		color[0] = color[1] = color[2] = 1;
		if (col)
			memcpy(color, col, 3*sizeof(float));
		else
			color[0] = color[1] = color[2] = 0;
	    winW = -1;
		int off = vertical? y : x;
		loc = Round(off+(float)(init-min)/(max-min)*size);
}

void Slider::SetValue(float val) {
	int off = vertical? y : x;
    loc = Round(off+(float)(val-min)/(max-min)*size);
}		

void Slider::SetRange(float min, float max, float init) {
	this->min = min;
	this->max = max;
	int off = vertical? y : x;
    loc = Round(off+(float)((init-min)/(max-min))*size);
}	

void Slider::Draw(char *nameOverride, vec3 *sliderColor) {
	//vec3 *sCol = sliderColor? sliderColor : &color;
	//glLineWidth(2);
	int iloc = (int) loc;
	float grays[] = {160, 105, 227, 255};
	if (vertical) {
		for (int i = 0; i < 4; i++)
			Rectangle(x-1+i, y, 1, size, vec3(grays[i]/255.f));
		Rectangle(x-1, y, 4, 1, wht);
		Rectangle(x, y+1, 1, 1, ltGry);
		Rectangle(x-1, y+size-1, 3, 1, mdGry);
		// slider
		Rectangle(x-10, iloc-3, 20, 7, offWht); // whole knob
		Rectangle(x-10, iloc-3, 20, 1, dkGry); // bottom
		Rectangle(x+10, iloc-3, 1, 7, dkGry);  // right
		Rectangle(x-10, iloc-2, 1, 6, wht);     // left
		Rectangle(x-10, iloc+3, 20, 1, wht);    // top
		Rectangle(x-9, iloc-1, 1, 4, ltGry);   // 1 pixel right of left
		Rectangle(x-9, iloc+2, 18, 1, ltGry);  // 1 pixel below top
		Rectangle(x-9, iloc-2, 18, 1, mdGry);  // 1 pixel above bottom
		Rectangle(x+9, iloc-2, 1, 5, mdGry);   // 1 pixel left of right
	}
	else {
		for (int i = 0; i < 4; i++)
			Rectangle(x, y-i+1, size, 1, vec3(grays[i]/255.f));
		Rectangle(x+size-1, y-1, 1, 3, wht);
		Rectangle(x+size-2, y, 1, 1, ltGry);
		Rectangle(x, y-1, 1, 3, mdGry);
		// slider
		Rectangle(iloc-3, y-10, 7, 20, offWht);
		Rectangle(iloc+3, y-9, 1, 20, dkGry);
		Rectangle(iloc-3, y-9, 1, 19, wht);
		Rectangle(iloc-3, y+10, 6, 1, wht);
		Rectangle(iloc-3, y-10, 7, 1, dkGry);
		Rectangle(iloc-2, y-9, 1, 18, ltGry);
		Rectangle(iloc+2, y-9, 1, 19, mdGry);
		Rectangle(iloc-2, y+9, 4, 1, ltGry);
		Rectangle(iloc-2, y-9, 5, 1, mdGry);
	}
	if (nameOverride || !name.empty()) {
		const char *s = nameOverride? nameOverride : name.c_str();
		if (winW == -1) {
			winW = glutGet(GLUT_WINDOW_WIDTH);
			winH = glutGet(GLUT_WINDOW_HEIGHT);
		}
		char buf[100], num[100];
		float val = GetValue();
		vec3 col(color[0], color[1], color[2]);
		char *start = val < 1? buf+1 : buf; // skip leading zero
		if (!vertical) {
			sprintf(num, val >= 1? "%3.2f" : val < .001? "%4.4f" : "%3.3f", val);
			sprintf(buf, "%s: %s", s, val < 1? num+1 : num); // +1: skip zero
			PutString(x+size+8, y-2, buf, col);
		}
		else {
			sprintf(buf, val >= 1? "%3.2f" : "%3.3f", val);
			int wName = 1+glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) s);
			int wBuf = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) start);
			PutString(x-wName/2, y+size+6, s, col);
			PutString(x+1-wBuf/2, y-15, start, col);
		}
	}
}

float Slider::GetValue() {
	int ref = vertical? y : x;
	return min+((float)(loc-ref)/size)*(max-min);
}

bool Slider::Hit(int xx, int yy) {
    return !vertical?
		xx >= x && xx <= x+size && yy >= y-10 && yy <= y+10 :
		xx >= x-16 && xx <= x+16 && yy >= y-32 && yy <= y+size+27;
}

bool Slider::Mouse(int ax, int ay) {
	// snap to mouse location
	int old = loc;
	int mouse = !vertical? ax : ay;
	int min = !vertical? x : y;
	loc = mouse < min? min : mouse > min+size? min+size : mouse;
	return old != loc;
}

// Mover

float DotProduct(float a[], float b[]) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

void SetPlane(vec3 &p, int x, int y, mat4 &modelview, mat4 *persp, float *plane) {
	vec3 p1, p2;									// two points that transform to pixel x,y
	mat4 ident;
	ScreenLine((float) x, (float) y, modelview, persp? *persp : ident, (float *) &p1, (float *) &p2);
	for (int i = 0; i < 3; i++)
		plane[i] = p2[i]-p1[i];						// set plane normal to that of line p1p2
	plane[3] = -plane[0]*p.x-plane[1]*p.y-plane[2]*p.z;
		// pass plane through point
}

void Mover::SetPlane(int x, int y, mat4 &modelview, mat4 *persp) { ::SetPlane(*point, x, y, modelview, persp, plane); }

void Mover::Set(vec3 *p) { point = p; }

void Mover::Down(int x, int y, mat4 &modelview, mat4 *persp) { SetPlane(x, y, modelview, persp); }

void Mover::Drag(int x, int y, mat4 &modelview, mat4 *persp) {
    float p1[3], p2[3], axis[3];
	mat4 ident;
    ScreenLine((float) x, (float) y, modelview, persp? *persp : ident, p1, p2);
        // get two points that transform to pixel x,y
    for (int i = 0; i < 3; i++)
        axis[i] = p2[i]-p1[i];
        // direction of line through p1
    float pdDot = DotProduct(axis, plane);
        // project onto plane normal
    float a = (-plane[3]-DotProduct(p1, plane))/pdDot;
    for (int j = 0; j < 3; j++)
        (*point)[j] = p1[j]+a*axis[j];
        // intersection of line with plane
}

bool Mover::Hit(int x, int y, mat4 &view) { return point? ScreenDistSq(x, y, *point, view) < 100 : false; }
