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

#include "gui.hpp"

#include <iostream>

#include <SFML/Graphics.hpp>


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



//=================================================================================================
//
//=================================================================================================
template<typename Type>
void typeless_default_deleter(void *ptr, size_t type_hash)
{
	assert(typeid(Type).hash_code() == type_hash);
	delete static_cast<Type*>(ptr);
}

class typeless_ptr : light::non_copyable
{
public:
	typedef void (*deleter_type)(void *ptr, size_t type_hash);

	// Constructor
	template<typename PointerType>
	explicit typeless_ptr(PointerType *ptr, deleter_type deleter = typeless_default_deleter<PointerType>) :
		m_pointer(ptr),
		m_type_hash(typeid(PointerType).hash_code()),
		m_deleter(deleter)
	{

	}

	// Move constructor
	typeless_ptr(typeless_ptr &&rhs) :
		m_pointer(rhs.m_pointer),
		m_type_hash(rhs.m_type_hash),
		m_deleter(rhs.m_deleter)
	{
		rhs.m_pointer = nullptr;
		rhs.m_type_hash = 0;
		rhs.m_deleter = nullptr;
	}

	// Destructor
	~typeless_ptr()
	{
		if(m_pointer)
			m_deleter(m_pointer, m_type_hash);
	}

	// Move assignment
	typeless_ptr& operator = (typeless_ptr &&rhs)
	{
		m_pointer = rhs.m_pointer;
		m_type_hash = rhs.m_type_hash;
		m_deleter = rhs.m_deleter;

		rhs.m_pointer = nullptr;
		rhs.m_type_hash = 0;
		rhs.m_deleter = nullptr;

		return *this;
	}

	// Access
	template<typename Type>
	Type* get_as() const
	{
		assert(typeid(Type).hash_code() == m_type_hash);
		return static_cast<Type*>(m_pointer);
	}

	void* get() const { return m_pointer; }

private:
	void *m_pointer;
	size_t m_type_hash;
	deleter_type m_deleter;
};


//=================================================================================================
//
//=================================================================================================
struct LastElement {};


template<typename T, typename Set>
T& internal_get(Set &set, std::true_type /* is wanted element */, std::false_type /* is not last element */)
{
	return set.m_value;
}

template<typename T, typename Set>
T& internal_get(Set &set, std::false_type /* is not wanted element */, std::false_type /* is not last element */)
{
	return internal_get<T>
	(
		set.m_next,
		typename std::is_same<T, typename Set::NextValueType>::type(), typename std::is_same<LastElement,
		typename Set::Next::NextValueType>::type()
	);
}

template<typename T, typename Set>
T& internal_get(Set &set, std::true_type /* is wanted element */, std::true_type /* is last element */)
{
	return set.m_value;
}

template<typename T, typename Set>
T& internal_get(Set &set, std::false_type /* is not wanted element */, std::true_type /* is last element */)
{
	static_assert(!sizeof(T), "Type not found in set");
}


template<typename Top, typename ...Types>
struct TypeSet
{
	typedef Top ValueType;
	ValueType m_value;

	typedef typename TypeSet<Types...>::ValueType NextValueType;
	typedef TypeSet<Types...> Next;
	Next m_next;

	template<typename T>
	T& get()
	{
		return internal_get<T>(*this, typename std::is_same<T, ValueType>::type(), std::false_type());
	}
};

template<typename Last>
struct TypeSet<Last>
{
	typedef Last ValueType;
	ValueType m_value;

	typedef LastElement NextValueType;

	template<typename T>
	T& get()
	{
		return internal_get<T>(*this, typename std::is_same<T, ValueType>::type(), std::true_type());
	}
};


template<typename TypeSet, typename Func>
void for_each(TypeSet &set, Func f)
{
	for_each_impl(set, f, typename std::is_same<LastElement, typename TypeSet::Next::NextValueType>::type());
}

template<typename TypeSet, typename Func>
void for_each_impl(TypeSet &set, Func f, std::false_type /* is not last element */)
{
	f(set.m_value);
	for_each_impl(set.m_next, f, typename std::is_same<LastElement, typename TypeSet::Next::NextValueType>::type());
}

template<typename TypeSet, typename Func>
void for_each_impl(TypeSet &set, Func f, std::true_type /* is last element */)
{
	f(set.m_value);
}


//=================================================================================================
//
//=================================================================================================

/// The Handle class represents a persistent index to an object in an array, even if objects
/// are added or removed.
class Handle
{
public:
	/// Indicates an invalid handle
	static size_t const invalid = static_cast<size_t>(-1);

	/// Constructs a new handle
	explicit Handle(size_t index = invalid) :
		m_index(index) {}

	/// Gets the index
	size_t index() const { return m_index; }
	/// Sets the index
	void index(size_t val) { m_index = val; }
	/// Checks whether the handle is valid
	bool is_valid() const { return m_index == invalid; }

private:
	size_t m_index;
};


/// The HandleTranslator converts persistent handle values into array indices which
/// may change over time.
class HandleTranslator
{
public:
	/// Maximum number of objects the HandleConverter can handle
	static size_t const max_entries = 1024;

	/// Constructor
	HandleTranslator() :
		m_next_index(0)
	{
		for(size_t entry = 0; entry < max_entries; ++entry)
		{
			m_entries[entry].m_target_index = static_cast<size_t>(-1);
			m_entries[entry].m_next_free_index = entry + 1;
			m_entries[entry].m_active = false;
		}
	}

	/// Creates a new handle which points to the index target_index. target_index must be
	/// an index larger as any other in the list.
	Handle push_back(size_t target_index)
	{
		assert(m_next_index < max_entries);
		assert(m_entries[m_next_index].m_active == false);

		m_entries[m_next_index].m_target_index = target_index;
		m_entries[m_next_index].m_active = true;

		Handle new_handle(m_next_index);
		m_next_index = m_entries[m_next_index].m_next_free_index;

		return new_handle;
	}

	/// Like @see push_back() but without any restrictions regarding target_index.
	Handle insert(size_t target_index)
	{
		auto new_handle = push_back(target_index);


		// Update all hanbdles
		for(size_t entry = 0; entry < max_entries; ++entry)
		{
			//if()
		}

		return new_handle;
	}

	/// Converts the specified handle to the corresponding array index
	size_t get(Handle index) const
	{
		assert(index.index() < max_entries);
		assert(m_entries[index.index()].m_active == true);

		return m_entries[index.index()].m_target_index;
	}

	/// Changes the index the specified handle points to (that's actually the one
	/// and only purpose of this class)
	void change(Handle index, size_t new_target_index)
	{
		assert(index.index() < max_entries);
		assert(m_entries[index.index()].m_active == true);

		m_entries[index.index()].m_target_index = new_target_index;
	}

	/// Removes the specified handle from the list
	void remove(Handle index)
	{
		assert(index.index() < max_entries);
		assert(m_entries[index.index()].m_active == true);

		m_entries[index.index()].m_target_index = static_cast<size_t>(-1);
		m_entries[index.index()].m_next_free_index = m_next_index;
		m_entries[index.index()].m_active = false;

		m_next_index = index.index();
	}

private:
	struct entry
	{
		size_t m_target_index;
		size_t m_next_free_index;
		bool m_active;
	};
	entry m_entries[max_entries];
	size_t m_next_index;
};


template<typename ...ValueTypes>
class CompoundCatalog
{
public:
	Handle add(ValueTypes const &...vals)
	{

	}


private:
	TypeSet<std::vector<ValueTypes>...> m_data;
	HandleTranslator m_handles;
};


template<typename TValueType>
class ElementCatalog
{
public:
	typedef TValueType ValueType;

	Handle add(ValueType const &val)
	{
		m_elements.push_back(Entry(val));
		auto handle = m_handles.push_back(m_elements.size() - 1);
		m_elements.back().m_handle = handle;

		return handle;
	}

	void add(Handle pos, size_t num, ValueType const *vals, Handle *handles)
	{
		assert(pos.index() + num < m_elements.size());
		assert(vals != nullptr);
		assert(handles != nullptr);

		// TODO: Improve performance and exception safety
		for(size_t i = 0; i < num; i++)
		{
			m_elements.insert(m_elements.begin() + pos.index() + i, 1, Entry(vals[i]));
			handles[i] = m_handles.push_back(pos.index() + i);
			m_elements[pos.index() + i].m_handle = handles[i];
		}

		// Update handles
		for(size_t i = pos.index() + num; i < m_elements.size(); i++)
			m_handles.change(m_elements[i].m_handle, i);
	}

	ValueType* get(Handle index)
	{
		return &m_elements[m_handles.get(index)].m_value;
	}

	ValueType const* get(Handle index) const
	{
		return &m_elements[m_handles.get(index)].m_value;
	}


	ValueType& operator [] (size_t index) { return m_elements[index].m_value; }
	ValueType const& operator [] (size_t index) const { return m_elements[index].m_value; }

private:
	struct Entry
	{
		Entry(ValueType const &value) :
			m_value(value) {}

		ValueType m_value;
		Handle m_handle;
	};

	HandleTranslator m_handles;
	::std::vector<Entry> m_elements;
};


template<typename ValueType>
class range
{
	typedef ValueType value_type;

	size_t size() { return m_end - m_begin; }
	value_type* begin() { return m_begin; }
	value_type* end() { return m_end; }

private:
	value_type *m_begin, *m_end;
};


//=================================================================================================
//
//=================================================================================================
struct SpatialBase
{
	Handle m_superior;
	Handle m_inferior;
	size_t m_num_children;
};

struct SpatialPosition
{
	red::vector2f m_position;
	red::vector2f m_bounding_box;
	red::vector2f m_world_position;
};

struct SpatialZIndex
{
	light::uint4 m_z_offset;
	light::uint4 m_depth;
	light::uint4 m_additional_depth;
	light::uint4 m_world_z_index;
};


class SpatialCatalog
{
public:
	Handle add()
	{
		return Handle();
	}


	void update_z()
	{

	}

private:

};


class SpatialHandle
{
public:
	SpatialHandle(Handle spatial, SpatialCatalog &catalog) :
		m_spatial(spatial),
		m_catalog(catalog) {}

	/// Gets the handle
	Handle spatial() const { return m_spatial; }

	// Gets spatial data
	red::vector2f position() const
	{
		//return get_data()->m_position;
		return red::vector2f();
	}
	red::vector2f bounding_box() const
	{
		//return get_data()->m_bounding_box;
		return red::vector2f();
	}

private:
	Handle m_spatial;
	SpatialCatalog &m_catalog;

	//struct Spatial* get_data() { return m_catalog.get(m_spatial); }
	//struct Spatial const* get_data() const { return m_catalog.get(m_spatial); }
};


//=================================================================================================
//
//=================================================================================================
class RectangleRenderer
{
public:
	RectangleRenderer(sf::RenderWindow &win) :
		m_window(win) {}

	void add(red::vector2f pos, red::vector2f dim, light::uint4 z_index)
	{
		sf::RectangleShape rectangle;
		rectangle.setSize(sf::Vector2f(dim.x(), dim.y()));
		rectangle.setFillColor(sf::Color::Red);
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
class widget_catalog_base
{
public:
	virtual void render() = 0;
};

template<typename CatalogType>
class widget_catalog_common
{
public:
	typedef CatalogType catalog_type;

	widget_catalog_common(catalog_type *catalog) :
		m_catalog(catalog) {}

	virtual void render()
	{
		m_catalog->render();
	}

private:
	catalog_type *m_catalog;
};

class widget_catalog
{
public:
	template<typename CatalogType>
	explicit widget_catalog(CatalogType *catalog) :
		m_catalog(new widget_catalog_common<CatalogType>(catalog)) {}

private:
	::std::unique_ptr<widget_catalog_base> m_catalog;
};





enum event_type
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
	mouse_release = 1024
};

struct event_entry
{
	Handle m_parent;
	Handle m_spatial;
	int m_events;
	//Group m_group;
};


enum class mouse_button
{
	left,
	middle,
	right
};


class InputCatalog : public ElementCatalog<event_entry>
{
public:
	InputCatalog()
	{

	}

	void mouse_button_press(mouse_button but)
	{
		Handle current;
		while(current.is_valid())
		{
			auto element = get(current);
			element->m_events |= mouse_press;
			current = element->m_parent;
		}
	}

private:
	Handle m_focused;
};


class InputHandle
{
public:
	InputHandle(Handle input, InputCatalog &catalog) :
		m_input(input),
		m_catalog(catalog) {}

	// Gets the handle
	Handle input() const { return m_input; }

private:
	Handle m_input;
	InputCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
enum ButtonState
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
	Button(Handle pos, Handle event_data) :
		m_position(pos) {}

	Handle m_position;
	ButtonState m_state;
	Handle m_event_data;
};

class ButtonCatalog : public ElementCatalog<Button>
{
public:
	ButtonCatalog(SpatialCatalog const &spatials, RectangleRenderer &renderer) :
		m_spatials(spatials),
		m_renderer(renderer) {}


	// Update/render whole list
	void update()
	{

	}
	void render()
	{
		/*for(auto const &but: m_elements)
		{
			Spatial const *spat = m_spatials.get(but.m_position);
			m_renderer.add(spat->m_world_position, spat->m_bounding_box, spat->m_world_z_index);
		}*/
	}

private:
	SpatialCatalog const &m_spatials;
	RectangleRenderer &m_renderer;
};


class ButtonHandle : public SpatialHandle
{
public:
	ButtonHandle(
		Handle spatial, SpatialCatalog &spat_cat,
		Handle button, ButtonCatalog &but_cat
	) :
		SpatialHandle(spatial, spat_cat),
		m_button(button),
		m_catalog(but_cat) {}

	Handle button() const { return m_button; }


private:
	Handle m_button;
	ButtonCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
struct window_entry
{
	window_entry(Handle pos, Handle event_data) :
		m_position(pos),
		m_event_data(event_data) {}

	Handle m_position;
	Handle m_event_data;
};

class WindowCatalog : public ElementCatalog<window_entry>
{
public:
	WindowCatalog(SpatialCatalog const &spatials, RectangleRenderer &renderer) :
		m_spatials(spatials),
		m_renderer(renderer) {}


	// Update/render whole list
	void update()
	{

	}
	void render()
	{
		/*for(auto const &win: m_elements)
		{
			Spatial const *spat = m_spatials.get(win.m_position);
			m_renderer.add(spat->m_world_position, spat->m_bounding_box, spat->m_world_z_index);
		}*/
	}

private:
	SpatialCatalog const &m_spatials;
	RectangleRenderer &m_renderer;
};


class WindowHandle : public SpatialHandle
{
public:
	WindowHandle(
		Handle spatial, SpatialCatalog &spat_cat,
		Handle window, WindowCatalog &win_cat
	) :
		SpatialHandle(spatial, spat_cat),
		m_window(window),
		m_catalog(win_cat) {}

	Handle window() const { return m_window; }


private:
	Handle m_window;
	WindowCatalog &m_catalog;
};


//=================================================================================================
//
//=================================================================================================
/*class GuiMananger
{
public:
	GuiMananger(RectangleRenderer &renderer) :
		m_input_data(),
		m_spatial_data(),
		m_button_data(m_spatial_data, renderer),
		m_window_data(m_spatial_data, renderer) {}

	ButtonHandle add_button(red::vector2f pos, red::vector2f dim, Handle spatial_parent = invalid_handle)
	{
		Spatial but_spat(Group::none, pos, dim);

		Handle spat_handle;
		if(spatial_parent != invalid_handle)
			m_spatial_data.add(spatial_parent, 1, &but_spat, &spat_handle);
		else
			spat_handle = m_spatial_data.add(but_spat);

		Button but_dat(spat_handle, static_cast<size_t>(-1));

		Handle but_handle =  m_button_data.add(but_dat);
		return ButtonHandle(spat_handle, m_spatial_data, but_handle, m_button_data);
	}

	WindowHandle add_window(red::vector2f pos, red::vector2f dim, Handle spatial_parent = invalid_handle)
	{
		Spatial win_begin(Group::begin, pos, dim);
		Spatial win_end(Group::end, pos, dim);

		Handle spat_handle;
		if(spatial_parent != invalid_handle)
		{
			m_spatial_data.add(spatial_parent, 1, &win_begin, &spat_handle);
			Handle dummy;
			m_spatial_data.add(spatial_parent, 1, &win_end, &dummy);
		}
		else
		{
			spat_handle = m_spatial_data.add(win_begin);
			m_spatial_data.add(win_end);
		}

		window_entry win_dat(spat_handle, static_cast<size_t>(-1));

		Handle win_handle = m_window_data.add(win_dat);
		return WindowHandle(spat_handle, m_spatial_data, win_handle, m_window_data);
	}

	window_entry* get_window(Handle h)
	{
		return m_window_data.get(h);
	}

	void update()
	{
		m_spatial_data.update();
	}

	void render()
	{
		m_window_data.render();
		m_button_data.render();
	}

private:
	InputCatalog m_input_data;
	SpatialCatalog m_spatial_data;
	ButtonCatalog m_button_data;
	WindowCatalog m_window_data;
};*/


//=================================================================================================
//
//=================================================================================================
template<typename Type>
struct size_of
{
	template<size_t Size>
	struct is;

	typedef typename is<sizeof(Type)>::byte size;
};

//size_of<spatial>::size;




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
	dev->write(str, strlen(str));
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

	/*sf::RenderWindow window(sf::VideoMode(800, 600), "My window");
	RectangleRenderer renderer(window);


	GuiMananger gui_data(renderer);

	auto win_handle = gui_data.add_window({100.f, 100.f}, {300.f, 300.f});
	auto but_handle = gui_data.add_button({100.f, 100.f}, {200.f, 200.f}, win_handle.spatial());



	// run the program as long as the window is open
	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}


		gui_data.update();
		gui_data.render();


		window.clear();
		renderer.display();
		renderer.clear();
		window.display();
	}*/


	return 0;
}


