set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(rime)
cmake_minimum_required(VERSION 3.0.2)

set(rime_version 1.5.3)
set(rime_soversion 1)

add_definitions(-DRIME_VERSION="${rime_version}")

include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install CACHE PATH "安装根目录" FORCE)
set(CMAKE_INSTALL_BINDIR ${CMAKE_CURRENT_SOURCE_DIR}/install/bin CACHE PATH "可执行文件目录" FORCE)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/install/lib CACHE PATH "库文件目录" FORCE)
set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/install/include CACHE PATH "库文件目录" FORCE)
# set(rime_data_dir "/share/rime" CACHE STRING "Rime 数据目录" FORCE)

option(BUILD_SHARED_LIBS "构建为动态库" ON)
option(BUILD_MERGED_PLUGINS "Merge plugins into one Rime library" ON)
option(BUILD_STATIC "Build with dependencies as static libraries" OFF)
option(BUILD_DATA "Build data for Rime" OFF)
option(BUILD_SAMPLE "Build sample Rime plugin" OFF)
option(BUILD_TEST "构建运行测试" OFF)
option(BUILD_SEPARATE_LIBS "Build separate rime-* libraries" OFF)
option(ENABLE_LOGGING "启用google-glog日志记录" ON)
option(BOOST_USE_CXX11 "Boost has been built with C++11 support" OFF)
option(BOOST_USE_SIGNALS2 "Boost use signals2 instead of signals" ON)
option(ENABLE_ASAN "Enable Address Sanitizer (Unix Only)" OFF)

set(rime_data_dir "/share/rime-data" CACHE STRING "Target directory for Rime data")

if(WIN32)
  set(ext ".exe")
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${PROJECT_SOURCE_DIR}/thirdparty")

if (ENABLE_ASAN)
  set(asan_cflags "-fsanitize=address -fno-omit-frame-pointer")
  set(asan_lflags "-fsanitize=address -lasan")
  set(CMAKE_C_FLAGS "${asan_cflags} ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${asan_cflags} ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${asan_lflags} ${CMAKE_EXE_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${asan_lflags} ${CMAKE_SHARED_LINKER_FLAGS}")
endif()

set(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(Gflags_STATIC ${BUILD_STATIC})
set(Glog_STATIC ${BUILD_STATIC})
set(LevelDb_STATIC ${BUILD_STATIC})
set(Marisa_STATIC ${BUILD_STATIC})
set(Opencc_STATIC ${BUILD_STATIC})
set(YamlCpp_STATIC ${BUILD_STATIC})

set(Boost_USE_MULTITHREADED ON)
if(MSVC)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

if(NOT BOOST_USE_CXX11)
  add_definitions("-DBOOST_NO_CXX11_SCOPED_ENUMS")
endif()

set(BOOST_COMPONENTS filesystem regex system)

if(BOOST_USE_SIGNALS2)
  add_definitions("-DBOOST_SIGNALS2")
else()
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} signals)
endif()

find_package(Boost 1.46.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

if(ENABLE_LOGGING)

  find_package(Gflags)
  if(Gflags_FOUND)
    include_directories(${Gflags_INCLUDE_PATH})
  endif()

  find_package(Glog REQUIRED)
  if(Glog_FOUND)
    include_directories(${Glog_INCLUDE_PATH})
  endif()
  if(Glog_STATIC)
    add_definitions(-DGOOGLE_GLOG_DLL_DECL=)
  endif()

  add_definitions(-DRIME_ENABLE_LOGGING)

endif()

find_package(Threads)

if(BUILD_TEST)
  find_package(GTest REQUIRED)
  if(GTEST_FOUND)
    enable_testing()
    include_directories(${GTEST_INCLUDE_DIRS})
  else()
    message(WARNING "GTest 未找到, 测试不可用")
  endif()
endif()

find_package(YamlCpp REQUIRED)
if(YamlCpp_FOUND)
  include_directories(${YamlCpp_INCLUDE_PATH})
endif()

find_package(LevelDb REQUIRED)
if(LevelDb_FOUND)
    include_directories(${LevelDb_INCLUDE_PATH})
endif()

find_package(Marisa REQUIRED)
if(Marisa_FOUND)
  include_directories(${Marisa_INCLUDE_PATH})
endif()

find_package(Opencc REQUIRED)
if(Opencc_FOUND)
include_directories(${Opencc_INCLUDE_PATH})
endif()
if(Opencc_STATIC)
  add_definitions(-DOpencc_BUILT_AS_STATIC)
endif()

find_path(X11Keysym X11/keysym.h)
if(X11Keysym)
  message(STATUS "Found X11/keysym.h at ${X11Keysym}")
  include_directories(${X11Keysym})
else()
  message(WARNING "X11/keysym.h not found.")
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include)
link_directories(${PROJECT_SOURCE_DIR}/thirdparty/lib)

if(MSVC)
  # https://stackoverflow.com/a/31264946
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  if(MSVC10)
    # stdint.h is missing from VS2010
    include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include/msvc)
  endif()
  # Force Boost.Uuid to use wincrypt as random provider instead of bcrypt for Windows XP compatibility
  add_definitions("/wd4244 /wd4996 /DBOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT")
endif()

if(UNIX)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
endif()

# uninstall target
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|DragonFly|GNU")
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(bindir "${exec_prefix}/${BIN_INSTALL_DIR}")
  set(libdir "${exec_prefix}/${LIB_INSTALL_DIR}")
  set(pkgdatadir "${prefix}${rime_data_dir}")
  set(includedir "${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
  configure_file(
      ${PROJECT_SOURCE_DIR}/rime.pc.in
      ${PROJECT_BINARY_DIR}/rime.pc
      @ONLY)
  install(FILES ${PROJECT_BINARY_DIR}/rime.pc
    DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif()

install(FILES cmake/RimeConfig.cmake DESTINATION share/cmake/rime)

file(GLOB rime_public_header_files ${PROJECT_SOURCE_DIR}/src/*.h)
install(FILES ${rime_public_header_files} DESTINATION include)

if(BUILD_DATA)
  file(GLOB rime_preset_data_files ${PROJECT_SOURCE_DIR}/data/preset/*.yaml)
  install(FILES ${rime_preset_data_files}
    DESTINATION ${CMAKE_INSTALL_PREFIX}${rime_data_dir})
endif()

if(BUILD_SHARED_LIBS)
  add_definitions(-DRIME_BUILD_SHARED_LIBS)
  set(rime_library rime)
  if(BUILD_SEPARATE_LIBS)
    set(rime_dict_library rime-dict)
    set(rime_gears_library rime-gears)
    set(rime_levers_library rime-levers)
  endif()
else()
  set(rime_library rime-static)
endif()

add_subdirectory(plugins)
message(STATUS "rime_plugins_libs: ${rime_plugins_deps}")
message(STATUS "rime_plugins_modules: ${rime_plugins_modules}")
set(list "")
foreach(mod ${rime_plugins_modules})
  set(list "${list},Q(${mod})")
endforeach()
add_definitions(-DRIME_EXTRA_MODULES=${list})
if(BUILD_SHARED_LIBS AND BUILD_SEPARATE_LIBS AND rime_plugins_objs)
  set(rime_plugins_library rime-plugins)
endif()

add_subdirectory(src)

if(BUILD_SHARED_LIBS)
  add_subdirectory(tools)

  if(BUILD_TEST)
    add_subdirectory(test)
  endif()

  if (BUILD_SAMPLE)
    add_subdirectory(sample)
  endif()
endif()
