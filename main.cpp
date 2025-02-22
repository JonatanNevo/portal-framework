//
// Created by Jonatan Nevo on 31/01/2025.
//


#include <iostream>
#include <filesystem>
#include <fstream>

#include "portal/serialization/binary_searilization.h"

inline std::ostream& operator<<(std::ostream& os, const std::vector<int> vec)
{
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        os << vec[i];
        if (i != vec.size() - 1)
            os << ", ";
    }
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec1 vec)
{
    os << "(" << vec.x << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::ivec2 vec)
{
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::dvec3 vec)
{
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4 vec)
{
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
    return os;
}

int main()
{
    std::stringstream ss;
    portal::BinarySerializer serializer(ss);
    int a = 5;
    serializer.add_property(a);
    float b = 3.14f;
    serializer.add_property(b);
    std::vector<int> c = {1, 2, 3};
    serializer.add_property(c);
    std::string d = "hello";
    serializer.add_property(d);
    glm::vec1 e{1.0f};
    serializer.add_property(e);
    glm::ivec2 f{1, 2};
    serializer.add_property(f);
    glm::dvec3 g{1.0, 2.0, 3.0};
    serializer.add_property(g);
    glm::vec4 h{1.0f, 2.0f, 3.0f, 4.0f};
    serializer.add_property(h);

    serializer.serialize();
    std::string s = ss.str();

    portal::BinaryDeserializer deserializer(s.data(), s.size());
    deserializer.deserialize();
    std::cout << "a: " << deserializer.get_property<int>() << std::endl;
    std::cout << "b: " << deserializer.get_property<float>() << std::endl;
    std::cout << "c: " << deserializer.get_property<std::vector<int>>() << std::endl;;
    std::cout << "d: " << deserializer.get_property<std::string>() << std::endl;
    std::cout << "e: " << deserializer.get_property<glm::vec1>() << std::endl;
    std::cout << "f: " << deserializer.get_property<glm::ivec2>() << std::endl;
    std::cout << "g: " << deserializer.get_property<glm::dvec3>() << std::endl;
    std::cout << "h: " << deserializer.get_property<glm::vec4>() << std::endl;
    return 0;
}