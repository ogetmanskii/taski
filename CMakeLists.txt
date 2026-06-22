cmake_minimum_required(VERSION 3.20)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(devkit LANGUAGES CXX)

#add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Чтение версии из файла
file(READ "${CMAKE_SOURCE_DIR}/version/VERSION" VERSION_FILE_CONTENT)
string(STRIP "${VERSION_FILE_CONTENT}" VERSION_FROM_FILE)

# Парсинг версии
string(REPLACE "." ";" VERSION_LIST ${VERSION_FROM_FILE})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)
list(GET VERSION_LIST 3 VERSION_TWEAK)

# Установка переменных проекта
set(PROJECT_VERSION_MAJOR ${VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${VERSION_PATCH})
set(PROJECT_VERSION_TWEAK ${VERSION_TWEAK})
set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PROJECT_VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}")

# Настройка заголовочного файла версии
configure_file(
    "${CMAKE_SOURCE_DIR}/version/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
)

# Ресурсный файл для Windows
if(WIN32)
    configure_file(
        "${CMAKE_SOURCE_DIR}/version/version.rc.in"
        "${CMAKE_BINARY_DIR}/generated/version.rc"
    )
endif()

find_package(inja REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

add_executable(
    cli 
    "src/main.cpp"
    "src/menu/menu.cpp"
    "src/processes/processes.cpp"
    "src/logging/console_logging.cpp"
    "src/util/util.cpp" "src/parsing/parsing.cpp"  "src/util/win_create_process.cpp"    "src/util/wstring.cpp" "src/parsing/env_parser.cpp"   "src/args/args.cpp" "src/context/pipeline_context.cpp" "src/action/action.cpp")
target_compile_features(cli PRIVATE cxx_std_20)
target_link_libraries(cli
    pantor::inja
    yaml-cpp::yaml-cpp
)

# Добавление ресурсов для Windows
if(WIN32)
    target_sources(cli PRIVATE 
        ${CMAKE_BINARY_DIR}/generated/version.rc
    )
endif()

# Установка свойства VERSION для цели
set_target_properties(cli PROPERTIES
    VERSION ${PROJECT_VERSION_FULL}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_executable(
    testrunner 
    "tests/processes_test.cpp"
    "src/menu/menu.cpp"
    "src/processes/processes.cpp"
    "src/logging/console_logging.cpp"
    
 "src/util/util.cpp" "src/parsing/parsing.cpp"  "src/util/win_create_process.cpp"    "tests/strings_test.cpp" "src/util/wstring.cpp" "src/parsing/env_parser.cpp"    "src/args/args.cpp" "src/context/pipeline_context.cpp" "src/action/action.cpp")
target_compile_features(testrunner PRIVATE cxx_std_20)
target_link_libraries(testrunner PRIVATE
    GTest::gtest
    GTest::gtest_main
    pantor::inja
    yaml-cpp::yaml-cpp
)
add_executable("test-executable" "test_executable/main.cpp")
target_compile_features("test-executable" PRIVATE cxx_std_20)

# Создание архива после сборки
add_custom_command(
    TARGET cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/ReleaseZip
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:cli>/examples

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/ReleaseZip
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cli>/examples
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_SOURCE_DIR}/examples 
        $<TARGET_FILE_DIR:cli>/examples
    COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_SOURCE_DIR}/readme.md
        $<TARGET_FILE_DIR:cli>/readme.md
    COMMAND ${CMAKE_COMMAND} -E tar "cf" 
        ${CMAKE_CURRENT_BINARY_DIR}/ReleaseZip/cli-${PROJECT_VERSION_FULL}.zip 
        --format=zip 
        -- 
        cli${CMAKE_EXECUTABLE_SUFFIX}
        examples
        readme.md
    WORKING_DIRECTORY $<TARGET_FILE_DIR:cli>
    COMMENT "Creating release archive cli-${PROJECT_VERSION_FULL}.zip"
)