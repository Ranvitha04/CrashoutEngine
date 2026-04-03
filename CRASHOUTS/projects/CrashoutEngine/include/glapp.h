/**
@file    	glapp.h
@author  	pghali@digipen.edu
@co-author  parminder.singh@digipen.edu
@date    	05/05/2025

This file contains the declaration of class GLApp that encapsulates the
functionality required to create an OpenGL context using GLFW; use GLEW
to load OpenGL extensions; initialize OpenGL state; and finally initialize
the OpenGL application by calling initalization functions associated with
objects participating in the application.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*//*__________________________________________________________________________*/

/*                                                                      guard
----------------------------------------------------------------------------- */
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <GL/glew.h> // for access to OpenGL API declarations 
#include <GLFW/glfw3.h>
#include <string>
#include <vector>


// Log headers for logging
#include "../include/log.h"

/*  _________________________________________________________________________ */
struct GLApp
	/*! GLApp structure to encapsulate initialization stuff ...
	*/
{
	static bool init(GLint w, GLint h, std::string t);
	static void Update();
	static void draw();
	static void cleanup();

	// callbacks ...
	static void error_cb(int error, char const* description);
	static void fbsize_cb(GLFWwindow* ptr_win, int width, int height);
	// I/O callbacks ...
	static void key_cb(GLFWwindow* pwin, int key, int scancode, int action, int mod);
	static void mousebutton_cb(GLFWwindow* pwin, int button, int action, int mod);
	static void mousescroll_cb(GLFWwindow* pwin, double xoffset, double yoffset);
	static void mousepos_cb(GLFWwindow* pwin, double xpos, double ypos);

	static void update_time(double fpsCalcInt = 1.0);

	static void ToggleFullscreen();
	static void ToggleMaximize();
	static void Minimize();
	static void Show();

	// State tracking variables
	static bool is_fullscreen;
	static bool is_maximized;
	static int  windowed_x, windowed_y, windowed_w, windowed_h;

	static GLint width, height;
	static GLdouble fps;
	static GLdouble delta_time; // time taken to complete most recent game loop
	static std::string title;
	static GLFWwindow* ptr_window;
	static GLfloat clear_color[4]; // rgba normalize components in range 0.0f to 1.0f

	struct GLViewport {
		GLint x, y;
		GLsizei width, height;
	};

	static std::vector<GLViewport> vps;

};


