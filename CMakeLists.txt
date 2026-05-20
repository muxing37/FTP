cmake_minimum_required(VERSION 3.10)

project(FTPProject)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(ftp
    src/main.cpp
    src/socket.cpp
    src/ftpserver.cpp
    src/ftpclient.cpp
    src/ftpctrl.cpp
    src/ftpls.cpp
)

# target_link_libraries(ftp pthread)

target_link_libraries(ftp pthread readline history)