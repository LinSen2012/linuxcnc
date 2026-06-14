# 示波器模块
add_library(lqt_gui_scope STATIC
    ScopeView.h ScopeView.cpp
    ScopeChannel.h ScopeChannel.cpp
)
target_include_directories(lqt_gui_scope PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)
target_link_libraries(lqt_gui_scope PUBLIC
    Qt5::Core Qt5::Gui Qt5::Widgets
    lqt_gui_core
)
