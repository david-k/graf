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

#include "graf/window.hpp"

#ifdef LIGHT_PLATFORM_LINUX
	#include "graf/internal/linux_window.hpp"
#else
	#error Platform not supported yet
#endif


namespace graf
{
	//=============================================================================================
	//
	//=============================================================================================
	window::window(const utf8_unit *_title, uint width, uint height, uint depth, uint stencil) :
		m_impl(new internal::window_impl(_title, width, height, depth, stencil))
	{

	}

	window::~window()
	{

	}

	void window::title(const utf8_unit *title)
	{
		m_impl->title(title);
	}

	bool window::process_events()
	{
		return m_impl->process_events();
	}

	void window::swap_buffers()
	{
		m_impl->swap_buffers();
	}

	uint window::screen_width()
	{
		return m_impl->screen_width();
	}

	uint window::screen_height()
	{
		return m_impl->screen_height();
	}

	internal::window_impl* window::platform_impl()
	{
		return m_impl.get();
	}


} // namespace: graf


