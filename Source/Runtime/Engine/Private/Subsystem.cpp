/// @file
/// @brief Subsystem base-class definitions and the global factory registry.

#include "Subsystem.h"
#include "SubsystemRegistry.h"

namespace goleta {

void Subsystem::initialize(Engine& /*Owner*/) {}
void Subsystem::deinitialize() {}
void Subsystem::tick(float /*DeltaSeconds*/) {}
bool Subsystem::shouldTick() const { return false; }
TickStage Subsystem::tickStage() const { return TickStage::Update; }

namespace detail {

static Vec<SubsystemFactoryEntry>& mutableRegistry()
{
    static Vec<SubsystemFactoryEntry> Registry;
    return Registry;
}

void registerSubsystemFactory(SubsystemFactoryEntry Entry)
{
    mutableRegistry().push(std::move(Entry));
}

const Vec<SubsystemFactoryEntry>& subsystemRegistry()
{
    return mutableRegistry();
}

} // namespace detail
} // namespace goleta
