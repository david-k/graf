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
#include "light/string/string.hpp"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
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


	namespace
	{
		//=============================================================================================
		//
		//=============================================================================================
		xlib_ptr< ::XVisualInfo > get_visual_info(::Display *display, int screen,
									  uint depth_size, uint stencil_size)
		{
			// Holds the attributes we want our display+graphics card to have
			// See http://www.opengl.org/sdk/docs/man/xhtml/glXChooseFBConfig.xml
			int attributes[] =
			{
				GLX_X_RENDERABLE, True,              // Considers only framebuffer configs with an associated X visual
													 // (otherwise we wouldn't be able to render to the fb)
				GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,   // Specifies which GLX drawable types we want. Valid bits areGLX_WINDOW_BIT,
													 // GLX_PIXMAP_BIT, and GLX_PBUFFER_BIT. We only want to draw to the window.
				GLX_RENDER_TYPE, GLX_RGBA_BIT,       // Specifies the OpenGL rendering mode we want. Valid bits are GLX_RGBA_BIT
													 // and GLX_COLOR_INDEX_BIT.
				GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,   // Use true color mode
				GLX_RED_SIZE, 8,                     // Number...
				GLX_GREEN_SIZE, 8,                   // ...of bits...
				GLX_BLUE_SIZE, 8,                    // ...for each...
				GLX_ALPHA_SIZE, 8,                   // ...color
				GLX_DEPTH_SIZE, int(depth_size),     // Size of the depth buffer, in bits
				GLX_STENCIL_SIZE, int(stencil_size), // Size of the stencil buffer, in bits
				GLX_DOUBLEBUFFER, True,              // Use doublebuffering
				None
			};

			// The moment of truth: are the attributes we have chosen supported?
			int num_configs = 0;
			xlib_ptr< ::GLXFBConfig > configs(::glXChooseFBConfig(display, screen, attributes, &num_configs));
			if(!configs)
				throw light::runtime_error(light::str_printf("Desired configuration\n\t"
				                                                 "depth: {}\n\t"
				                                                 "stencil: {}\n"
				                                             "not supported", depth_size, stencil_size));

			// Just take the first configuration available. For the future: do something more
			// impressive (like choosing the *best* configuration...).
			::GLXFBConfig config = configs.get()[0];

			// Gets the visual associated with 'config'
			xlib_ptr< ::XVisualInfo > info(::glXGetVisualFromFBConfig(display, config));

			if(!info)
				throw light::runtime_error("Cannot get visual info from config");

			return info;
		}
	}


	//=============================================================================================
	//
	//=============================================================================================
	class window_impl : light::non_copyable
	{
	public:
		// Constructor
		window_impl(utf8_unit const *_title, uint width, uint height, uint depth, uint stencil) :
			m_screen()

		{
			xlib_ptr< ::XVisualInfo > visual = get_visual_info(display(), screen(), depth, stencil);

			// Set window attributes
			unsigned long attr_values = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;
			::XSetWindowAttributes attr;
			// Background color
			attr.background_pixel = BlackPixel(display(), screen());
			// Border color
			attr.border_pixel = BlackPixel(display(), screen());
			// Select the event types we want to receive
			attr.event_mask = ExposureMask |       // "Selects Expose events, which occur when the window is first displayed
			                                       // and whenever it becomes visible after being obscured. Expose events
			                                       // signal that the application should redraw itself."
			                  KeyPressMask |       // Events for pressing...
			                  KeyReleaseMask |     // ...and releasing keys
			                  ButtonPressMask |    // Events for pressing...
			                  ButtonReleaseMask |  // ...and releasing mouse buttons
			                  StructureNotifyMask; // Selects quite a few events, amongst others the resize event
			// Create colormap
			attr.colormap = XCreateColormap(display(), RootWindow(display(), screen()), visual->visual, AllocNone);

			// Create the window
			m_window = ::XCreateWindow(
				display(),                          // X Server connection
				RootWindow(display(), screen()),    // The parent window
				0, 0,                               // Position of the top-left corner
				width, height,                      // Hmmm...
				2,                                  // Width and of the border (has no effect (on my PC anyway))
				depth,
				InputOutput,                        // We need a window that receives input (events) and displays output (the rendered images)
				visual->visual,
				attr_values, &attr
			);

			// An Atom is the ID for a property. Properties enable you to associate arbitrary data with a window.
			// Here we query the Atom for the WM_DELETE_WINDOW property defined by the window manager
			atom_delete_window = XInternAtom(display(), "WM_DELETE_WINDOW", True);
			// Tells the window manager to send us a message if the user has closed the window
			XSetWMProtocols(display(), m_window, &atom_delete_window, 1);

			title(_title);

			// Displays the window
			XMapWindow(display(), m_window);

			// Flushes the ouput buffer (because X is network based it buffers the client's
			// requests for performance reasons, but in this case we want to be sure that the
			// window is mapped).
			XFlush(display());
		}

		// Sets the title
		void title(utf8_unit const *title)
		{
			XTextProperty text =
			{
				reinterpret_cast<unsigned char*>(const_cast<utf8_unit*>(title)),
				XInternAtom(display(), "UTF8_STRING", False),
				8,
				strlen(title)
			};

			// We can't use XStoreName if we want to support UTF-8 encoded titles (and we want that. UTF-8 FTW!)
			// XSetWMName is a shorthand for XSetTestProperty which is a shorthand for XChangeProperty
			XSetWMName(display(), m_window, &text);
		}

		// Destructor
		~window_impl()
		{
			// Not really necessary (all windows get destroyed when the connection is closed)
			// but I do it anyway.
			XDestroyWindow(display(), m_window);
		}

		// Processes events like keyboard input, mouse input and window resizing.
		// Returns false if the user has closed the window.
		bool process_events()
		{
			// XNextEvent gets the next event or blocks if the queue is empty. Since we want XNextEvent
			// to return immediately, we call it only if there are events waiting. To get the number
			// of events currently in the queue, we call XPending.
			while(XPending(display()))
			{
				XEvent event;
				XNextEvent(display(), &event);

				// Process event
				switch(event.type)
				{
					case ClientMessage:
						// If the window mamager has send us a WM_DELETE_WINDOW property we will tell the user
						// we are finished here.
						if(static_cast<Atom>(event.xclient.data.l[0]) == atom_delete_window)
							return false;
					break;
				}

			} // loop: while

			return true;
		}

		// Get screen dimension
		uint screen_width() const { return m_screen.width(); }
		uint screen_height() const { return m_screen.height(); }

	private:
		x_screen m_screen;

		// The window resource ID
		Window m_window;
		Atom atom_delete_window;

		// Convenience getter
		::Display* display() { return m_screen.display(); }
		int screen() { return m_screen.screen(); }
	};

} // namespace: internal
} // namespace: graf


#endif // conditional compilation: LIGHT_PLATFORM_LINUX
