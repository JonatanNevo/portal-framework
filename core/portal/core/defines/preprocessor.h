//
// Created by thejo on 5/4/2025.
//

#pragma once

// Turns an preprocessor token into a real string
#define PORTAL_STRINGIZE(token) PORTAL_PRIVATE_STRINGIZE(token)
#define PORTAL_PRIVATE_STRINGIZE(token) #token

// Concatenates two preprocessor tokens, performing macro expansion on them first
#define PORTAL_JOIN(token_a, token_b) PORTAL_PRIVATE_JOIN(token_a, token_b)
#define PORTAL_PRIVATE_JOIN(token_a, token_b) token_a##token_b

// Creates a string that can be used to include a header in the form "Platform/PlatformHeader.h", like "Windows/WindowsPlatformFile.h"
#define COMPILED_PLATFORM_HEADER(prefix, suffix) PORTAL_STRINGIZE(PORTAL_JOIN(PORTAL_JOIN(portal/platform/prefix/PORTAL_PLATFORM/PORTAL_PLATFORM, _), suffix))

