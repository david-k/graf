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

#include <vector>

#include <light/utility/array_set.hpp>


/// The Handle class represents a persistent index to an object in an array, even if objects
/// are added or removed.
template<typename T>
class Handle
{
	static size_t const invalid_index = static_cast<size_t>(-1);

public:
	/// Indicates an invalid handle
	static Handle const invalid;

	/// Constructs a new handle
	explicit Handle(size_t index = invalid_index) :
		m_index(index) {}

	bool operator == (Handle const &rhs) const { return m_index == rhs.m_index; }
	bool operator != (Handle const &rhs) const { return m_index != rhs.m_index; }

	/// Gets the index
	size_t index() const { return m_index; }
	/// Sets the index
	void index(size_t val) { m_index = val; }
	/// Checks whether the handle is valid
	bool is_valid() const { return *this != invalid; }

private:
	size_t m_index;
};

template<typename T>
Handle<T> const Handle<T>::invalid = Handle<T>(Handle<T>::invalid_index);


/// The HandleTranslator converts persistent handle values into array indices which
/// may change over time.
template<typename THandle>
class HandleTranslator
{
public:
	typedef THandle HandleType;

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

	/// Creates a new handle which points to the index target_index
	HandleType add(size_t target_index)
	{
		assert(m_next_index < max_entries);
		assert(m_entries[m_next_index].m_active == false);

		m_entries[m_next_index].m_target_index = target_index;
		m_entries[m_next_index].m_active = true;

		HandleType new_handle(m_next_index);
		m_next_index = m_entries[m_next_index].m_next_free_index;

		return new_handle;
	}

	/// Converts the specified handle to the corresponding array index
	size_t get(HandleType index) const
	{
		assert(index.index() < max_entries);
		assert(m_entries[index.index()].m_active == true);

		return m_entries[index.index()].m_target_index;
	}

	/// Changes the index the specified handle points to (that's actually the one
	/// and only purpose of this class)
	void change(HandleType index, size_t new_target_index)
	{
		assert(index.index() < max_entries);
		assert(m_entries[index.index()].m_active == true);

		m_entries[index.index()].m_target_index = new_target_index;
	}

	/// Removes the specified handle from the list
	void remove(HandleType index)
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


template<typename THandle, typename ...TValueTypes>
class CatalogSet
{
public:
	typedef THandle HandleType;
	typedef light::array_set<TValueTypes...> CompoundType;

	HandleType add(TValueTypes const &...vals)
	{
		m_elements.add(vals...);
		auto handle = m_handles.add(m_elements.size() - 1);
		m_index_to_handle.push_back(handle);

		assert(m_elements.size() == m_index_to_handle.size());

		return handle;
	}

	void add(size_t pos, size_t num, TValueTypes const *...vals, HandleType *handles)
	{
		assert(pos <= m_elements.size());
		//assert((vals != nullptr)...);
		assert(handles != nullptr);

		// TODO: Think about exception safeness
		m_elements.insert(pos, num, vals...);

		for(size_t i = 0; i < num; i++)
		{
			handles[i] = m_handles.add(pos + i);
			m_index_to_handle.insert(m_index_to_handle.begin() + pos + i, handles[i]);
		}

		assert(m_elements.size() == m_index_to_handle.size());

		// Update handles
		for(size_t i = pos + num; i < m_elements.size(); i++)
			m_handles.change(m_index_to_handle[i], i);
	}

	size_t size() const
	{
		return m_elements.size();
	}

	template<typename T>
	T& get(HandleType h)
	{
		assert(h.is_valid());

		auto index = m_handles.get(h);
		assert(index < m_elements.size());

		return m_elements.template array<T>()[index];
	}

	template<typename T>
	T const& get(HandleType h) const
	{
		assert(h.is_valid());

		auto index = m_handles.get(h);
		assert(index < m_elements.size());

		return m_elements.template array<T>()[index];
	}


	template<typename T>
	T& at(size_t index)
	{
		assert(index < m_elements.size());
		return m_elements.template array<T>()[index];
	}

	template<typename T>
	T const& at(size_t index) const
	{
		assert(index < m_elements.size());
		return m_elements.template array<T>()[index];
	}


	template<typename T>
	typename CompoundType::template vector_type<T>::iterator begin()
	{
		return m_elements.template array<T>().begin();
	}

	template<typename T>
	typename CompoundType::template vector_type<T>::iterator end()
	{
		return m_elements.template array<T>().end();
	}


	size_t get_index(HandleType h) const { return m_handles.get(h); }
	HandleType get_handle(size_t index) const
	{
		assert(index < m_index_to_handle.size());
		return m_index_to_handle[index];
	}

private:
	HandleTranslator<HandleType> m_handles;
	CompoundType m_elements;
	std::vector<HandleType> m_index_to_handle;
};
