goleta_add_module(EditorUI
    DEPENDENCIES EditorCore
)

set_target_properties(EditorUI PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(EditorUI PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
