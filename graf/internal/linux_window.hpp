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

#pragma once

#include "graf/graf.hpp"

#ifdef LIGHT_PLATFORM_LINUX

#include "light/diagnostics/errors.hpp"
#include "light/utility/non_copyable.hpp"

#include <memory>
#include <X11/Xlib.h>
#include <GL/glx.h>


namespace graf
{
namespace internal
{
	//=============================================================================================
	// A custom deleter for XLib resources that are freed via XFree()
	//=============================================================================================
	struct xlib_deleter
	{
		void operator () (void *p) { XFree(p); }
	};

	template<typename Resource>
	using xlib_ptr = ::std::unique_ptr<Resource, xlib_deleter>;


	//=========================================================================================
	// Error handling
	// Most functions of the XLib don't return an error code because they do their work
	// asynchronous. That's why there is a callback function that is called for every protocol
	// error (for errors concerning the connection to the X server there is another callback
	// function) that occurs during execution. Since we can't throw exceptions from the callback
	// (it is called from a different module) and setjmp/longjmp don't work well with C++ (no
	// stack-unwinding-> no destructors called) we have to introduce this disgusting global state
	// variable that holds the error.
	//=========================================================================================
	extern struct xlib_error
	{
		enum { buffer_size = 1024 };

		unsigned char code;
		char description[buffer_size];
	} g_error;

	// Should be called after each XLib function call, but at least after each flush/sync
	void check_for_errors();


	//=============================================================================================
	// The X Window System has a client-server architecture. The Display represents the connection
	// between the client (the application) and the X Server. Only the X Server has access to the
	// drawing area and the input channel. Clients can send requests (like creating a window,
	// drawing a line) to the X Server over a communication channel that uses the X Protocol. The
	// X Protocol itself is send via TCP/IP or any other protocol that is available between server
	// and client.
	// A set of screens for a single user with one keyboard and one pointer (usually a mouse) is
	// called a display.
	//
	// See: http://www.sbin.org/doc/Xlib/
	//=============================================================================================
	class x_screen : light::non_copyable
	{
	public:
		x_screen() :
			// Open connection to the X-server. Since display_name = nullptr, this call connects
			// to the display specified in the environment variable DISPLAY.
			m_display(XOpenDisplay(nullptr)),
			// Get the default screen
			m_screen(DefaultScreen(display())),
			// Get screen dimension
			m_width(DisplayWidth(display(), screen())),
			m_height(DisplayHeight(display(), screen()))
		{
			if(!display())
				throw light::runtime_error("Cannot open display");
		}

		~x_screen()
		{
			// Closes the connection to the X server and releases all resources (Windows, Cursors etc.)
			XCloseDisplay(display());
		}

		uint width() const { return m_width; }
		uint height() const { return m_height; }

		::Display* display() { return m_display; }
		int screen() { return m_screen; }

	private:
		// "A large structure that contains information about the server and its screens."
		::Display *m_display;
		// A display can have several screens. m_screen stores the ID of the screen we are drawing to
		int m_screen;
		// The dimension of the screen hold by m_screen
		uint m_width, m_height;
	};


	//=============================================================================================
	//
	//=============================================================================================
	class window_impl : light::non_copyable
	{
	public:
		// Constructor
		window_impl(utf8_unit const *_title, uint width, uint height, uint depth, uint stencil);

		// Sets the title
		void title(utf8_unit const *title);

		// Destructor
		~window_impl();

		// Processes events like keyboard input, mouse input and window resizing.
		// Returns false if the user has closed the window.
		bool process_events();

		// Get screen dimension
		uint screen_width() const { return m_screen.width(); }
		uint screen_height() const { return m_screen.height(); }

		// Swaps the backbuffer with the frontbuffer, so all your work becomes
		// visible.
		void swap_buffers();


		::Display* display() { return m_screen.display(); }
		int screen() { return m_screen.screen(); }
		Window window() { return m_window; }
		GLXFBConfig framebuffer_config() { return m_fb_config; }

	private:
		x_screen m_screen;

		// The window resource ID
		Window m_window;
		Atom m_atom_delete_window;

		GLXFBConfig m_fb_config;
	};

} // namespace: internal
} // namespace: graf


#endif // conditional compilation: LIGHT_PLATFORM_LINUX
