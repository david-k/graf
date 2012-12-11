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


//=============================================================================================
//
//=============================================================================================
struct chunk_wise_tag {};
struct random_access_tag : chunk_wise_tag {};
struct direct_access_tag : random_access_tag {};

struct binary_content_tag {};
struct text_content_tag {};

struct io_result
{
	// Digression: The English language
	//    - status: classification of state among several well-defined possibilities.
	//    - state: a durable or lasting condition
	// See http://forum.wordreference.com/showthread.php?t=287984&langid=3
	//     http://english.stackexchange.com/questions/12958/status-vs-state
	enum state {ok, eof, error};
	state status;
	size_t amount;
};

template<typename CharType, typename TraitsType = std::char_traits<CharType> >
class string_device
{
public:
	typedef CharType char_type;
	typedef TraitsType traits_type;
	typedef direct_access_tag category;
	typedef text_content_tag content;

	typedef std::basic_string<char_type, traits_type> string_type;

	string_device(string_type *dev) :
		m_device(dev),
		m_pos(&m_device->front()){}

	// Chunk-wise category
	io_result read(char_type *dest, size_t size);
	io_result write(char_type const *dest, size_t size);

	// Random access category
	size_t size() const;
	void seek(size_t pos);

	// Direct access category
	char_type* data();
	char_type const* data() const;
	char_type const* cdata() const;

private:
	string_type *m_device;
	char_type m_pos;
};


template<typename Device>
void write_content(Device dev, char const *str, text_content_tag)
{
	dev.write
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
typedef size_t handle;

template<typename TargetType>
class handle_references
{
public:
	static size_t const max_entries = 1024;
	typedef TargetType target_type;

	handle_references() :
		m_next_index(0)
	{
		for(size_t entry = 0; entry < max_entries; ++entry)
		{
			m_entries[entry].m_target = nullptr;
			m_entries[entry].m_next_free_index = entry + 1;
			m_entries[entry].m_active = false;
		}
	}

	handle add(target_type *target)
	{
		assert(m_next_index < max_entries);
		assert(m_entries[m_next_index].m_active == false);

		m_entries[m_next_index].m_target = target;
		m_entries[m_next_index].m_active = true;

		m_next_index = m_entries[m_next_index].m_next_free_index;
	}

	target_type* get(handle index) const
	{
		assert(index < max_entries);
		assert(m_entries[index].m_active == true);

		return m_entries[index].m_target;
	}

	void change(handle index, target_type *new_target)
	{
		assert(index < max_entries);
		assert(m_entries[index].m_active == true);

		m_entries[index].m_target = new_target;
	}

	void remove(handle index)
	{
		assert(index < max_entries);
		assert(m_entries[index].m_active == true);

		m_entries[index].m_target = nullptr;
		m_entries[index].m_next_free_index = m_next_index;
		m_entries[index].m_active = false;

		m_next_index = index;
	}

private:
	struct entry
	{
		target_type *m_target;
		size_t m_next_free_index;
		bool m_active;
	};
	entry m_entries[max_entries];
	size_t m_next_index;
};




//=================================================================================================
//
//=================================================================================================
enum class group : char { none, begin, end };
struct spatial
{
	spatial
	(
		group type,
		red::vector2f const &pos = red::vector2f(),
		red::vector2f const &bounding = red::vector2f()
	) :
		m_position(pos),
		m_bounding_box(bounding),
		m_world_position(bounding),
		m_z_index(.0f),
		m_group(type) {}


	red::vector2f m_position;
	red::vector2f m_bounding_box;
	red::vector2f m_world_position;
	float m_z_index;
	group m_group;
};

class spatial_catalog
{
public:
	handle add(spatial const &s)
	{
		m_entries.push_back(s);
		return m_handles.add(&m_entries.back());
	}

	void update()
	{
		auto parrent_pos = red::vector2f(.0f, .0f);
		for(size_t i = 0; i < m_entries.size(); i++)
		{
			m_entries[i].m_world_position = parrent_pos + m_entries[i].m_position;
			if(m_entries[i].m_group == group::begin)
				parrent_pos += m_entries[i].m_position;
			else if(m_entries[i].m_group == group::end)
				parrent_pos -= m_entries[i].m_position;
		}
	}

private:
	handle_references<spatial> m_handles;
	std::vector<spatial> m_entries;
};


//=================================================================================================
//
//=================================================================================================
struct button
{
	button(handle pos) :
		m_position(pos) {}

	handle m_position;
};

class button_catalog
{
public:
	handle add(button const &s)
	{
		m_entries.push_back(s);
		return m_handles.add(&m_entries.back());
	}

	void update()
	{

	}

	void render(handle_references<spatial> const &spat_cat)
	{
		for(auto const &but: m_entries)
		{
			spatial const *spat = spat_cat.get(but.m_position);
		}
	}

private:
	handle_references<button> m_handles;
	std::vector<button> m_entries;
};




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

			glBegin(GL_TRIANGLES);
			glVertex3f(-0.5f, 0.5f, 0.1);
			glVertex3f(0.5f, 0.5f, 0.1);
			glVertex3f(0.5f, -0.5f, 0.1);
			glVertex3f(-0.5f, -0.5f, 0.1);
			glEnd();

			render_win.swap_buffers();
		}
	}
	catch(::std::exception const &e)
	{
		GRAF_ERROR_MSG("Unhandled exception: {}\n", e.what());
	}

	return 0;
}


