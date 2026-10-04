cmake_minimum_required(VERSION 3.25)
project(JerzyAudioPlugins VERSION 0.4.1.0 DESCRIPTION "Jerzy Audio VST3 Plugins")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VST3_SDK_ROOT "" CACHE PATH "Path to a recursively cloned Steinberg vst3sdk")

if(VST3_SDK_ROOT AND EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    set(JERZY_VST3_SDK_DIR "${VST3_SDK_ROOT}")
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
    set(JERZY_VST3_SDK_DIR "${vst3sdk_SOURCE_DIR}")
endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(JerzyTapeDrive
    source/version.h source/tapedrive_ids.h source/tapedrive_params.h source/tapedrive_state.h
    source/tapedrive_dsp.h source/tapedrive_processor.h source/tapedrive_processor.cpp
    source/tapedrive_controller.h source/tapedrive_controller.cpp source/tapedrive_editor_geometry.h
    source/tapedrive_editor.h source/tapedrive_editor.cpp source/tapedrive_gui_views.h
    source/tapedrive_gui_views.cpp source/tapedrive_entry.cpp resource/tapedrive_vintage.uidesc
)
target_link_libraries(JerzyTapeDrive PRIVATE sdk vstgui_support)
if(WIN32)
    target_link_libraries(JerzyTapeDrive PRIVATE comctl32)
endif()
target_compile_features(JerzyTapeDrive PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyTapeDrive)
smtg_target_add_plugin_resources(JerzyTapeDrive RESOURCES resource/tapedrive_vintage.uidesc resource/tapedrive_vintage_background.jpg)
if(SMTG_MAC)
    smtg_target_set_bundle(JerzyTapeDrive BUNDLE_IDENTIFIER "pl.jerzyaudio.tapedrive" COMPANY_NAME "Jerzy Audio")
endif()

smtg_add_vst3plugin(JerzyMXAnalogDelay
    source/mxdelay_version.h source/mxdelay_ids.h source/mxdelay_params.h source/mxdelay_state.h
    source/mxdelay_dsp.h source/mxdelay_processor.h source/mxdelay_processor.cpp
    source/mxdelay_editor_geometry.h source/mxdelay_editor.h source/mxdelay_editor.cpp
    source/mxdelay_controller.h source/mxdelay_controller.cpp
    source/tapedrive_gui_views.h source/tapedrive_gui_views.cpp
    source/mxdelay_entry.cpp resource/mxdelay.uidesc
)
target_link_libraries(JerzyMXAnalogDelay PRIVATE sdk vstgui_support)
if(WIN32)
    target_link_libraries(JerzyMXAnalogDelay PRIVATE comctl32)
endif()
target_compile_features(JerzyMXAnalogDelay PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyMXAnalogDelay)
smtg_target_add_plugin_resources(JerzyMXAnalogDelay RESOURCES resource/mxdelay.uidesc)
if(SMTG_MAC)
    smtg_target_set_bundle(JerzyMXAnalogDelay BUNDLE_IDENTIFIER "pl.jerzyaudio.mxanalogdelay" COMPANY_NAME "Jerzy Audio")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(TapeDriveDSPTests tests/tapedrive_test.cpp)
    add_test(NAME TapeDriveDSP COMMAND TapeDriveDSPTests)

    add_executable(MXDelayDSPTests tests/mxdelay_test.cpp)
    target_include_directories(MXDelayDSPTests PRIVATE source)
    add_test(NAME MXDelayDSP COMMAND MXDelayDSPTests)
endif()

if(BUILD_TESTING AND WIN32)
    target_sources(JerzyTapeDrive PRIVATE tests/tapedrive_render_probe.cpp)
    target_sources(JerzyMXAnalogDelay PRIVATE tests/mxdelay_render_probe.cpp)
    add_executable(TapeDriveGUITests
        tests/tapedrive_gui_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
    )
    target_compile_definitions(TapeDriveGUITests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(TapeDriveGUITests PRIVATE sdk_hosting user32 ole32 shell32)
    add_dependencies(TapeDriveGUITests JerzyTapeDrive)
    add_test(NAME TapeDriveGUI COMMAND TapeDriveGUITests $<TARGET_FILE:JerzyTapeDrive>)
    set_tests_properties(TapeDriveGUI PROPERTIES TIMEOUT 90)

    add_executable(MXDelayGUITests
        tests/mxdelay_gui_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
    )
    target_compile_definitions(MXDelayGUITests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(MXDelayGUITests PRIVATE sdk_hosting user32 ole32 shell32)
    add_dependencies(MXDelayGUITests JerzyMXAnalogDelay)
    add_test(NAME MXDelayGUI COMMAND MXDelayGUITests $<TARGET_FILE:JerzyMXAnalogDelay>)
    set_tests_properties(MXDelayGUI PROPERTIES TIMEOUT 90)
endif()
