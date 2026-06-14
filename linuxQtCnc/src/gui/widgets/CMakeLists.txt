add_library(lqt_gui_widgets STATIC
    DROWidget.h DROWidget.cpp
    GCodeEditor.h GCodeEditor.cpp
    JogPanel.h JogPanel.cpp
    StatusBar.h StatusBar.cpp
    HalMonitorWidget.h HalMonitorWidget.cpp
    ScopeWidget.h ScopeWidget.cpp
    ToolTableWidget.h ToolTableWidget.cpp
    OffsetTableWidget.h OffsetTableWidget.cpp
)
target_include_directories(lqt_gui_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(lqt_gui_widgets PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    lqt_gui_core
)
