set(TARGET imgui)

set(CMAKE_CXX_STANDARD 14)
include(DobieHelpers)

set(SOURCE
    imconfig.h
    imgui_internal.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    imgui.h

    imgui_widgets.cpp
    imgui_draw.cpp
    imgui.cpp)

add_library(${TARGET} STATIC ${SOURCE})
add_library(Ext::imgui ALIAS ${TARGET})

target_include_directories(${TARGET}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
dobie_cxx_compile_options(${TARGET})
set_property(TARGET ${TARGET} PROPERTY FOLDER External)
