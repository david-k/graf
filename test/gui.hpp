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

#pragma once

#include <light/string/string.hpp>


namespace gui
{
	using namespace ::light::types;

	//=================================================================================================
	// This class handles the way how a widget writes to your data. The widget can either write to
	// your data (pretty convenient) or only after a call to sync (good if you are running multiple
	// threads).
	//=================================================================================================
	enum class data_usage { shared, independent };


	template<typename DataType, data_usage Usage>
	class data_target;


	//-----------------------------------------------------------------------------
	// Directly writing to data
	//-----------------------------------------------------------------------------
	template<typename DataType>
	class data_target<DataType, data_usage::shared> : light::non_copyable
	{
	public:
		typedef DataType data_type;

		// All data is directly written to 'shared'
		explicit data_target(data_type &shared) :
			m_shared(shared)
		{

		}

		// Assigns a new value
		data_target& operator = (data_type const &val)
		{
			m_shared = val;
			return *this;
		}

		// Gets the value
		operator data_type& () { return m_shared; }
		operator data_type const& () const { return m_shared; }

		// No op
		void sync_data() {}
		void sync_dispaly() {}

	private:
		data_type &m_shared;
	};


	//-----------------------------------------------------------------------------
	// Writing to data after call to sync()
	//-----------------------------------------------------------------------------
	template<typename DataType>
	class data_target<DataType, data_usage::independent> : light::non_copyable
	{
	public:
		typedef DataType data_type;

		// 'shared' is only updated after calling sync()
		explicit data_target(data_type &shared) :
			m_shared(shared),
			m_working_copy(m_shared)
		{

		}

		// Assigns a new value to internal copy
		data_target& operator = (data_type const &val)
		{
			m_working_copy = val;
			return *this;
		}

		// Gets value of internal copy
		operator data_type& () { return m_working_copy; }
		operator data_type const& () const { return m_working_copy; }

		// Updates data
		void sync_data() { m_shared = m_working_copy; }
		void sync_display() { m_working_copy = m_shared; }

	private:
		data_type &m_shared;
		data_type m_working_copy;
	};


	//=================================================================================================
	// Represents the intermediary between user data and widget.
	//=================================================================================================
	template<typename DisplayType>
	class intermediary
	{
	public:
		// The type being displayed (if your data is of type int the display_type is still string because that is what is displayed.
		// But display_type can also be an image etc.)
		typedef DisplayType display_type;

		virtual ~intermediary() {}

		// The widget calls this method to update the users data (if display_data is of a different type it is automatically converted).
		virtual void push(display_type const &value) = 0;

		// The widget calls this method to get the users data. (if the user data is of a different type it is automatically converted).
		virtual display_type pull() = 0;

		// This method can be used to modify the data directly, though it's only possible if the widget knows the type of the
		// underlying data.
		template<typename Type>
		Type& modify()
		{
			Type &data = *static_cast<Type*>(get_pointer(typeid(Type).hash_code()));
			return data;
		}

	protected:
		// Gets a pointer to the data. If the type hashes differ throws an exception
		virtual void* get_pointer(size_t type_hash) = 0;
	};


	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	template<typename DataType, data_usage Usage>
	class text_intermediary : public intermediary<light::utf8_string>
	{
	public:
		typedef DataType data_type;
		typedef typename intermediary<light::utf8_string>::display_type display_type;
		typedef data_target<data_type, Usage> usage_type;

		text_intermediary(data_type &data) :
			m_data(data)
		{

		}

		text_intermediary(data_type const &data) :
			m_data(data)
		{

		}

		virtual void push(const display_type &value)
		{
			light::scanf_from(light::make_reader(value), "{}", static_cast<data_type&>(m_data));
		}

		virtual display_type pull()
		{
			display_type display;
			light::print_to(light::make_readwriter(display), static_cast<data_type>(m_data));
			return display;
		}

	protected:
		virtual void* get_pointer(size_t type_hash)
		{
			if(type_hash != typeid(data_type).hash_code())
				throw light::runtime_error("Type mismatch");

			return &static_cast<data_type&>(m_data);
		}

	private:
		usage_type m_data;
	};


	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	class image
	{
	public:
		void pixel(int, int) {}
	};

	template<data_usage Usage>
	class image_intermediary : public intermediary<image>
	{
	public:
		typedef image data_type;
		typedef typename intermediary<image>::display_type display_type;
		typedef data_target<data_type, Usage> usage_type;

		image_intermediary(data_type &data) :
			m_data(data)
		{

		}

		image_intermediary(data_type const &data) :
			m_data(data)
		{

		}

		virtual void push(const display_type &)
		{

		}

		virtual display_type pull()
		{
			return display_type();
		}

	protected:
		virtual void* get_pointer(size_t type_hash)
		{
			if(type_hash != typeid(data_type).hash_code())
				throw light::runtime_error("Type mismatch");

			return &static_cast<data_type&>(m_data);
		}

	private:
		usage_type m_data;
	};


	//=================================================================================================
	//
	//=================================================================================================
	class text_widget
	{
	public:
		text_widget(intermediary<light::utf8_string> &data) :
			m_data(data)
		{

		}

		void text(light::utf8_string const &str)
		{
			m_data.push(str);
		}

	private:
		intermediary<light::utf8_string> &m_data;
	};


	class image_widget
	{
	public:
		image_widget(intermediary<image> &data) :
			m_data(data)
		{

		}

		void pixel(int x, int y)
		{
			m_data.modify<image>().pixel(x, y);
		}

	private:
		intermediary<image> &m_data;
	};

} // namespace: graf
