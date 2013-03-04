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


//=================================================================================================
// Abstract device
//=================================================================================================
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
