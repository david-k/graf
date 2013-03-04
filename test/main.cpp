#include "light/light.hpp"

#ifdef LIGHT_RELEASE
	#define NDEBUG
#endif

#include <functional>

#include "red/static_vector.hpp"
//#include "red/dynamic_vector.hpp"
#include "red/vector_operations.hpp"
#include "red/static_matrix.hpp"
#include "red/matrix_operations.hpp"

//#include "light/diagnostics/logging.hpp"
#include "light/utility/safe_int_cast.hpp"
//#include "light/io/mmfile.hpp"
//#include "light/chrono/hires_clock.hpp"
//#include "light/chrono/stopwatch.hpp"
#include "light/utility/non_copyable.hpp"

#include "catalog_set.hpp"

//#include "graf/window.hpp"
//#include "graf/opengl.hpp"
//#include "graf/logger.hpp"

//#include "gui.hpp"

//#include <iostream>

#include <SFML/Graphics.hpp>


using namespace light;

namespace red
{
    /*template<typename Writer, typename Vector,
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
	}*/
}


//=================================================================================================
//
//=================================================================================================
class IndexedStorage
{
public:
	IndexedStorage() :
		m_next_free_entry(0) {}

	template<typename T>
	size_t add(T const &value)
	{
		static_assert(sizeof(T) <= sizeof(size_t), "Value types bigger than sizeof(size_t) not yet supported");
		assert(m_next_free_entry <= m_entries.size());

		Entry ent = {typeid(T).hash_code(), *reinterpret_cast<size_t*>(&value)};

		if(m_next_free_entry == m_entries.size())
			m_entries.insert(m_entries.begin() + m_next_free_entry, ent);
		else
			m_entries[m_next_free_entry] = ent;

		m_next_free_entry = m_entries.size();
	}

	template<typename T>
	T& get(size_t index)
	{
		assert(index < m_entries.size());
		assert(m_entries[index].m_value == typeid(T).hash_code());

		return *reinterpret_cast<T*>(&m_entries[index].m_value);
	}

private:
	struct Entry
	{
		size_t m_type;
		size_t m_value;
	};

	std::vector<Entry> m_entries;
	size_t m_next_free_entry;
};


//=================================================================================================
//
//=================================================================================================
class SpatialCatalog
{
private:
	class UniqueType {};

public:
	typedef Handle<UniqueType> HandleType;

	struct Base
	{
		HandleType m_parent;
		HandleType m_predecessor;
		HandleType m_successor;
	};
	struct Position
	{
		red::vector2f m_position;
		red::vector2f m_bounding_box;
		red::vector2f m_world_position;
	};
	struct ZData
	{
		light::uint4 m_z_offset;
		light::uint4 m_depth;
		light::uint4 m_world_z_index;
	};

	typedef CatalogSet<HandleType, Base, Position, ZData> CatalogType;


	HandleType add(HandleType parent, red::vector2f const pos, red::vector2f const &bbox, light::uint4 depth = 5)
	{
		HandleType handle;

		Base base = {parent, last_child(parent), HandleType()};
		Position spos = {pos, bbox, red::vector2f()};
		ZData z_index = {1, depth, 0};

		// Compute insert position
		HandleType last_element = parent;
		HandleType cur;
		while((cur = last_child(last_element)).is_valid())
			last_element = cur;
		size_t insert_pos = last_element.is_valid() ? m_spatials.get_index(last_element) + 1 : 0;

		m_spatials.add(insert_pos, 1, &base, &spos, &z_index, &handle);
		if(base.m_predecessor.is_valid())
			get<Base>(base.m_predecessor).m_successor = handle;

		return handle;
	}

	/// Updates position and and z-order of all elements
	void update()
	{
		update_z();
		update_position();
	}

	/// Returns the first child of the given element. If it has no children, an
	/// invalid handle is returned
	HandleType first_child(HandleType parent)
	{
		HandleType first;

		if(parent.is_valid())
		{
			auto child_index = m_spatials.get_index(parent) + 1;
			if(child_index < m_spatials.size())
			{
				if(m_spatials.at<Base>(child_index).m_parent == parent)
					first = m_spatials.get_handle(child_index);
			}
		}

		return first;
	}

	/// Returns the last child of the given element. If it has no children, an
	/// invalid handle is returned
	HandleType last_child(HandleType parent)
	{
		HandleType last;
		auto array_size = m_spatials.size();
		size_t current = parent.is_valid() ? m_spatials.get_index(parent) + 1 : 0;

		while(current < array_size && m_spatials.at<Base>(current).m_parent == parent)
			last = m_spatials.get_handle(current++);

		return last;
	}

	/// Returns whether the given element has children or not
	bool has_children(HandleType h)
	{
		if(h.is_valid())
		{
			size_t child_index = m_spatials.get_index(h) + 1;
			if(child_index < m_spatials.size())
				return m_spatials.at<Base>(child_index).m_parent == h;
		}

		return false;
	}

	template<typename T>
	T& get(HandleType h)
	{
		return m_spatials.get<T>(h);
	}

	template<typename T>
	T const& get(HandleType h) const
	{
		return m_spatials.get<T>(h);
	}

	template<typename T>
	T& at(size_t i)
	{
		return m_spatials.at<T>(i);
	}

	template<typename T>
	T const& at(size_t i) const
	{
		return m_spatials.at<T>(i);
	}


	size_t get_index(HandleType h) const
	{
		return m_spatials.get_index(h);
	}

	HandleType get_handle(size_t index) const
	{
		return m_spatials.get_handle(index);
	}

private:
	CatalogType m_spatials;

	void update_position()
	{
		for(size_t i = 0; i < m_spatials.size(); ++i)
		{
			auto &spatial = m_spatials.at<Position>(i);
			auto &base = m_spatials.at<Base>(i);

			if(base.m_parent.is_valid())
				spatial.m_world_position = spatial.m_position + m_spatials.get<Position>(base.m_parent).m_world_position;
			else
				spatial.m_world_position = spatial.m_position;
		}
	}

	void update_z()
	{
		if(m_spatials.size())
		{
			size_t offset = 0;
			update_z_internal(0, &offset);
		}
	}

	void update_z_internal(size_t index, size_t *offset)
	{
		auto *base = &m_spatials.at<Base>(index);
		auto *z_data = &m_spatials.at<ZData>(index);
		auto current = m_spatials.get_handle(index);

		while(true)
		{
			z_data->m_world_z_index = *offset + z_data->m_z_offset;
			*offset += z_data->m_z_offset + z_data->m_depth;

			if(has_children(current))
				update_z_internal(m_spatials.get_index(current) + 1, offset);

			current = base->m_successor;
			if(current.is_valid())
			{
				z_data = &m_spatials.get<ZData>(base->m_successor);
				base = &m_spatials.get<Base>(base->m_successor);
			}
			else
				break;
		}
	}
};


class SpatialHandle
{
public:
	typedef SpatialCatalog::HandleType HandleType;

	SpatialHandle(HandleType spatial, SpatialCatalog &catalog) :
		m_handle(spatial),
		m_catalog(catalog) {}

	/// Gets the handle
	HandleType spatial() const { return m_handle; }

	// Gets spatial data
	red::vector2f& position()
	{
		return m_catalog.get<SpatialCatalog::Position>(m_handle).m_position;
	}
	red::vector2f& bounding_box()
	{
		return m_catalog.get<SpatialCatalog::Position>(m_handle).m_bounding_box;
	}

private:
	HandleType m_handle;
	SpatialCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
class RectangleRenderer
{
public:
	RectangleRenderer(sf::RenderWindow &win) :
		m_window(win) {}

	void add(red::vector2f pos, red::vector2f dim, light::uint4 z_index, sf::Color color = sf::Color::Red)
	{
		sf::RectangleShape rectangle;
		rectangle.setSize(sf::Vector2f(dim.x(), dim.y()));
		rectangle.setFillColor(color);
		rectangle.setOutlineThickness(5);
		rectangle.setPosition(pos.x(), pos.y());

		m_rects.push_back(entry(rectangle, z_index));
	}

	void display()
	{
		std::sort(m_rects.begin(), m_rects.end());

		for(auto &shape: m_rects)
			m_window.draw(shape.m_shape);
	}

	void clear()
	{
		m_rects.clear();
	}

private:
	struct entry
	{
		entry(sf::RectangleShape const &shape, size_t z_index) :
			m_shape(shape),
			m_z_index(z_index) {}

		bool operator < (entry const &rhs) const
		{
			return m_z_index < rhs.m_z_index;
		}

		sf::RectangleShape m_shape;
		size_t m_z_index;
	};

	sf::RenderWindow &m_window;
	std::vector<entry> m_rects;
};



//=================================================================================================
//
//=================================================================================================
enum EventType
{
	resize = 1,
	move = 2,
	mouse_enter = 4,
	mouser_over = 8,
	mouse_leave = 16,
	key_press = 32,
	key_down = 64,
	key_release = 128,
	mouse_press = 256,
	mouse_down = 512,
	mouse_release = 1024,
	focus = 2048,
	blur = 4096
};

enum class MouseButton
{
	left,
	middle,
	right
};


class InputCatalog
{
private:
	class UniqueType {};

public:
	typedef Handle<UniqueType> HandleType;

	struct Event
	{
		Event() :
			m_events(0) {}

		int m_events;
	};


	InputCatalog(SpatialCatalog &spatials) :
		m_spatials(spatials) {}


	void add(size_t pos, Event e)
	{
		m_events.insert(m_events.begin() + pos, e);
	}

	Event& get(SpatialCatalog::HandleType h)
	{
		auto index = m_spatials.get_index(h);
		assert(index < m_events.size());

		return m_events[index];
	}

	void update()
	{
		for(size_t i = 0; i < m_events.size(); i++)
		{
			m_events[i].m_events = 0;
		}
	}

	void mouse_button_press(MouseButton but, red::vector2f pos)
	{
		auto selected = get_element_by_pos(pos);
		if(selected.is_valid() && selected == m_focused)
		{
			get(m_focused).m_events |= mouse_press;
		}
		else if(selected.is_valid())
		{
			if(m_focused.is_valid())
				get(m_focused).m_events |= blur;

			m_focused = selected;
			get(m_focused).m_events |= focus;
		}
		else
		{
			if(m_focused.is_valid())
			{
				get(m_focused).m_events |= blur;
				m_focused = {};
			}
		}
	}

	bool point_in_rect(red::vector2f point, red::vector2f top_left, red::vector2f dim)
	{
		return
			point.x() >= top_left.x() &&
			point.x() < top_left.x() + dim.x() &&
			point.y() >= top_left.y() &&
			point.y() < top_left.y() + dim.y();
	}

	SpatialCatalog::HandleType get_element_by_pos(red::vector2f pos)
	{
		SpatialCatalog::HandleType element;

		size_t cur_z = 0;
		for(size_t i = 0; i < m_events.size(); i++)
		{
			auto spatial = m_spatials.at<SpatialCatalog::Position>(i);
			auto z_data = m_spatials.at<SpatialCatalog::ZData>(i);

			if(z_data.m_world_z_index > cur_z)
			{
				if(point_in_rect(pos, spatial.m_world_position, spatial.m_bounding_box))
					element = m_spatials.get_handle(i);
			}
		}

		return element;
	}

private:
	std::vector<Event> m_events;
	SpatialCatalog &m_spatials;
	SpatialCatalog::HandleType m_focused;
};


class InputHandle
{
public:
	InputHandle(SpatialHandle spatial, InputCatalog &catalog) :
		m_spatial(spatial),
		m_catalog(catalog) {}

	SpatialHandle spatial() { return m_spatial; }

	int events() { return m_catalog.get(spatial().spatial()).m_events; }

private:
	SpatialHandle m_spatial;
	InputCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
class DisplayCatalog
{
private:
	class UniqueType {};

public:
	typedef Handle<UniqueType> HandleType;

	struct Entity
	{
		Entity(SpatialCatalog::HandleType spatial, sf::Color color) :
			m_spatial(spatial),
			m_color(color) {}

		SpatialCatalog::HandleType m_spatial;
		sf::Color m_color;
	};

	typedef CatalogSet<HandleType, Entity> CatalogType;

	DisplayCatalog(SpatialCatalog const &spatials, RectangleRenderer &renderer) :
		m_spatials(spatials),
		m_renderer(renderer) {}

	HandleType add(Entity const &entity)
	{
		return m_entities.add(entity);
	}

	void render()
	{
		for(auto ent = m_entities.begin<Entity>(); ent != m_entities.end<Entity>(); ++ent)
		{
			SpatialCatalog::Position const &pos = m_spatials.get<SpatialCatalog::Position>(ent->m_spatial);
			SpatialCatalog::ZData const &z_data = m_spatials.get<SpatialCatalog::ZData>(ent->m_spatial);
			m_renderer.add(pos.m_world_position, pos.m_bounding_box, z_data.m_world_z_index, ent->m_color);
		}
	}

	Entity& get(HandleType h)
	{
		return m_entities.get<Entity>(h);
	}

	Entity const& get(HandleType h) const
	{
		return m_entities.get<Entity>(h);
	}

private:
	CatalogType m_entities;
	SpatialCatalog const &m_spatials;
	RectangleRenderer &m_renderer;
};

class DisplayHandle
{
public:
	DisplayHandle(
		SpatialHandle spatial, DisplayCatalog::HandleType display, DisplayCatalog &dis_cat
	) :
		m_spatial(spatial),
		m_display(display),
		m_catalog(dis_cat) {}

	DisplayCatalog::HandleType display() { return m_display; }
	SpatialHandle spatial() { return m_spatial; }

	sf::Color& color() { return m_catalog.get(display()).m_color; }

private:
	SpatialHandle m_spatial;
	DisplayCatalog::HandleType m_display;
	DisplayCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
class ButtonCatalog
{
private:
	class UniqueType {};

public:
	typedef Handle<UniqueType> HandleType;

	enum State
	{
		clicked = 1, // Mouse button down + mouse button up while above button
		pressed = 2,
		released = 4,
		down = 8,
		up = 16,
		mouse_over = 32,
		mouse_entered = 64,
		mouse_left = 128,
		focused = 256,
		blured = 512
	};
	struct Button
	{
		Button(sf::Color color, SpatialCatalog::HandleType pos) :
			m_position(pos),
			m_color(color) {}

		SpatialCatalog::HandleType m_position;
		State m_state;
		sf::Color m_color;
	};

	typedef CatalogSet<HandleType, Button> CatalogType;

	ButtonCatalog(SpatialCatalog const &spatials, RectangleRenderer &renderer) :
		m_spatials(spatials),
		m_renderer(renderer) {}

	HandleType add(Button const &button)
	{
		return m_buttons.add(button);
	}


	// Update/render whole
	void update()
	{

	}

	Button& get(HandleType h)
	{
		return m_buttons.get<Button>(h);
	}

	Button const& get(HandleType h) const
	{
		return m_buttons.get<Button>(h);
	}

private:
	CatalogType m_buttons;
	SpatialCatalog const &m_spatials;
	RectangleRenderer &m_renderer;
};


class ButtonHandle
{
public:
	ButtonHandle(
		DisplayHandle display, InputHandle input,
		ButtonCatalog::HandleType button, ButtonCatalog &but_cat
	) :
		m_display(display),
		m_input(input),
		m_button(button),
		m_catalog(but_cat) {}

	ButtonCatalog::HandleType button() { return m_button; }

	InputHandle input() { return m_input; }
	DisplayHandle display() { return m_display; }
	SpatialHandle spatial() { return m_display.spatial(); }

private:
	DisplayHandle m_display;
	InputHandle m_input;
	ButtonCatalog::HandleType m_button;
	ButtonCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
class GuiMananger
{
public:
	GuiMananger(RectangleRenderer &renderer) :
		m_spatial_data(),
		m_button_data(m_spatial_data, renderer),
		m_input(m_spatial_data),
		m_display(m_spatial_data, renderer) {}

	ButtonHandle add_button(red::vector2f pos, red::vector2f dim, sf::Color color, SpatialCatalog::HandleType parent = SpatialCatalog::HandleType())
	{
		SpatialCatalog::HandleType spatial = m_spatial_data.add(parent, pos, dim);
		ButtonCatalog::HandleType button = m_button_data.add(ButtonCatalog::Button(color,  spatial));
		m_input.add(m_spatial_data.get_index(spatial), InputCatalog::Event());
		DisplayCatalog::HandleType display = m_display.add(DisplayCatalog::Entity(spatial, color));

		SpatialHandle spat_handle(spatial, m_spatial_data);

		return ButtonHandle(DisplayHandle(spat_handle, display, m_display), InputHandle(spat_handle, m_input), button, m_button_data);
	}

	InputCatalog& input() { return m_input; }

	void update()
	{
		m_spatial_data.update();
		m_input.update();
		m_button_data.update();
	}

	void render()
	{
		m_display.render();
	}

private:
	SpatialCatalog m_spatial_data;
	ButtonCatalog m_button_data;
	InputCatalog m_input;
	DisplayCatalog m_display;
};


//=================================================================================================
// Abstract device
//=================================================================================================
namespace
{
struct chunk_wise_tag {};
struct random_access_tag : chunk_wise_tag {};
struct direct_access_tag : random_access_tag {};

struct io_result
{
	// Digression: The English language
	//    - status: classification of state among several well-defined possibilities.
	//    - state: a durable or lasting condition
	// See http://forum.wordreference.com/showthread.php?t=287984&langid=3
	//     http://english.stackexchange.com/questions/12958/status-vs-state
	enum state {ok, eof, error};

	io_result(state s, size_t num) :
		status(s),
		amount(num) {}

	state status;
	size_t amount;
};

template<typename CharType, typename TraitsType = std::char_traits<CharType> >
class string_buffer
{
public:
	typedef CharType value_type;
	typedef direct_access_tag category;

	typedef TraitsType traits_type;
	typedef std::basic_string<value_type, traits_type> string_type;

	string_buffer(string_type *dev) :
		m_device(dev),
		m_pos(data()){}

	// Chunk-wise category
	size_t read(value_type *dest, size_t size)
	{
		assert(m_pos + size < data() + this->size());
		std::copy(m_pos, m_pos + size, dest);

		return size;
	}

	size_t write(value_type const *src, size_t size)
	{
		if(m_pos + size > data() + this->size())
			m_device->resize(m_pos + size);
		m_device->replace(m_pos, size, src, size);

		return size;
	}

	// Random access category
	size_t size() const   { return m_device->size(); }
	void seek(size_t pos) { assert(pos < size()); m_pos = data() + pos; }
	size_t tell() const   { return m_pos - data(); }

	// Direct access category
	value_type* data()              { return &(*m_device)[0]; }
	value_type const* data() const  { return &(*m_device)[0]; }
	value_type const* cdata() const { return &(*m_device)[0]; }

private:
	string_type *m_device;
	value_type *m_pos;
};





struct raw_content_tag {};
struct text_content_tag {};

struct utf8_content_tag : text_content_tag {};


template<
	typename BufferType,
	typename ContentType
>
class device
{
public:
	typedef BufferType buffer_type;
	typedef ContentType content_type;
	typedef typename buffer_type::value_type value_type;

	device(buffer_type *buffer) :
		m_buffer(buffer) {}

	buffer_type* operator * () const { return m_buffer; }
	buffer_type* operator -> () const { return m_buffer; }

private:
	buffer_type *m_buffer;
};



typedef device<string_buffer<char>, utf8_content_tag>  u8string_device;

template<typename Device>
void write(Device dev, char const *str)
{
	write_content(dev, Device::content_type(), str);
}

template<typename Device>
void write_content(Device dev, utf8_content_tag, char const *str)
{
	//dev->write(str, strlen(str));
}

}


//=================================================================================================
//
//=================================================================================================
int main()
{
	//using namespace graf;

    /*g_info.add_target(&std_out);
	g_error.add_target(&std_error);
    LIGHT_LOG_INFO("G'day\n");*/



	/*try
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
	}*/

	sf::RenderWindow window(sf::VideoMode(800, 600), "My window");
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
	}


	return 0;
}
