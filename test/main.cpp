#include "light/light.hpp"

#ifdef LIGHT_RELEASE
	#define NDEBUG
#endif


#include "red/static_vector.hpp"


//#include "light/diagnostics/logging.hpp"
#include "light/utility/safe_int_cast.hpp"
//#include "light/io/mmfile.hpp"
//#include "light/chrono/hires_clock.hpp"
//#include "light/chrono/stopwatch.hpp"
#include "light/utility/non_copyable.hpp"

#include "catalog_set.hpp"

#include "graf/window.hpp"
#include "graf/opengl.hpp"
#include "graf/logger.hpp"

//#include "gui.hpp"

#include <iostream>

#include <SFML/Graphics.hpp>


using namespace light;


//=================================================================================================
//
//=================================================================================================
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


//=================================================================================================
//
//=================================================================================================
int main()
{
	using namespace graf;

    g_info.add_target(&std_out);
	g_error.add_target(&std_error);
    LIGHT_LOG_INFO("G'day\n");



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




	/*sf::RenderWindow window(sf::VideoMode(800, 600), "My window");
	RectangleRenderer renderer(window);


	GuiMananger gui_data(renderer);

	auto button = gui_data.add_button({100.f, 100.f}, {200.f, 200.f}, sf::Color::Red);
	button.display().color() = sf::Color::Blue;

	auto child_button = gui_data.add_button({30.0f, 0.0f}, {100.0f, 100.0f}, sf::Color::Red, button.spatial().spatial());



	// run the program as long as the window is open
	while (window.isOpen())
	{
		gui_data.update();

		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::MouseButtonPressed)
			{
				if(event.mouseButton.button == sf::Mouse::Left)
				{
					gui_data.input().mouse_button_press(MouseButton::left, red::vector2f(event.mouseButton.x, event.mouseButton.y));
				}
			}
		}

		if(child_button.input().events() & focus)
			child_button.display().color() = sf::Color::Green;

		if(child_button.input().events() & blur)
			child_button.display().color() = sf::Color::Red;

		gui_data.render();

		window.clear();
		renderer.display();
		renderer.clear();
		window.display();
	}*/


	return 0;
}
