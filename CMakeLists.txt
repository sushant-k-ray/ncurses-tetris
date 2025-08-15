cmake_minimum_required(VERSION 3.16)

project(tetris LANGUAGES C)

add_executable(tetris main.c)
target_link_libraries(tetris ncurses)

include(GNUInstallDirs)
install(TARGETS tetris
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
