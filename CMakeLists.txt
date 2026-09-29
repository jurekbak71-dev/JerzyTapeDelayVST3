cmake_minimum_required(VERSION 3.25)
project(JerzyTapeDelay VERSION 0.1.0.0 DESCRIPTION "Tape Delay VST3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VST3_SDK_ROOT "" CACHE PATH "Path to a recursively cloned Steinberg vst3sdk")

if(VST3_SDK_ROOT AND EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
else()
    include(FetchContent)
    message(STATUS "VST3_SDK_ROOT not supplied; fetching official Steinberg VST3 SDK")
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_SUBMODULES_RECURSE TRUE
    )
    FetchContent_MakeAvailable(vst3sdk)
endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(JerzyTapeDelay
    source/version.h
    source/tapedelay_ids.h
    source/tapedelay_params.h
    source/tapedelay_dsp.h
    source/tapedelay_processor.h
    source/tapedelay_processor.cpp
    source/tapedelay_controller.h
    source/tapedelay_controller.cpp
    source/tapedelay_entry.cpp
)

target_link_libraries(JerzyTapeDelay PRIVATE sdk)
target_compile_features(JerzyTapeDelay PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyTapeDelay)

if(SMTG_MAC)
    smtg_target_set_bundle(JerzyTapeDelay
        BUNDLE_IDENTIFIER "pl.jerzyaudio.tapedelay"
        COMPANY_NAME "Jerzy Audio"
    )
endif()
