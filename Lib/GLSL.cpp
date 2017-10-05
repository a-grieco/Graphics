/*  ===========================================
    GLSL.cpp - GLSL support
    Copyright (c) Jules Bloomenthal, 2011, 2012
    All rights reserved
    ===========================================
*/

#include "GLSL.h"

// Support

GLuint InitShader(const char *vertexShader, const char *fragmentShader, const char *geometryShader) {
	GLint status = GL_FALSE;
	int id = GLSL::LinkProgramViaCode(vertexShader, fragmentShader, geometryShader);
	if (id)
    	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		printf("failed to link\n");
		id = 0;
	}
	// else {
   	//	  GLSL::PrintProgramAttributes(id);
	//    GLSL::PrintProgramUniforms(id);
	// }
	return id;
}

int Errors() {
    char buf[1000];
    int nErrors = 0;
    buf[0] = 0;
    for (;;) {
        GLenum n = glGetError();
        if (n == GL_NO_ERROR)
            break;
        sprintf(buf+strlen(buf), "%s%s", !nErrors++? "" : ", ", gluErrorString(n));
            // do not call Debug() while looping through errors, so accumulate in buf
    }
    printf("%s\n", nErrors? buf : "no GL errors");
    return nErrors;
}

// Print OpenGL, GLSL Details

void GLSL::PrintVersionInfo() {
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *version     = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GL vendor: %s\n", vendor);
    printf("GL renderer: %s\n", renderer);
    printf("GL version: %s\n", version);
    printf("GLSL version: %s\n", glslVersion);
 // GLint major, minor;
 // glGetIntegerv(GL_MAJOR_VERSION, &major);
 // glGetIntegerv(GL_MINOR_VERSION, &minor);
 // printf("GL version (integer): %d.%d\n", major, minor);
}

void GLSL::PrintExtensions() {
    const GLubyte *extensions = glGetString(GL_EXTENSIONS);
    char *skip = "(, \t\n", buf[100];
    printf("\nGL extensions:\n");
		if (extensions)
			for (char *c = (char *) extensions; *c; ) {
					c += strspn(c, skip);
					int nchars = strcspn(c, skip);
					strncpy(buf, c, nchars);
					buf[nchars] = 0;
					printf("  %s\n", buf);
					c += nchars;
	}
}

void GLSL::PrintProgramLog(int programID) {
    GLint logLen;
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 0) {
        char *log = new char[logLen];
        GLsizei written;
        glGetProgramInfoLog(programID, logLen, &written, log);
        printf("Program log\n%s", log);
        delete [] log;
    }
}

void GLSL::PrintProgramAttributes(int programID) {
    GLint maxLength, nAttribs;
    glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTES, &nAttribs);
    glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
    char *name = new char[maxLength];
    GLint written, size;
    GLenum type;
	printf("  %i attributes\n", nAttribs);
    for (int i = 0; i < nAttribs; i++) {
        glGetActiveAttrib(programID, i, maxLength, &written, &size, &type, name);
        GLint location = glGetAttribLocation(programID, name);
        printf("    %-5i  |  %s\n", location, name);
    }
    delete [] name;
}

void GLSL::PrintProgramUniforms(int programID) {
    GLenum type;
    GLchar name[201];
    GLint nUniforms, length, size;
    glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &nUniforms);
    printf("  %i uniforms\n", nUniforms);
    for (int i = 0; i < nUniforms; i++) {
        glGetActiveUniform(programID, i, 200, &length, &size, &type, name);
        printf("    %s\n", name);
    }
}

// Compilation

int GLSL::CompileShaderViaFile(const char *filename, GLint type) {
	FILE* fp = fopen(filename, "r");
	if (fp == NULL)
		return 0;
	char buf[10000];
	for (char *c = buf;; c++) {
		*c = fgetc(fp);
		if (*c == EOF) {
			*c = NULL;
			break;
		}
	}
	fclose(fp);
	return CompileShaderViaCode(buf, type);
}

int GLSL::CompileShaderViaCode(const char *code, GLint type) {
    GLuint shader = glCreateShader(type);
	if (!shader) {
		Errors();
		return false;
	}
    glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
    // check compile status
    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        // report logged errors
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            GLsizei written;
            char *log = new char[logLen];
            glGetShaderInfoLog(shader, logLen, &written, log);
            printf(log);
            delete [] log;
        }
        else
            printf("shader compilation failed\n");
        return 0;
    }
    // const char *t = type == GL_VERTEX_SHADER? "vertex" : "fragment";
    // printf("  %s shader (%i) successfully compiled\n", t, shader);
    return shader;
}

// Linking

int GLSL::LinkProgramViaFile(const char *vertexShaderFile, const char *fragmentShaderFile) {
	int vshader = CompileShaderViaFile(vertexShaderFile, GL_VERTEX_SHADER);
	int fshader = CompileShaderViaFile(fragmentShaderFile, GL_FRAGMENT_SHADER);
	return LinkProgram(vshader, fshader);
}

int GLSL::LinkProgramViaCode(const char *vertexShaderCode,
	                         const char *fragmentShaderCode,
							 const char *geometryShaderCode) {
	int vshader = CompileShaderViaCode(vertexShaderCode, GL_VERTEX_SHADER);
	int fshader = CompileShaderViaCode(fragmentShaderCode, GL_FRAGMENT_SHADER);
	int gshader = -1;
	if (geometryShaderCode)
		gshader = CompileShaderViaCode(geometryShaderCode, GL_GEOMETRY_SHADER);
	return LinkProgram(vshader, fshader, gshader);
}

int GLSL::LinkProgram(int vshader, int fshader, int gshader) {
    int programID = 0;
    // create shader program
    if (vshader && fshader)
        programID = glCreateProgram();
    if (programID > 0) {
        // attach shaders to program
        glAttachShader(programID, vshader);
        glAttachShader(programID, fshader);
		if (gshader >= 0)
			glAttachShader(programID, gshader);
        // link and verify
        glLinkProgram(programID);
        GLint status;
        glGetProgramiv(programID, GL_LINK_STATUS, &status);
        // if (status == GL_FALSE)
        PrintProgramLog(programID);
	    if (status == GL_TRUE) {
			PrintProgramAttributes(programID);
			PrintProgramUniforms(programID);
		}
    }
    return programID;
}

int GLSL::CurrentShader() {
	int shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	return shader;
}

bool Error(const char *name) {
#ifdef _DEBUG
	printf("can't find shader variable %s\n", name);
#endif
	return false;
}

// Uniform Access

bool GLSL::SetUniform(int shader, const char *name, int val) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform1i(id, val);
	return true;
}

bool GLSL::SetUniformv(int shader, const char *name, int count, int *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform1iv(id, count, v);
	return true;
}

bool GLSL::SetUniformv(int shader, const char *name, int count, float *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform1fv(id, count, v);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, float val) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform1f(id, val);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, vec2 v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform2f(id, v.x, v.y);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, vec3 v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform3f(id, v.x, v.y, v.z);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, vec4 v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform4f(id, v.x, v.y, v.z, v.w);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, vec3 *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform3fv(id, 1, (float *) v);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, vec4 *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform4fv(id, 1, (float *) v);
	return true;
}

bool GLSL::SetUniform3(int shader, const char *name, float *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform3fv(id, 1, v);
	return true;
}

bool GLSL::SetUniform3v(int shader, const char *name, int count, float *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform3fv(id, count, v);
	return true;
}

bool GLSL::SetUniform4v(int shader, const char *name, int count, float *v) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniform4fv(id, count, v);
	return true;
}

bool GLSL::SetUniform(int shader, const char *name, mat4 m) {
	GLint id = glGetUniformLocation(shader, name);
	if (id < 0)
		return Error(name);
	glUniformMatrix4fv(id, 1, true, (float *) &m[0][0]);
	return true;
}

// Attribute Access

void GLSL::DisableVertexAttribute(int shader, const char *name) {
	GLint id = glGetAttribLocation(shader, name);
	if (id >= 0)
		glDisableVertexAttribArray(id);
	else
		Error(name);
}

int GLSL::EnableVertexAttribute(int shader, const char *name) {
	GLint id = glGetAttribLocation(shader, name);
	if (id >= 0)
		glEnableVertexAttribArray(id);
	else
		Error(name);
	return id;
}

void GLSL::VertexAttribPointer(int shader, const char *name, GLint ncomponents, GLenum datatype,
							   GLboolean normalized, GLsizei stride, const GLvoid *pointer) {
	GLuint id = GLSL::EnableVertexAttribute(shader, name);
    glVertexAttribPointer(id, ncomponents, datatype, normalized, stride, pointer);
}
