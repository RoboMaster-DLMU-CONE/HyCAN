include(FetchContent)
execute_process(
        COMMAND git ls-remote --heads https://github.com/mutouyun/cpp-ipc master
        RESULT_VARIABLE cpp_ipc_primary_repo_ok
        OUTPUT_QUIET
        ERROR_QUIET
)
if (NOT cpp_ipc_primary_repo_ok EQUAL 0)
    message(STATUS "Primary fetch failed, falling back to Gitee mirror")
    set(cpp_ipc_repo https://gitee.com/dlmu-cone/cpp-ipc)
else ()
    set(cpp_ipc_repo https://github.com/mutouyun/cpp-ipc master)
endif ()

FetchContent_Declare(cpp_ipc
        GIT_REPOSITORY ${cpp_ipc_repo}
        GIT_TAG master
)
FetchContent_MakeAvailable(cpp_ipc)

# Create alias with namespace for proper export
if (TARGET ipc AND NOT TARGET cpp-ipc::ipc)
    add_library(cpp-ipc::ipc ALIAS ipc)
endif ()
