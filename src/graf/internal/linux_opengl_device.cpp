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

#include <graf/graf.hpp>

#ifdef LIGHT_PLATFORM_LINUX

#include <graf/internal/linux_opengl_device.hpp>
#include <graf/internal/linux_window.hpp>


namespace graf
{
namespace internal
{
	//=============================================================================================
	//
	//=============================================================================================
	opengl_device_impl::opengl_device_impl(window_impl *window) :
		m_window(window)
	{
		typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

		// Get the context creation function
		glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
			reinterpret_cast<glXCreateContextAttribsARBProc>(glXGetProcAddress(reinterpret_cast<GLubyte const*>("glXCreateContextAttribsARB")));

		// If the function doesn't exist it probaly means there is no OpenGL >= 3 available
		if(!glXCreateContextAttribsARB)
			throw light::runtime_error("\"glXCreateContextAttribsARB()\" not found. That probably means that OpenGL >= 3.0 is not available");

		// Attributes for the new context
		int context_attribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 3,
			None
		};

		m_context = glXCreateContextAttribsARB(m_window->display(), m_window->framebuffer_config(),
		                                       nullptr,           // No shared context
		                                       True,              // Enable direct rendering
		                                       context_attribs);

		XSync(m_window->display(), False);
		check_for_errors();

		glXMakeCurrent(m_window->display(), m_window->window(), m_context);
	}


	//=============================================================================================
	//
	//=============================================================================================
	opengl_device_impl::~opengl_device_impl()
	{
		glXMakeCurrent(m_window->display(), 0, 0);
		glXDestroyContext(m_window->display(), m_context);
	}


} // namespace: internal
} // namespace: graf


#endif // conditional compilation: LIGHT_PLATFORM_LINUX
