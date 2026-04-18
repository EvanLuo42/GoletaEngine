#pragma once

/// @file
/// @brief EditorEngine -- Engine specialization instantiated by the editor executable.

#include "EditorCoreExport.h"
#include "EngineCore.h"

namespace goleta
{

/// @brief Engine flavour used by the editor. Accepts Engine, Game, and Editor subsystem categories.
class EDITORCORE_API EditorEngine : public Engine
{
public:
    EditorEngine();
    ~EditorEngine() override;

protected:
    bool acceptsCategory(SubsystemCategory Category) const override;
};

} // namespace goleta
