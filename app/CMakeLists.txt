# app/CMakeLists.txt
#
# Builds the client_sim Qt6 desktop application. This file is only
# processed by the top-level CMakeLists.txt when Qt6 with Widgets is
# available.

qt_standard_project_setup()

qt_add_executable(client_sim
    main.cpp

    MainWindow.h          MainWindow.cpp
    WzTreeModel.h         WzTreeModel.cpp
    WzTreeView.h          WzTreeView.cpp
    ImageViewer.h         ImageViewer.cpp
    RgbaToPixmap.h        RgbaToPixmap.cpp
)

target_link_libraries(client_sim
    PRIVATE
        wzlib
        Qt6::Core
        Qt6::Widgets
)

set_target_properties(client_sim PROPERTIES
    WIN32 TRUE
    MACOSX_BUNDLE TRUE
)
