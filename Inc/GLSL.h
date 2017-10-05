/*  =====================================
    GLSL.h - GLSL support
    Copyright (c) Jules Bloomenthal, 2011
    All rights reserved
    =====================================
*/

#ifndef GLSL_HDR
#define GLSL_HDR

#include "mat.h"

GLuint InitShader(const char *vertexShader,
			      const char *fragmentShader,
			      const char *geometryShader = NULL);

namespace GLSL { // avoid name conflicts

// Print Info
void PrintVersionInfo();
void PrintExtensions();
void PrintProgramLog(int programID);
void PrintProgramAttributes(int programID);
void PrintProgramUniforms(int programID);

// Shader Compilation
int CompileShaderViaFile(const char *filename, int type);
int CompileShaderViaCode(const char *code, int type);

// Program Linking
int LinkProgramViaFile(const char *vertexShaderFile, const char *fragmentShaderFile);
int LinkProgramViaCode(const char *vertexShaderCode, const char *fragmentShaderCode, const char *geometryShaderCode = NULL);
int LinkProgram(int vshader, int fshader, int gshader = -1);
int CurrentShader();

// Uniform Access
//     if in debug mode, print any failure to find uniform
bool SetUniform(int shader, const char *name, int val);
bool SetUniformv(int shader, const char *name, int count, int *v);
bool SetUniformv(int shader, const char *name, int count, float *v);
bool SetUniform(int shader, const char *name, float val);
bool SetUniform(int shader, const char *name, vec2 v);
bool SetUniform(int shader, const char *name, vec3 v);
bool SetUniform(int shader, const char *name, vec4 v);
bool SetUniform(int shader, const char *name, vec3 *v);
bool SetUniform(int shader, const char *name, vec4 *v);
bool SetUniform3(int shader, const char *name, float *v);
bool SetUniform3v(int shader, const char *name, int count, float *v);
bool SetUniform4v(int shader, const char *name, int count, float *v);
bool SetUniform(int shader, const char *name, mat4 m);

// Attribute Access
//     if in debug mode, print any failure to find attribute
int EnableVertexAttribute(int shader, const char *name);
	// find named attribute and enable
void DisableVertexAttribute(int shader, const char *name);
	// find named attribute and disable
void VertexAttribPointer(int shader, const char *name, GLint ncomponents, GLenum datatype,
						 GLboolean normalized, GLsizei stride, const GLvoid *pointer);
	// convenience routine to find and set named attribute

} // end namespace GLSL

#endif
