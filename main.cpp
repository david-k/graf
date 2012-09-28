#include "light/light.hpp"

#ifdef LIGHT_RELEASE
	#define NDEBUG
#endif

#include "red/static_vector.hpp"
#include "red/dynamic_vector.hpp"
#include "red/vector_operations.hpp"
//#include "red/static_matrix.hpp"
//#include "red/matrix_operations.hpp"

#include "light/diagnostics/logging.hpp"
#include "light/utility/safe_int_cast.hpp"
#include "light/io/mmfile.hpp"
//#include "light/chrono/hires_clock.hpp"
//#include "light/chrono/stopwatch.hpp"

#include <iostream>


using namespace light;

namespace red
{
	template<typename Writer, typename Vector,
			 typename light::enable_if<is_vector<Vector>::value, int>::type = 0>
	Writer write_to(Writer writer, Vector const &vec, char const *fmt = "{}")
	{
		auto const *begin = &vec[0];
		auto const *end = begin + vec.dimension();
		//enum { test = RED_DIMENSION(vec) };
		return light::print_range(writer, begin, end, ", ", fmt);
	}

	/*template<typename Writer, typename Matrix,
			 typename light::enable_if<matrix_traits<Matrix>::is_matrix, int>::type = 0>
	Writer write_to(Writer writer, Matrix const &mat, char const *fmt = "{}")
	{
		//auto const *begin = &mat[0];
		//auto const *end = begin + num_elements(vec);
		//enum { test = num_elements(vec) };
		//return light::print_range(writer, begin, end, ", ", fmt);
	}*/
}


template<typename Real, size_t Dimension>
class Vector
{
public:
	Vector() = default;

	constexpr size_t dimension() { return Dimension; }

private:
	Real m_elements[Dimension];
};


int main()
{
	g_info.add_target(&std_out);
	LIGHT_LOG_INFO("G'day\n");

	//red::static_matrix<red::vector2f, 2> mat;

	red::vector2f vec(2.0f, 3.43f);
	red::vector2f b = vec + vec;

	red::dynamic_vector<float> dvec(4);
	dvec[0] = 1.2f;
	dvec[1] = 2.3f;
	dvec[2] = 3.4f;
	dvec[3] = 4.5f;

	std::cout << str_printf("vec = ({.2})\nlength(b) = {}", vec, red::length(b)) << std::endl;
	std::cout << str_printf("dvec = ({})", dvec) << std::endl;

	return 0;
}


