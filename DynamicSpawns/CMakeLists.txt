# Mimimum CMake version
cmake_minimum_required(VERSION 3.24.0)

# =========================
# Project Configuration
# =========================
project(
    # Must match folder name
    DynamicSpawns
    DESCRIPTION "DynamicSpawns"
    VERSION     1.0
    LANGUAGES   CXX
)

# =========================
# Metadata / Licensing
# =========================
set(PROJECT_AUTHOR  "Mithras666")
set(PROJECT_YEAR    "(C) 2025")
set(PROJECT_LICENSE "MIT")

# =========================
# CMake Module Path
# =========================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# =========================
# Project Modules
# =========================
include(utility_config)
include(project_config)

# =========================
# CommonLibSSE Variant
# =========================
set(COMMONLIB_VARIANT "vs2022-ng")
option(SKYRIM_SUPPORT_AE "Enables support for Skyrim AE" ON)
option(INSTALL_IMGUI "Installs imgui framework" OFF)

# =========================
# Submodules / Dependencies
# =========================
include(submodule_config)
