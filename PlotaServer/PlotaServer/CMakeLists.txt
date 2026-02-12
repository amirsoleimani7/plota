cmake_minimum_required(VERSION 3.16)

project(PlotaServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

add_executable(PlotaServer
  main.cpp
  Validators.h
  Validators.cpp
  PasswordHasher.h
  PasswordHasher.cpp
  Database.h
  Database.cpp
  SessionManager.h
  SessionManager.cpp
  AuthService.h
  AuthService.cpp
  OthelloLogic.h
  OthelloLogic.cpp
  RoomManager.h
  RoomManager.cpp
  OthelloRoomService.h
  OthelloRoomService.cpp
  OthelloService.h
  OthelloService.cpp
)

target_link_libraries(PlotaServer PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
)

include(GNUInstallDirs)
install(TARGETS PlotaServer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
