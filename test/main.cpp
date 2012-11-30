#include "light/light.hpp"

#ifdef LIGHT_RELEASE
	#define NDEBUG
#endif

#include "red/static_vector.hpp"
#include "red/dynamic_vector.hpp"
#include "red/vector_operations.hpp"
#include "red/static_matrix.hpp"
#include "red/matrix_operations.hpp"

#include "light/diagnostics/logging.hpp"
#include "light/utility/safe_int_cast.hpp"
#include "light/io/mmfile.hpp"
#include "light/chrono/hires_clock.hpp"
#include "light/chrono/stopwatch.hpp"

#include "graf/window.hpp"
#include "graf/opengl.hpp"
#include "graf/logger.hpp"

#include <iostream>


using namespace light;

namespace red
{
    template<typename Writer, typename Vector,
			 typename light::enable_if<is_vector<Vector>::value, int>::type = 0>
	Writer write_to(Writer writer, Vector const &vec, char const *fmt = "{}")
	{
		return light::print_range(writer, vec.cbegin(), vec.cend(), ", ", fmt);
    }

	template<typename Writer, typename Matrix,
			 typename light::enable_if<is_matrix<Matrix>::value, int>::type = 0>
	Writer write_to(Writer writer, Matrix const &mat, char const *fmt = "{}")
	{
		return light::print_range(writer, mat.cbegin(), mat.cend(), "\n", fmt);
	}
}



int main()
{
	using namespace graf;

    g_info.add_target(&std_out);
	g_error.add_target(&std_error);
    LIGHT_LOG_INFO("G'day\n");

	red::vector3f a(2.f, 2.f, 2.f);
	red::vector3f b(2.f, 2.f, 2.f);
	auto c = red::cross(a, b);

	red::matrix4x4f mat;
	for(auto &col: mat)
		col = 2.333f;

	std::cout << light::str_printf("{.2}\n", mat);

	try
	{
		window render_win("ÖpänJüÄl", 800, 600, 24, 8);
		opengl_device opengl(&render_win);

		std::cout << str_printf("width: {}\nheight: {}", render_win.screen_width(), render_win.screen_height()) << std::endl;

		glClearColor(0.5, 0, 0, 1);
		while(render_win.process_events())
		{
			glClear(GL_COLOR_BUFFER_BIT);

			render_win.swap_buffers();
		}
	}
	catch(::std::exception const &e)
	{
		GRAF_ERROR_MSG("Unhandled exception: {}\n", e.what());
	}

	return 0;
}


