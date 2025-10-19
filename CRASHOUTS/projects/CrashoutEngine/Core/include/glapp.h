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

*//*__________________________________________________________________________*/

/*                                                                      guard
----------------------------------------------------------------------------- */
#pragma once
#pragma warning(disable: 4251)
/*                                                                   includes
----------------------------------------------------------------------------- */
#include <GL/glew.h> // for access to OpenGL API declarations 
#include <GLFW/glfw3.h>
#include <string>
#include <vector>


// Log headers for logging
#include "ImportExport.h"
#include "../include/log.h"

/*  _________________________________________________________________________ */
struct GLApp
	/*! GLApp structure to encapsulate initialization stuff ...
	*/
{
	DLL_API static bool init(GLint w, GLint h, std::string t);
	DLL_API static void update();
	DLL_API static void draw();
	DLL_API static void cleanup();

	// callbacks ...
	DLL_API static void error_cb(int error, char const* description);
	DLL_API static void fbsize_cb(GLFWwindow* ptr_win, int width, int height);
	// I/O callbacks ...
	DLL_API static void key_cb(GLFWwindow* pwin, int key, int scancode, int action, int mod);
	DLL_API static void mousebutton_cb(GLFWwindow* pwin, int button, int action, int mod);
	DLL_API static void mousescroll_cb(GLFWwindow* pwin, double xoffset, double yoffset);
	DLL_API static void mousepos_cb(GLFWwindow* pwin, double xpos, double ypos);

	DLL_API static void update_time(double fpsCalcInt = 1.0);

	DLL_API static GLint width, height;
	DLL_API static GLdouble fps;
	DLL_API static GLdouble delta_time; // time taken to complete most recent game loop
	DLL_API static std::string title;
	DLL_API static GLFWwindow* ptr_window;
	static GLfloat clear_color[4]; // rgba normalize components in range 0.0f to 1.0f

	DLL_API struct GLViewport {
		GLint x, y;
		GLsizei width, height;
	};

	static std::vector<GLViewport> vps;

};

