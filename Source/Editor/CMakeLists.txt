find_package(Qt6 QUIET COMPONENTS Core Gui Widgets)

add_subdirectory(EditorCore)

if(Qt6_FOUND)
    qt_standard_project_setup()
    add_subdirectory(EditorUI)
    add_subdirectory(EditorApp)
else()
    message(STATUS
        "Qt6 not found; EditorUI and EditorApp targets will not be built. "
        "Point CMAKE_PREFIX_PATH at your Qt install (e.g. D:/Qt/6.11.0/msvc2022_64) to enable them."
    )
endif()
