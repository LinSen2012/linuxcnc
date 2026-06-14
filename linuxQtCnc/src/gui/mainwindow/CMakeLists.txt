add_library(lqt_gui_mainwindow STATIC
    MainWindow.h MainWindow.cpp
)
target_include_directories(lqt_gui_mainwindow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(lqt_gui_mainwindow PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    lqt_gui_core
    lqt_gui_widgets
    lqt_gui_preview
)
