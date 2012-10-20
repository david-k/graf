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

#include "graf/internal/linux_window.hpp"

#include <iostream>


using namespace light;

namespace red
{
    template<typename Writer, typename Vector,
			 typename light::enable_if<is_vector<Vector>::value, int>::type = 0>
	Writer write_to(Writer writer, Vector const &vec, char const *fmt = "{}")
	{
		auto const *begin = &vec[0];
		auto const *end = begin + RED_DIMENSION(vec);
		//enum { test = RED_DIMENSION(vec) };
		return light::print_range(writer, begin, end, ", ", fmt);
    }

	template<typename Writer, typename Matrix,
			 typename light::enable_if<is_matrix<Matrix>::value, int>::type = 0>
	Writer write_to(Writer writer, Matrix const &mat, char const *fmt = "{}")
	{
		auto const *begin = &mat[0];
		auto const *end = begin + RED_COLUMNS(mat);
		return light::print_range(writer, begin, end, "\n", fmt);
	}
}



int main()
{
	using namespace graf;

    g_info.add_target(&std_out);
	g_error.add_target(&std_error);
    LIGHT_LOG_INFO("G'day\n");

	try
	{
		internal::window_impl window("ÖpänJüÄl", 800, 600, 24, 8);

		std::cout << str_printf("width: {}\nheight: {}", window.screen_width(), window.screen_height()) << std::endl;

		while(window.process_events())
		{

		}
	}
	catch(::std::exception const &e)
	{
		LIGHT_LOG_ERROR("Unhandled exception: {}\n", e.what());
	}

	return 0;
}


