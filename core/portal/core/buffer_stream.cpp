//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "buffer_stream.h"

namespace portal
{
BufferStreamReader::BufferStreamReader(const Buffer& buffer) : std::istream(this), buffer(buffer), _position(0)
{
    if (buffer.data)
        setg(
            buffer.as<char*>(),
            buffer.as<char*>(),
            buffer.as<char*>() + buffer.size
        );
}

std::streamsize BufferStreamReader::xsgetn(char* s, std::streamsize n)
{
    const auto remaining = buffer.size - _position;
    const auto to_read = (std::min)(static_cast<size_t>(n), remaining);

    if (to_read > 0)
    {
        std::memcpy(s, static_cast<const char*>(buffer.data) + _position, to_read);
        _position += to_read;
        gbump(static_cast<int>(to_read));
    }
    return to_read;
}

std::streampos BufferStreamReader::seekoff(const std::streamoff off, const seekdir dir, const openmode which)
{
    if (!(which & in))
        return std::streampos(-1);

    std::streampos new_pos;

    switch (dir)
    {
    case beg:
        new_pos = off;
        break;
    case cur:
        new_pos = _position + off;
        break;
    case end:
        new_pos = buffer.size + off;
        break;
    default:
        return {-1};
    }

    if (new_pos < 0 || new_pos > static_cast<std::streampos>(buffer.size))
        return {-1};

    _position = new_pos;
    setg(
        buffer.as<char*>(),
        buffer.as<char*>() + _position,
        buffer.as<char*>() + buffer.size
    );
    return new_pos;
}

BufferStreamWriter::BufferStreamWriter(size_t initial_capacity)
    : std::ostream(this),
      managed_buffer(Buffer::allocate(initial_capacity))
{
    if (managed_buffer.data)
        setp(
            managed_buffer.as<char*>(),
            managed_buffer.as<char*>() + managed_buffer.size
        );
}

std::streambuf::int_type BufferStreamWriter::overflow(std::streambuf::int_type ch)
{
    if (ch == traits_type::eof())
        return traits_type::eof();

    const size_t current_pos = get_position();

    // Grow the buffer
    grow(current_pos + 1);

    // Write the character
    *pptr() = static_cast<char>(ch);
    pbump(1);

    return ch;
}

std::streamsize BufferStreamWriter::xsputn(const char* s, std::streamsize n)
{
    const size_t current_pos = get_position();
    const size_t required_size = current_pos + n;

    // Grow if necessary
    if (required_size > managed_buffer.size)
        grow(required_size);

    std::memcpy(pptr(), s, n);
    pbump(static_cast<int>(n));

    return n;
}

void BufferStreamWriter::grow(size_t min_capacity)
{
    const size_t current_pos = get_position();

    // Grow by 1.5x or to min_capacity, whichever is larger
    const size_t new_capacity = (std::max)(min_capacity, managed_buffer.size + managed_buffer.size / 2);

    Buffer new_buffer = Buffer::allocate(new_capacity);
    if (current_pos > 0 && managed_buffer.data)
        std::memcpy(new_buffer.data_ptr(), managed_buffer.data, current_pos);

    managed_buffer = std::move(new_buffer);

    // Reset stream buffer pointers
    setp(
        managed_buffer.as<char*>(),
        managed_buffer.as<char*>() + managed_buffer.size
    );

    // Advance to current position
    pbump(static_cast<int>(current_pos));
}
}
