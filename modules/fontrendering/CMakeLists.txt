add_subdirectory(ext/freetype)

#--------------------------------------------------------------------
# Inviwo FontRendering Module
ivw_module(FontRendering)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/datastructures/texatlasentry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fontrenderingmodule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fontrenderingmoduledefine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/textoverlaygl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/properties/fontproperty.h
    ${CMAKE_CURRENT_SOURCE_DIR}/textrenderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/fontutils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/textureatlas.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/datastructures/texatlasentry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fontrenderingmodule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/textoverlaygl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/properties/fontproperty.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/textrenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/fontutils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/textureatlas.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Add shaders
set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl/fontrendering_freetype.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl/fontrendering_freetype.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl/textrenderer.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl/textrenderer.vert
)
ivw_group("Shader Files" ${SHADER_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Package or build shaders into resources
ivw_handle_shader_resources(${CMAKE_CURRENT_SOURCE_DIR}/glsl ${SHADER_FILES})

#--------------------------------------------------------------------
# Add font directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/fonts)

#--------------------------------------------------------------------
# register license files
ivw_register_license_file(NAME FreeType MODULE FontRendering
    URL "https://www.freetype.org/"
    TYPE "BSD-style, acknowledgement: Portions of this software are copyright The FreeType Project (www.freetype.org). All rights reserved."
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/FTL.txt
)

#--------------------------------------------------------------------
# Add dependency to ext/utf 
target_link_libraries(inviwo-module-fontrendering PRIVATE utf)
target_link_libraries(inviwo-module-fontrendering PUBLIC freetype)
ivw_make_package(InviwoFontRenderingModule inviwo-module-fontrendering)



