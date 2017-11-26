/*	==============================
    Mesh.h - 3D mesh of triangles
    Copyright (c) Jules Bloomenthal, 2012-2016
    All rights reserved
	=============================== */

#ifndef MESH_HDR
#define MESH_HDR

#include <vector>
#include "mat.h"

using std::vector;

// STL format

struct VertexSTL {
	vec3 point, normal;
	VertexSTL() { }
	VertexSTL(float *p, float *n) : point(vec3(p[0], p[1], p[2])), normal(vec3(n[0], n[1], n[2])) { }
};

int ReadSTL(char *filename, vector<VertexSTL> &vertices);
	// return # triangles

// OBJ format

bool ReadAsciiObj(char          *filename,
				  vector<vec3>	&points,
				  vector<int3>	&triangles,
				  vector<vec3>	*normals  = NULL,
				  vector<vec2>	*textures = NULL,
				  vector<int>	*triangleGroups = NULL);
	// return true if successful

// Normals

void Normalize(vector<vec3> &points, float scale = 1);

void Normalize(vector<VertexSTL> &vertices, float scale = 1);
	// translate and apply uniform scale so that vertices fit in -1,1 in X,Y and 0,1 in Z

void SetVertexNormals(vector<vec3> &points, vector<int3> &triangles, vector<vec3> &normals);
	// compute/recompute vertex normals as the average of surrounding triangle normals

// Texture

char *ReadTexture(const char *filename, int &width, int &height, int &bitsPerPixel);

GLuint SetHeightfield(const char *filename, int whichTexture = 0);

#endif
