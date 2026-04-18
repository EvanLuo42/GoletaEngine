/// @file
/// @brief EditorPanelRegistry implementation.

#include "EditorPanelRegistry.h"

#include <cassert>
#include <utility>

namespace goleta
{

void EditorPanelRegistry::add(std::unique_ptr<IEditorPanel> Panel)
{
    assert(Panel && "EditorPanelRegistry::add: null panel");
    Panels.push_back(std::move(Panel));
}

} // namespace goleta
