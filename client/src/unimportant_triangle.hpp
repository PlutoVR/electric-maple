// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Very simple shader program to display a triangle. Safe to remove once we have something more interesting to
 * render.
 * @author Moshi Turner <moses@collabora.com>
 */

#pragma once
#include "common.hpp"


GLuint
make_program()
{
	// Vertex shader program
	const char *vertex_shader_source =
	    "#version 300 es\n"
	    "in vec3 position;\n"
	    "in vec4 color;\n"
	    "out vec4 vertex_color;\n"
	    "void main()\n"
	    "{\n"
	    "    gl_Position = vec4(position, 1.0);\n"
	    "    vertex_color = color;\n"
	    "}\n";

	// Fragment shader program
	const char *fragment_shader_source =
	    "#version 300 es\n"
	    "precision mediump float;\n"
	    "in vec4 vertex_color;\n"
	    "out vec4 frag_color;\n"
	    "void main()\n"
	    "{\n"
	    "    frag_color = vertex_color;\n"
	    "}\n";


	// Create the vertex shader object
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	// Attach the vertex shader source code and compile it
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	// Check for any compile errors
	GLint success;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		U_LOG_E("Vertex shader compilation failed: %s\n", info_log);
	}

	// Create the fragment shader object
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach the fragment shader source code and compile it
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	// Check for any compile errors
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		U_LOG_E("Fragment shader compilation failed: %s\n", info_log);
	}

	// Create the shader program object
	GLuint shader_program = glCreateProgram();

	// Attach the vertex and fragment shaders to the program and link it
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	// Check for any link errors
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(shader_program, 512, NULL, info_log);
		U_LOG_E("Shader program linking failed: %s\n", info_log);
	}

	// Clean up the shader objects
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}


void
drawTriangle(GLuint shader_program)
{
	// Define the vertices of the triangle
	GLfloat vertices[] = {
	    0.0f,  0.5f,  0.0f, // top
	    -0.5f, -0.5f, 0.0f, // bottom left
	    0.5f,  -0.5f, 0.0f  // bottom right
	};

	// Define the colors of the vertices
	GLfloat colors[] = {
	    1.0f, 0.0f, 0.0f, 1.0f, // top vertex is red
	    0.0f, 1.0f, 0.0f, 1.0f, // bottom left vertex is green
	    0.0f, 0.0f, 1.0f, 1.0f  // bottom right vertex is blue
	};

	// Use the shader program
	glUseProgram(shader_program);

	// Create vertex buffer objects for the vertices and colors
	GLuint vbo[2];
	glGenBuffers(2, vbo);

	// Bind the vertex buffer for the vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind the vertex buffer for the colors
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

	// Enable the vertex attribute for the vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Enable the vertex attribute for the colors
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Draw the triangle
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Check for errors
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		U_LOG_E("OpenGL error: %d\n", error);
	}
}
