// TestGL.cpp: an application to determine GL and GLSL versions

#include "stdafx.h"
#include <iostream>
#include "glew.h"
#include "freeglut.h"

int main(int ac, char **av) {
	// establish stub GL rendering context
    glutInitWindowSize(1, 1);
    glutInitWindowPosition(0, 0);
    glutInit(&ac, av);
    glutCreateWindow(av[0]);
	// initialize OpenGL extension entry points
    GLenum err = glewInit();
	if (err != GLEW_OK)
        printf("Error initializing GLEW: %s\n", glewGetErrorString(err));
	// print GL info
    else {
		printf("GL vendor: %s\n", glGetString(GL_VENDOR));
		printf("GL renderer: %s\n", glGetString(GL_RENDERER));
		printf("GL version: %s\n", glGetString(GL_VERSION));
		printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	}
	// keep app open
    getchar();
    return 0;
}
