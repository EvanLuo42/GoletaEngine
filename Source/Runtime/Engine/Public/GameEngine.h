#pragma once

/// @file
/// @brief GameEngine -- Engine specialization instantiated by standalone game executables.

#include "EngineCore.h"

namespace goleta {

/// @brief Engine flavour used by shipped game executables. Accepts Engine and Game subsystem categories.
class ENGINE_API GameEngine : public Engine
{
public:
    GameEngine();
    ~GameEngine() override;
};

} // namespace goleta
