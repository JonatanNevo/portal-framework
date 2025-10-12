//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <Windows.h>

#include "string_utils.h"

#include <vector>

namespace portal::details
{

std::wstring to_wstring(const std::string& str, const Encoding encoding)
{
    int encoding_num = 0;
    switch (encoding)
    {
    case Encoding::UTF8:
        encoding_num = CP_UTF8;
        break;
    case Encoding::ANSI:
        encoding_num = CP_ACP;
        break;
    }

    const int string_len = static_cast<int>(str.length()) + 1;
    const int len = MultiByteToWideChar(encoding_num, 0, str.c_str(), string_len, nullptr, 0);

    std::vector<wchar_t> buf(len);
    MultiByteToWideChar(encoding_num, 0, str.c_str(), string_len, buf.data(), len);
    return std::wstring(buf.data(), buf.data() + len);
}
}
