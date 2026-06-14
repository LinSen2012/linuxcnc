add_library(lqt_gui_core STATIC
    LcncCore.h LcncCore.cpp
    LcncStatusData.h
    LcncCommand.h LcncCommand.cpp
    LcncHalMonitor.h LcncHalMonitor.cpp
    LcncToolTable.h LcncToolTable.cpp
)
target_include_directories(lqt_gui_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CORE_INCLUDE_DIRS}
)
target_link_libraries(lqt_gui_core PUBLIC
    Qt5::Core
    Qt5::Gui
)
