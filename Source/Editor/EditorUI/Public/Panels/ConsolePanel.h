#pragma once

/// @file
/// @brief ConsolePanel: read-only log output dock.

#include "EditorUIExport.h"
#include "IEditorPanel.h"

namespace goleta
{

/// @brief Bottom-dock log panel. Content currently placeholder; will hook the engine log sink.
class EDITORUI_API ConsolePanel : public IEditorPanel
{
public:
    ConsolePanel() = default;
    ~ConsolePanel() override = default;

    QString panelId() const override;
    QString displayName() const override;
    Qt::DockWidgetArea defaultArea() const override { return Qt::BottomDockWidgetArea; }
    QWidget* createWidget(QWidget* Parent) override;
};

} // namespace goleta
