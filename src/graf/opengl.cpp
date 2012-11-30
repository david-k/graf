/**************************************************************************************************
 * graf library                                                                                   *
 * Copyright © 2012 David Kretzmer                                                                *
 *                                                                                                *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software  *
 * and associated documentation files (the "Software"), to deal in the Software without           *
 * restriction,including without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute,sublicense, and/or sell copies of the Software, and to permit persons to whom the   *
 * Software is furnished to do so, subject to the following conditions:                           *
 *                                                                                                *
 * The above copyright notice and this permission notice shall be included in all copies or       *
 * substantial portions of the Software.                                                          *
 *                                                                                                *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING  *
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND     *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        *
 *                                                                                                *
 *************************************************************************************************/

#include <graf/opengl.hpp>
#include <graf/window.hpp>
#include <graf/logger.hpp>

#ifdef LIGHT_PLATFORM_LINUX
	#include <graf/internal/linux_opengl_device.hpp>
#else
	#error Platform not supported yet
#endif

#include <GL3/gl3w.h>


namespace graf
{

	//=============================================================================================
	//
	//=============================================================================================
	opengl_device::opengl_device(window *win) :
		m_impl(new internal::opengl_device_impl(win->platform_impl()))
	{
		if(gl3wInit())
			throw light::runtime_error("Initializing gl3w failed");

		int major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		GRAF_INFO_MSG("OpenGL {}.{} context created\n", major, minor);
	}

	opengl_device::~opengl_device()
	{

	}


} // namespace: graf

