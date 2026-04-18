#pragma once

/// @file
/// @brief Registry of editor dock panels assembled at startup.

#include <memory>
#include <vector>

#include "EditorUIExport.h"
#include "IEditorPanel.h"

namespace goleta
{

/// @brief Owns the collection of editor panels. EditorMainWindow walks this registry at
///        construction to materialise docks and the central widget.
class EDITORUI_API EditorPanelRegistry
{
public:
    EditorPanelRegistry() = default;
    ~EditorPanelRegistry() = default;

    EditorPanelRegistry(const EditorPanelRegistry&) = delete;
    EditorPanelRegistry& operator=(const EditorPanelRegistry&) = delete;

    /// @brief Take ownership of a panel. Must be called before constructing EditorMainWindow.
    void add(std::unique_ptr<IEditorPanel> Panel);

    /// @brief Panels in registration order.
    const std::vector<std::unique_ptr<IEditorPanel>>& all() const noexcept { return Panels; }

private:
    std::vector<std::unique_ptr<IEditorPanel>> Panels;
};

} // namespace goleta
