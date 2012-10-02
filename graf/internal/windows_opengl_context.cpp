/* Kaos Framework - David Kretzmer */

#include "graf/graf.hpp"

#ifdef LIGHT_PLATFORM_WINDOWS

#define GLEW_STATIC
#include "GL/glew.h"

#include "kaos/graphics/OpenGL.hpp"
//#include "GL3/gl3w.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// GL3W does not provide any functionality for WGL, so some constants must be defined by hand.
// See http://www.opengl.org/registry/, especially http://www.opengl.org/registry/api/wglenum.spec.
///////////////////////////////////////////////////////////////////////////////////////////////////

// WGL_ARB_pixel_format
unsigned int const WGL_NUMBER_PIXEL_FORMATS_ARB = 0x2000;
unsigned int const WGL_DRAW_TO_WINDOW_ARB  = 0x2001;
unsigned int const WGL_DRAW_TO_BITMAP_ARB  = 0x2002;
unsigned int const WGL_ACCELERATION_ARB  = 0x2003;
unsigned int const WGL_NEED_PALETTE_ARB  = 0x2004;
unsigned int const WGL_NEED_SYSTEM_PALETTE_ARB = 0x2005;
unsigned int const WGL_SWAP_LAYER_BUFFERS_ARB = 0x2006;
unsigned int const WGL_SWAP_METHOD_ARB  = 0x2007;
unsigned int const WGL_NUMBER_OVERLAYS_ARB  = 0x2008;
unsigned int const WGL_NUMBER_UNDERLAYS_ARB = 0x2009;
unsigned int const WGL_TRANSPARENT_ARB  = 0x200A;
unsigned int const WGL_SHARE_DEPTH_ARB  = 0x200C;
unsigned int const WGL_SHARE_STENCIL_ARB  = 0x200D;
unsigned int const WGL_SHARE_ACCUM_ARB  = 0x200E;
unsigned int const WGL_SUPPORT_GDI_ARB  = 0x200F;
unsigned int const WGL_SUPPORT_OPENGL_ARB  = 0x2010;
unsigned int const WGL_DOUBLE_BUFFER_ARB  = 0x2011;
unsigned int const WGL_STEREO_ARB   = 0x2012;
unsigned int const WGL_PIXEL_TYPE_ARB  = 0x2013;
unsigned int const WGL_COLOR_BITS_ARB  = 0x2014;
unsigned int const WGL_RED_BITS_ARB  = 0x2015;
unsigned int const WGL_RED_SHIFT_ARB  = 0x2016;
unsigned int const WGL_GREEN_BITS_ARB  = 0x2017;
unsigned int const WGL_GREEN_SHIFT_ARB  = 0x2018;
unsigned int const WGL_BLUE_BITS_ARB  = 0x2019;
unsigned int const WGL_BLUE_SHIFT_ARB  = 0x201A;
unsigned int const WGL_ALPHA_BITS_ARB  = 0x201B;
unsigned int const WGL_ALPHA_SHIFT_ARB  = 0x201C;
unsigned int const WGL_ACCUM_BITS_ARB  = 0x201D;
unsigned int const WGL_ACCUM_RED_BITS_ARB  = 0x201E;
unsigned int const WGL_ACCUM_GREEN_BITS_ARB = 0x201F;
unsigned int const WGL_ACCUM_BLUE_BITS_ARB  = 0x2020;
unsigned int const WGL_ACCUM_ALPHA_BITS_ARB = 0x2021;
unsigned int const WGL_DEPTH_BITS_ARB  = 0x2022;
unsigned int const WGL_STENCIL_BITS_ARB  = 0x2023;
unsigned int const WGL_AUX_BUFFERS_ARB  = 0x2024;
unsigned int const WGL_NO_ACCELERATION_ARB  = 0x2025;
unsigned int const WGL_GENERIC_ACCELERATION_ARB = 0x2026;
unsigned int const WGL_FULL_ACCELERATION_ARB = 0x2027;
unsigned int const WGL_SWAP_EXCHANGE_ARB  = 0x2028;
unsigned int const WGL_SWAP_COPY_ARB  = 0x2029;
unsigned int const WGL_SWAP_UNDEFINED_ARB  = 0x202A;
unsigned int const WGL_TYPE_RGBA_ARB  = 0x202B;
unsigned int const WGL_TYPE_COLORINDEX_ARB  = 0x202C;
unsigned int const WGL_DRAW_TO_PBUFFER_ARB  = 0x202D;
unsigned int const WGL_MAX_PBUFFER_PIXELS_ARB = 0x202E;
unsigned int const WGL_MAX_PBUFFER_WIDTH_ARB = 0x202F;
unsigned int const WGL_MAX_PBUFFER_HEIGHT_ARB = 0x2030;
unsigned int const WGL_PBUFFER_LARGEST_ARB  = 0x2033;
unsigned int const WGL_PBUFFER_WIDTH_ARB  = 0x2034;
unsigned int const WGL_PBUFFER_HEIGHT_ARB  = 0x2035;
unsigned int const WGL_TRANSPARENT_RED_VALUE_ARB = 0x2037;
unsigned int const WGL_TRANSPARENT_GREEN_VALUE_ARB = 0x2038;
unsigned int const WGL_TRANSPARENT_BLUE_VALUE_ARB = 0x2039;
unsigned int const WGL_TRANSPARENT_ALPHA_VALUE_ARB = 0x203A;
unsigned int const WGL_TRANSPARENT_INDEX_VALUE_ARB = 0x203B;

// WGL_ARB_create_context
unsigned int const WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
unsigned int const WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
unsigned int const WGL_CONTEXT_LAYER_PLANE_ARB = 0x2093;
unsigned int const WGL_CONTEXT_FLAGS_ARB = 0x2094;

// WGLContextProfileMask
unsigned int const WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;          // ARB_create_context_profile
unsigned int const WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002; // ARB_create_context_profile
unsigned int const WGL_CONTEXT_ES2_PROFILE_BIT_EXT = 0x00000004;           // EXT_create_context_es2_profile

// WGL_ARB_create_context_profile
unsigned int const WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;


namespace kaGraphics
{
	///////////////////////////////////////////////////////////////////////////////////////////////
	//

	//=========================================================================
	//
	OpenGLContext::OpenGLContext(kaCore::Window const &window, kaCore::Byte depthBits, kaCore::Byte stencilBits)
	{
		mGLData.mDeviceContext = window.windowData().mDeviceContext;

		// We want a pixel format with the following properties:
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,                             // "Specifies the version of this data structure. This value should be set to 1."
			PFD_DRAW_TO_WINDOW |           // Support for drawing to a window
			PFD_SUPPORT_OPENGL |           // Support for OpenGL
			PFD_DOUBLEBUFFER,              // Support for double-buffering
			PFD_TYPE_RGBA,                 // Four components per pixel: red, green, blue, alpha
			window.bitsPerPixel(),         // TODO: What exactly does this mean? Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,              // TODO: What exactly does this mean?
			0,                             // Number of alpha bits.
			0,                             // Shift count of alpha bitplanes. I really don't know what this means.
			0,                             // Number of bitplanes in the accumulation buffer. TODO: What is an accumulation buffer?
			0, 0, 0, 0,                    // Number of bitplanes for each color in the accumulation buffer
			depthBits,                     // Number of bits for the depthbuffer.
			stencilBits,                   // Number of bits for the stencilbuffer.
			0,                             // Number of Aux buffers.
			PFD_MAIN_PLANE,                // Ignored. Used in earlier versions of OpenGL.
			0,                             // Reserved. I don't understand it anyway.
			0,                             // Ignored. Used in earlier versions of OpenGL.
			0,                             // A transparent RGB color value.
			0                              // Ignored. Used in earlier versions of OpenGL.
		};

		// Chooses the best fitting pixel format that the device context supports
		int pixelFormatID = ChoosePixelFormat(mGLData.mDeviceContext, &pfd);
		if(!pixelFormatID)
			kaCore::PlatformAPIException::logAndThrow("ChoosePixelFormat");

		// Check the chosen pixel format
		std::memset(&pfd, 0, sizeof(pfd));
		if(DescribePixelFormat(mGLData.mDeviceContext, pixelFormatID, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
			kaCore::PlatformAPIException::logAndThrow("DescribePixelFormat");

		if(pfd.cColorBits != window.bitsPerPixel() || pfd.cDepthBits != depthBits ||
		   pfd.cStencilBits != stencilBits)
		{
			kaCore::UTF8String error = kaCore::formatString("The demanded pixel format is not available\n"
			                                            "\tDemanded: {} bitsPerPixel, {} depthBits, {} stencilBits\n"
			                                            "\tBest available: {} bitsPerPixel, {} depthBits, {} stencilBits",
			                                            static_cast<kaCore::Int>(window.bitsPerPixel()), static_cast<kaCore::Int>(depthBits), static_cast<kaCore::Int>(stencilBits),
														static_cast<kaCore::Int>(pfd.cColorBits), static_cast<kaCore::Int>(pfd.cDepthBits), static_cast<kaCore::Int>(pfd.cStencilBits));
			kaCore::logAndThrow< ::std::runtime_error >(error.c_str());
		}


		// Now set the chosen pixel format
		if(SetPixelFormat(mGLData.mDeviceContext, pixelFormatID, &pfd) == FALSE)
			kaCore::PlatformAPIException::logAndThrow("SetPixelFormat");

		// Creates a new OpenGL rendering context with the same pixel format as
		// the device context.
		mGLData.mRenderContext = wglCreateContext(mGLData.mDeviceContext);
		if(!mGLData.mRenderContext)
			kaCore::PlatformAPIException::logAndThrow("wglCreateContext");

		// Makes the new created rendering context current for this thread, so all
		// following OpenGL calls use this context.
		if(wglMakeCurrent(mGLData.mDeviceContext, mGLData.mRenderContext) == FALSE)
			kaCore::PlatformAPIException::logAndThrow("wglMakeCurrent");


		kaCore::UTF8String info = kaCore::formatString("OpenGL context created\n"
		                                           "\tVersion: {}\n"
		                                           "\tPixel format: {} bitsPerPixel, {} depthBits, {} stencilBits\n",
		                                           glGetString(GL_VERSION),
		                                           static_cast<kaCore::Int>(pfd.cColorBits), static_cast<kaCore::Int>(pfd.cDepthBits), static_cast<kaCore::Int>(pfd.cStencilBits));
		KAOS_INFO(info);

		if(glewInit() != GLEW_OK)
			kaCore::logAndThrow< ::std::runtime_error >("glewInit failed");

		if(GLEW_ARB_fragment_program)
			KAOS_INFO("GLEW_ARB_fragment_program is supported");

		KAOS_INFO("Shader version: " << reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

		/*if(gl3wInit())
			kaCore::logAndThrow<kaCore::PlatformAPIException>("Creating GL3W failed.\n"
			                                              "This usually means that your graphics card does not support OpenGL 3.0.");

		// Und jetzt kann ein "richtiges" context erstellt werden
		// Again: http://www.opengl.org/wiki/OpenGL_3.0_and_beyond,_creating_a_context
		//
		const int attribList[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0,        //End
		};

		// Aus irgendeinem Grund erstellt GLEW keine wglChoosePixelFormatARB-Funktion
		// Immer muss man alles selber machen.
		//
		typedef BOOL (*_wglChoosePixelFormatARB)(HDC, int const*, float const*, UINT, int*, UINT*);
		_wglChoosePixelFormatARB wglChoosePixelFormatARB = (_wglChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB");
		if(!wglChoosePixelFormatARB)
			kaCore::logAndThrow<kaCore::PlatformAPIException>("Cannot get function pointer to 'wglChoosePixelFormatARB'");

		kaCore::Int32 pixelFormat;
		kaCore::UInt32 numFormats;
		if(!wglChoosePixelFormatARB(mGLData.mDeviceContext, attribList, nullptr, 1, &pixelFormat, &numFormats))
		{
			kaCore::logAndThrow<kaCore::PlatformAPIException>("Choosing OpenGL pixel format failed");
		}

		//if(!WGL_ARB_create_context_profile)
		//	kaCore::logAndThrow<kaCore::PlatformAPIException>("WGL_ARB_create_context_profile not supported.\n"
		//					"Although there is a way to create an OpenGL 3.x context anyway, it is still not supported.");

		// Alten Context löschen
		//
		//wglDeleteContext(mGLData.mRenderContext);

		//----------------------------------
		// TODO: OpenGL Context Erstellung 3.x überarbeiten
		//----------------------------------

		int minor=0;
		int major=0;
		glGetIntegerv(GL_MAJOR_VERSION,&major);
		glGetIntegerv(GL_MINOR_VERSION,&minor);
		KAOS_INFO("OpenGL " << major << "." << minor << " context created.");


		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, major,
			WGL_CONTEXT_MINOR_VERSION_ARB, minor,
			//WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0
		};

		//mGLData.mRenderContext = wglCreateContextAttribsARB(mGLData.mDeviceContext, nullptr, attribs);
		if(!mGLData.mRenderContext)
			kaCore::logAndThrow<kaCore::PlatformAPIException>("Creating real OpenGL context failed.");*/
	}

	//=========================================================================
	//
	OpenGLContext::~OpenGLContext()
	{
		// Releases the device context and deactivates the rendering context.
		if(wglMakeCurrent(nullptr, nullptr))
		{
			// Deletes the OpenGL rendering context.
			if(wglDeleteContext(mGLData.mRenderContext))
				KAOS_INFO("OpenGL context released");
			else
				KAOS_WARN("Function \"wglDeleteContext\" failed: " << kaCore::windows::lastErrorDescription());
		}
		else
			KAOS_WARN("Function \"wglMakeCurrent(nullptr, nullptr)\" failed: " << kaCore::windows::lastErrorDescription());
	}

	//=========================================================================
	//
	void OpenGLContext::swapBuffers()
	{
		if(!SwapBuffers(mGLData.mDeviceContext))
			kaCore::PlatformAPIException::logAndThrow("SwapBuffers");
	}

} // End: namespace: kaGraphics


#endif // conditional compilation: LIGHT_PLATFORM_LINUX
