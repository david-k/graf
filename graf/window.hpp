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

#include "graf/graf.hpp"

#include <memory>


namespace graf
{
	namespace internal { class window_impl; }

	//=============================================================================================
	//
	//=============================================================================================
	class window
	{
	public:
		// Constructor
		window(utf8_unit const *_title, uint width, uint height, uint depth, uint stencil);

		// Destructor
		~window();

		// Sets the title
		void title(utf8_unit const *title);

		// Processes events like keyboard input, mouse input and window resizing.
		// Returns false if the user has closed the window.
		bool process_events();

		// Get screen dimension
		uint screen_width();
		uint screen_height();

		// Swaps the backbuffer with the frontbuffer, so all your work becomes
		// visible.
		void swap_buffers();


		internal::window_impl* platform_impl();

	private:
		::std::unique_ptr<internal::window_impl> m_impl;
	};

} // namespace: graf


