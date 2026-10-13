# ---------------------------------------------------------------------------
# sokol — header-only GPU/app/audio/time layer.
# Pulled from master. If you need to pin, replace GIT_TAG with a specific
# commit hash from `git -C build/_deps/sokol-src log`.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(sokol)

set(SOKOL_INCLUDE_DIR ${sokol_SOURCE_DIR} CACHE INTERNAL "")