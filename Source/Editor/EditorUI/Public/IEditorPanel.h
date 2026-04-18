#pragma once

/// @file
/// @brief Interface for dockable editor panels.

#include <cstdint>

#include <QString>
#include <QtCore/qnamespace.h>

#include "EditorUIExport.h"

class QWidget;

namespace goleta
{

/// @brief How a panel is placed inside EditorMainWindow.
enum class PanelRole : uint8_t
{
    Central,
    Dock,
};

/// @brief Abstract editor pane. Subclass to add a new dock or central panel.
/// @note  Panel objects are owned by EditorPanelRegistry. createWidget() is called once
///        by EditorMainWindow at construction; the returned QWidget is owned by Qt's
///        parent-child hierarchy (the enclosing QDockWidget or the main window itself).
class EDITORUI_API IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    IEditorPanel(const IEditorPanel&) = delete;
    IEditorPanel& operator=(const IEditorPanel&) = delete;

    /// @brief Stable identifier used for layout save/restore and menu wiring. Must be unique.
    virtual QString panelId() const = 0;

    /// @brief User-visible title shown on the dock tab or window.
    virtual QString displayName() const = 0;

    /// @brief Whether this panel is a central widget or a dock. Defaults to Dock.
    virtual PanelRole role() const { return PanelRole::Dock; }

    /// @brief Preferred initial dock area. Ignored when role() is Central.
    virtual Qt::DockWidgetArea defaultArea() const { return Qt::RightDockWidgetArea; }

    /// @brief Build and return the panel's widget, parented to Parent.
    virtual QWidget* createWidget(QWidget* Parent) = 0;

protected:
    IEditorPanel() = default;
};

} // namespace goleta
