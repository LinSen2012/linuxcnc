add_library(lqt_gui_halmon STATIC
    HalMonitor.h HalMonitor.cpp
    HalTreeModel.h HalTreeModel.cpp
)
target_include_directories(lqt_gui_halmon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(lqt_gui_halmon PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    lqt_gui_core
)
