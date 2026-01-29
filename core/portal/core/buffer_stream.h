//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <streambuf>
#include <ostream>
#include <istream>

#include "buffer.h"

namespace portal
{
class BufferStreamReader final : std::streambuf, public std::istream
{
public:
    explicit BufferStreamReader(const Buffer& buffer);
    [[nodiscard]] size_t position() const { return _position; }

protected:
    std::streamsize xsgetn(char* s, std::streamsize n) override;
    std::streampos seekoff(std::streamoff off, seekdir dir, openmode which) override;

private:
    const Buffer& buffer;
    size_t _position;
};

class BufferStreamWriter final : std::streambuf, public std::ostream
{
public:
    explicit BufferStreamWriter(Buffer& buffer);
    [[nodiscard]] Buffer get_buffer() const { return Buffer::copy(buffer.data, get_position()); }

    [[nodiscard]] size_t size() const { return get_position(); }
    [[nodiscard]] size_t capacity() const { return buffer.size; }

protected:
    std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
    std::streamsize xsputn(const char* s, std::streamsize n) override;

private:
    void grow(size_t min_capacity);
    [[nodiscard]] size_t get_position() const { return pptr() - pbase(); }

private:
    Buffer& buffer;
};
}
