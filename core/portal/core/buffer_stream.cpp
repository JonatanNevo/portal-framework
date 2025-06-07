//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "buffer_stream.h"

namespace portal
{
BufferStreamReader::BufferStreamReader(const Buffer& buffer): std::istream(this), buffer(buffer), _position(0)
{
    if (buffer.data)
        setg(
            static_cast<char*>(buffer.data),
            static_cast<char*>(buffer.data),
            static_cast<char*>(buffer.data) + buffer.size
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
    setg(static_cast<char*>(buffer.data),
         static_cast<char*>(buffer.data) + _position,
         static_cast<char*>(buffer.data) + buffer.size);
    return new_pos;
}

BufferStreamWriter::BufferStreamWriter(Buffer& buffer): std::ostream(this), buffer(buffer), position(0)
{
    if (buffer.data)
        setp(
            static_cast<char*>(buffer.data),
            static_cast<char*>(buffer.data) + buffer.size
        );
}

std::streambuf::int_type BufferStreamWriter::overflow(std::streambuf::int_type)
{
    return traits_type::eof();
}

std::streamsize BufferStreamWriter::xsputn(const char* s, std::streamsize n)
{
    if (position + n > buffer.size)
        return 0;

    buffer.write(s, n, position);
    position += n;
    pbump(static_cast<int>(n));
    return n;
}
}
