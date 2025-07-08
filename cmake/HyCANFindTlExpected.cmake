cmake_policy(PUSH)
cmake_policy(SET CMP0135 NEW)
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)
include(FetchContent)
FetchContent_Declare(
        tl-expected
        GIT_REPOSITORY "https://github.com/TartanLlama/expected"
        GIT_TAG "master"
)
FetchContent_MakeAvailable(tl-expected)
cmake_policy(POP)