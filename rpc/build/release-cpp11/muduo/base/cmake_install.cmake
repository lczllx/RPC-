# Install script for directory: /home/lcz/practice/json-rpc/muduo/muduo/base

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/lcz/practice/json-rpc/build/release-install-cpp11")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/lcz/practice/json-rpc/build/release-cpp11/lib/libmuduo_base.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/base" TYPE FILE FILES
    "/home/lcz/practice/json-rpc/muduo/muduo/base/AsyncLogging.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Atomic.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/BlockingQueue.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/BoundedBlockingQueue.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Condition.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/CountDownLatch.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/CurrentThread.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Date.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Exception.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/FileUtil.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/GzipFile.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/LogFile.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/LogStream.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Logging.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Mutex.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/ProcessInfo.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Singleton.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/StringPiece.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Thread.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/ThreadLocal.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/ThreadLocalSingleton.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/ThreadPool.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/TimeZone.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Timestamp.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/Types.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/WeakCallback.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/copyable.h"
    "/home/lcz/practice/json-rpc/muduo/muduo/base/noncopyable.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/lcz/practice/json-rpc/build/release-cpp11/muduo/base/tests/cmake_install.cmake")

endif()

