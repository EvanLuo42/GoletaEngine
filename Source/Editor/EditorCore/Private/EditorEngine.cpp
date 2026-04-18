/// @file
/// @brief EditorEngine implementation.

#include "EditorEngine.h"

namespace goleta
{

EditorEngine::EditorEngine() = default;
EditorEngine::~EditorEngine() = default;

bool EditorEngine::acceptsCategory(const SubsystemCategory Category) const
{
    return Category == SubsystemCategory::Engine
        || Category == SubsystemCategory::Game
        || Category == SubsystemCategory::Editor;
}

} // namespace goleta
