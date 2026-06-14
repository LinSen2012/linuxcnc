add_library(lqt_gui_preview STATIC
    ToolpathView.h ToolpathView.cpp
    GCodeParser.h GCodeParser.cpp
)
target_include_directories(lqt_gui_preview PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(lqt_gui_preview PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    lqt_gui_core
)
