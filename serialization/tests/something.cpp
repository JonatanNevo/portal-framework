//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <vector>
#include <iostream>
#include <glm/glm.hpp>

struct TestObjectMinimal
{
    int id = 0;
    std::string name{};
    glm::vec3 position{0.0f, 0.0f, 0.0f};
};

int main()
{
    std::cout << "sizeof: " << sizeof(TestObjectMinimal) << '\n';
    std::vector<TestObjectMinimal> vec(2);
    std::cout << "span: " << (reinterpret_cast<char*>(&vec[1]) - reinterpret_cast<char*>(&vec[0])) << '\n';
    vec[0] = {1, "A", {1,0,0}};
    vec[1] = {2, "B", {0,1,0}};  // Does this crash?
    std::cout << "success\n";
}