set(_target EditorApp)

qt_add_executable(${_target}
    Main.cpp
)

target_link_libraries(${_target} PRIVATE
    EditorCore
    EditorUI
    Engine
    Core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_compile_features(${_target} PRIVATE cxx_std_20)
set_target_properties(${_target} PROPERTIES
    FOLDER "Editor"
    WIN32_EXECUTABLE ON
)

if(WIN32)
    add_custom_command(TARGET ${_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${_target}>
                $<TARGET_FILE_DIR:${_target}>
        COMMAND_EXPAND_LISTS
    )

    get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(GOLETA_WINDEPLOYQT_EXE NAMES windeployqt HINTS "${_qt_bin_dir}")
    if(GOLETA_WINDEPLOYQT_EXE)
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND "${GOLETA_WINDEPLOYQT_EXE}"
                    --no-translations
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    $<IF:$<CONFIG:Debug>,--debug,--release>
                    "$<TARGET_FILE:${_target}>"
            COMMENT "Deploying Qt runtime for ${_target}"
        )
    endif()
endif()

install(TARGETS ${_target} RUNTIME DESTINATION bin)
