//
// Created by thejo on 3/21/2025.
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "portal/core/log.h"

// We need this due to a bug in vcpkg in windows with gtest, which renders the target `GTest::gtest_main` unusable.

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    portal::Log::init();
    return RUN_ALL_TESTS();
}