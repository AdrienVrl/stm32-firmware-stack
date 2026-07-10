# bsp/CMakeLists.txt

# CONFIGURE_DEPENDS re-globs on every build so newly added driver files are
# picked up without touching this file. No sources exist yet, so start as
# INTERFACE (include path only) and add compiled sources once they land.
file(GLOB BSP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(bsp INTERFACE)
target_include_directories(bsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BSP_SOURCES)
  target_sources(bsp INTERFACE ${BSP_SOURCES})
endif()
