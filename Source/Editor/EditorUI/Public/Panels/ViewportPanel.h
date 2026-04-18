#pragma once

/// @file
/// @brief ViewportPanel: central viewport placeholder that will later host the RHI surface.

#include "EditorUIExport.h"
#include "IEditorPanel.h"

namespace goleta
{

/// @brief Central-area viewport. Currently renders a placeholder label.
/// @note  When the RHI is wired in, createWidget() will return a QWidget embedding the swapchain.
class EDITORUI_API ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel() = default;
    ~ViewportPanel() override = default;

    QString panelId() const override;
    QString displayName() const override;
    PanelRole role() const override { return PanelRole::Central; }
    QWidget* createWidget(QWidget* Parent) override;
};

} // namespace goleta
