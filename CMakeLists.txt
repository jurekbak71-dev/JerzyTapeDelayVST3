cmake_minimum_required(VERSION 3.25)
project(JerzyAudioPlugins VERSION 0.2.0.0 DESCRIPTION "Jerzy Audio VST3 Plugins")

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
    resource/tapedelay.uidesc
)
target_link_libraries(JerzyTapeDelay PRIVATE sdk vstgui_support)
target_compile_features(JerzyTapeDelay PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyTapeDelay)
smtg_target_add_plugin_resources(JerzyTapeDelay
    RESOURCES
        resource/tapedelay.uidesc
        resource/tapedelay_background.jpg
)

smtg_add_vst3plugin(JerzyTapeDrive
    source/version.h
    source/tapedrive_ids.h
    source/tapedrive_params.h
    source/tapedrive_state.h
    source/tapedrive_dsp.h
    source/tapedrive_processor.h
    source/tapedrive_processor.cpp
    source/tapedrive_controller.h
    source/tapedrive_controller.cpp
    source/tapedrive_editor.h
    source/tapedrive_editor.cpp
    source/tapedrive_gui_views.h
    source/tapedrive_gui_views.cpp
    source/tapedrive_entry.cpp
    resource/tapedrive_vintage.uidesc
)
target_link_libraries(JerzyTapeDrive PRIVATE sdk vstgui_support)
target_compile_features(JerzyTapeDrive PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyTapeDrive)
smtg_target_add_plugin_resources(JerzyTapeDrive
    RESOURCES
        resource/tapedrive_vintage.uidesc
        resource/tapedrive_vintage_background.jpg
)

if(SMTG_MAC)
    smtg_target_set_bundle(JerzyTapeDelay BUNDLE_IDENTIFIER "pl.jerzyaudio.tapedelay" COMPANY_NAME "Jerzy Audio")
    smtg_target_set_bundle(JerzyTapeDrive BUNDLE_IDENTIFIER "pl.jerzyaudio.tapedrive" COMPANY_NAME "Jerzy Audio")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(TapeDriveDSPTests tests/tapedrive_test.cpp)
    add_executable(TapeDelayDSPTests tests/dsp_smoke_test.cpp)
    add_test(NAME TapeDriveDSP COMMAND TapeDriveDSPTests)
    add_test(NAME TapeDelayDSP COMMAND TapeDelayDSPTests)
endif()
