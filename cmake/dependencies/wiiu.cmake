#=================== nlohmann-json ===================
find_package(nlohmann_json QUIET)
if (NOT ${nlohmann_json_FOUND})
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

#=================== spdlog ===================
find_package(spdlog QUIET)
if (NOT ${spdlog_FOUND})
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
        OVERRIDE_FIND_PACKAGE
        PATCH_COMMAND patch -p1 -i "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/spdlog-wiiu.patch"
        # don't try to apply the patch multiple times https://stackoverflow.com/a/73725257
        UPDATE_DISCONNECTED 1
    )

    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_BUILD_TESTS "" OFF)
    option(SPDLOG_INSTALL "" OFF)

    # Disable TLS and thread id on Wii U
    option(SPDLOG_NO_TLS "" ON)
    option(SPDLOG_NO_THREAD_ID "" ON)

    FetchContent_MakeAvailable(spdlog)
endif()

#======== thread-pool ========
# Wii U needs a patch to get rid of thread_local
FetchContent_Declare(
    ThreadPool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.1.0
    PATCH_COMMAND patch -p2 -i "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/patches/threadpool-wiiu.patch"
    # don't try to apply the patch multiple times https://stackoverflow.com/a/73725257
    UPDATE_DISCONNECTED 1
)
FetchContent_MakeAvailable(ThreadPool)
list(APPEND ADDITIONAL_LIB_INCLUDES ${threadpool_SOURCE_DIR}/include)
