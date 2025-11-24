cmake_policy(SET CMP0135 NEW)
include(FetchContent)

find_package(tl-expected REQUIRED)
if (NOT TARGET tl::expected)
    execute_process(
            COMMAND git ls-remote --heads https://github.com/TartanLlama/expected master
            RESULT_VARIABLE tl_expected_primary_repo_ok
            OUTPUT_QUIET
            ERROR_QUIET
    )
    if (NOT tl_expected_primary_repo_ok EQUAL 0)
        message(STATUS "Primary fetch failed, falling back to Gitee mirror")
        set(tl_expected_repo https://gitee.com/dlmu-cone/expected.git)
    else ()
        set(tl_expected_repo https://github.com/TartanLlama/expected)
    endif ()

    FetchContent_Declare(tl_expected
            GIT_REPOSITORY ${tl_expected_repo}
            GIT_TAG master
    )

    FetchContent_GetProperties(tl_expected)
    if (NOT tl_expected_POPULATED)
        FetchContent_Populate(tl_expected)
    endif ()

    if (NOT TARGET tl_expected)
        add_library(tl_expected INTERFACE)
        target_include_directories(tl_expected INTERFACE
                $<BUILD_INTERFACE:${tl_expected_SOURCE_DIR}/include>
        )
        add_library(tl::expected ALIAS tl_expected)
    endif ()
endif ()