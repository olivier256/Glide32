cmake_minimum_required(VERSION 3.16)

project(pixel C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(minifb)

find_package(Threads REQUIRED)

# ---- Simulateur 32X : modules communs à tous les jeux ----------------------

set(SIMULATOR_SOURCES vdp.c interface.c)

# ---- Démo d'origine --------------------------------------------------------

add_executable(pixel
	game.c
	${SIMULATOR_SOURCES}
	platform_minifb.c
)
target_compile_options(pixel PRIVATE -Wall -Wextra)
target_link_libraries(pixel PRIVATE minifb Threads::Threads)
set_target_properties(pixel PROPERTIES LINKER_LANGUAGE CXX)

# ---- Outil : BMP 8 bits -> données C ---------------------------------------

add_executable(bmp2c tools/bmp2c.c)
target_include_directories(bmp2c PRIVATE amb96)
target_compile_options(bmp2c PRIVATE -Wall -Wextra)

# ---- AMB96 -----------------------------------------------------------------
# Les images sont converties à chaque modification du manifeste ou d'un BMP.

set(AMB96_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/amb96/assets/assets.txt)
file(GLOB AMB96_BMP CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/amb96/assets/*.bmp)
set(AMB96_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/generated/amb96_assets)

add_custom_command(
	OUTPUT ${AMB96_GENERATED}.c ${AMB96_GENERATED}.h
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/generated
	COMMAND bmp2c ${AMB96_MANIFEST} ${AMB96_GENERATED}
	DEPENDS bmp2c ${AMB96_MANIFEST} ${AMB96_BMP}
	COMMENT "Conversion des images AMB96"
)

set(AMB96_SOURCES
	amb96/amb96.c
	amb96/md68k.c
	amb96/framebuffer.c
	amb96/plane.c
	amb96/sprite.c
	${AMB96_GENERATED}.c
	${SIMULATOR_SOURCES}
)

# Avec fenêtre
add_executable(amb96 ${AMB96_SOURCES} platform_minifb.c)
target_include_directories(amb96 PRIVATE . amb96 ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_compile_options(amb96 PRIVATE -Wall -Wextra)
target_link_libraries(amb96 PRIVATE minifb Threads::Threads)
set_target_properties(amb96 PROPERTIES LINKER_LANGUAGE CXX)

# Sans fenêtre : tests et captures (voir platform_headless.c)
add_executable(amb96_headless ${AMB96_SOURCES} platform_headless.c)
target_include_directories(amb96_headless PRIVATE . amb96 ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_compile_options(amb96_headless PRIVATE -Wall -Wextra)
target_link_libraries(amb96_headless PRIVATE Threads::Threads)
