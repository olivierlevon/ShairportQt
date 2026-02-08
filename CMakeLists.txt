cmake_minimum_required(VERSION 3.20.0)
project(ShairportQt VERSION 1.0.0.5 LANGUAGES CXX C)

if (APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# locate packages
if (UNIX AND NOT APPLE)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets DBus)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets DBus)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
endif()

find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(sockpp CONFIG REQUIRED)

# Shairport Library
set(LIBRARY_SOURCES lib/dnssd.cpp lib/RaopServer.cpp lib/LayerCake.cpp lib/base64.cpp 
        lib/crypto.cpp lib/HairTunes.cpp lib/libutils.cpp lib/RaopEndpoint.cpp
        lib/audio/AlsaAudio.cpp lib/audio/AudioPlayer.cpp lib/SuspendInhibitor.cpp lib/Networking.cpp
        lib/DmapParser.cpp lib/KeyboardHook.cpp lib/MediaIntegration.cpp)

if (MSVC)
    list(APPEND LIBRARY_SOURCES lib/SmtcIntegration.cpp lib/audio/WasapiPlayer.cpp lib/audio/AsioPlayer.cpp)
endif()
if (UNIX AND NOT APPLE)
    list(APPEND LIBRARY_SOURCES lib/MprisIntegration.cpp)
endif()
if (APPLE)
    list(APPEND LIBRARY_SOURCES lib/NowPlayingIntegration.mm)
endif()

add_library(ShairLib STATIC ${LIBRARY_SOURCES})
if (MSVC)
    # we apply the static CRT for MSVC
    set_property(TARGET ShairLib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # dismiss several warnings
    target_compile_options(ShairLib PRIVATE -wd4996)
else()
    # dismiss several warnings
    target_compile_options(ShairLib PRIVATE -Wno-deprecated-declarations)
endif()
target_include_directories(ShairLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_include_directories(ShairLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/Bonjour")
target_link_libraries(ShairLib PRIVATE spdlog::spdlog Sockpp::sockpp-static)
if (MSVC)
    # cwASIO: clean-room ASIO SDK reimplementation (MIT license)
    include(FetchContent)
    FetchContent_Declare(cwASIO
        GIT_REPOSITORY https://github.com/s13n/cwASIO.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(cwASIO)
    target_link_libraries(ShairLib PRIVATE cwASIO::lib)
    target_link_libraries(ShairLib PRIVATE windowsapp.lib)
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(ShairLib PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
endif()
if (APPLE)
    target_link_libraries(ShairLib PRIVATE "-framework MediaPlayer" "-framework AppKit" "-framework Foundation")
endif()

# Shairport Application
set(LOCALIZATION_SOURCES    app/localization/LanguageManager.cpp 
                            app/localization/StringIDs.cpp
                            app/localization/English.cpp
                            app/localization/Japanese.cpp
                            app/localization/German.cpp
                            app/localization/Spanish.cpp
                            app/localization/Catalan.cpp
                            app/localization/French.cpp
                            app/localization/Italian.cpp)

set(APP_SOURCES app/main.cpp
                app/MainDlg.cpp
                app/DacpService.cpp
                app/TimeLabel.cpp
                ${LOCALIZATION_SOURCES})

set(APP_RESOURCE res/resource.qrc)
if (MSVC)
    # include MSVS's native resource file and application manifest
    set(APP_RESOURCE app/ShairportQt.rc app/ShairportQt.manifest ${APP_RESOURCE})
endif()

add_executable(ShairportQt ${APP_SOURCES} ${APP_RESOURCE})
target_compile_definitions(ShairportQt PRIVATE SHAIRPORTQT_VERSION="${PROJECT_VERSION}")

if (MSVC)
    # we apply the static CRT for MSVC
    set_property(TARGET ShairportQt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if (CMAKE_BUILD_TYPE MATCHES ".*Rel.*")
        # this magic setting makes our app native to Windows (just for Release builds)
        # otherwise we'll create a "console" app
        set_target_properties(ShairportQt PROPERTIES WIN32_EXECUTABLE ON)
    endif()
    # dismiss several warnings
    target_compile_options(ShairportQt PRIVATE -wd4996)
else()
    # dismiss several warnings
    target_compile_options(ShairportQt PRIVATE -Wno-deprecated-declarations -Wno-narrowing)
endif()

target_link_libraries(ShairportQt PRIVATE ShairLib)
if (UNIX AND NOT APPLE)
    target_link_libraries(ShairportQt PRIVATE asound)
endif()
target_link_libraries(ShairportQt PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(ShairportQt PRIVATE spdlog::spdlog)
target_link_libraries(ShairportQt PRIVATE Sockpp::sockpp-static)
if (UNIX AND NOT APPLE)
    target_link_libraries(ShairportQt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DBus)
else()
    target_link_libraries(ShairportQt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endif()

if (NOT(CMAKE_BUILD_TYPE MATCHES ".*MinSize.*") AND NOT(CMAKE_BUILD_TYPE MATCHES ".*RelWith.*"))
    # Unit testing
    find_package(GTest CONFIG REQUIRED)
    include(CTest)
    enable_testing()

    set(TEST_SOURCES    test/main.cpp 
                        test/StreamTest.cpp 
                        test/EndpointTest.cpp 
                        test/TrimTest.cpp
                        test/QueueTest.cpp
                        test/NetworkingTest.cpp)

    add_executable(ShairportQtTest ${TEST_SOURCES})

    if (MSVC)
        # we apply the dynamic CRT for MSVC
        set_property(TARGET ShairportQtTest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        # dismiss several warnings
        target_compile_options(ShairportQtTest PRIVATE -wd4996)
    else()
        # dismiss several warnings
        target_compile_options(ShairportQtTest PRIVATE -Wno-deprecated-declarations)
    endif()
    target_link_libraries(ShairportQtTest PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    target_link_libraries(ShairportQtTest PRIVATE ShairLib)
    target_link_libraries(ShairportQtTest PRIVATE spdlog::spdlog)
    target_link_libraries(ShairportQtTest PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(ShairportQtTest PRIVATE Sockpp::sockpp-static)

    add_test(AllTests ShairportQtTest)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ShairportQt)
endif()