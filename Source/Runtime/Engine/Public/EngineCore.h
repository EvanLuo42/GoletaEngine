#pragma once

/// @file
/// @brief Engine class -- owns the subsystem registry instance for one engine-level lifetime.

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "EngineExport.h"
#include "Subsystem.h"

namespace goleta
{

/// @brief Central lifecycle owner for a collection of subsystems.
/// @note  One process owns at most one Engine (or Engine subclass). Not thread-safe;
///        subsystems may be touched from other threads once started, but start()/tick()/stop()
///        run on a single thread (the main loop thread).
class ENGINE_API Engine
{
public:
    Engine();
    virtual ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// @brief Instantiate every registered subsystem this engine accepts and call initialize()
    ///        on each, in registration order. Idempotent -- a second call is a no-op.
    void start();

    /// @brief Advance one frame: call tick() on every subsystem whose shouldTick() is true,
    ///        in registration order. Requires start() to have been called.
    void tick(float DeltaSeconds);

    /// @brief Call deinitialize() on every subsystem in reverse of initialize() order,
    ///        then destroy them. Safe to call even if start() was never called.
    void stop();

    /// @brief Whether start() has been called and stop() has not.
    bool isRunning() const { return Running; }

    /// @brief Return the subsystem of type T owned by this engine, or nullptr if none is registered.
    /// @note  The TypeId invariant (key -> concrete type) is enforced at registration time; the
    ///        factory that produced Slot is guaranteed to have returned a T. There is no
    ///        dynamic_cast check here because the engine builds -fno-rtti / /GR-.
    template <class T>
    T* findSubsystem()
    {
        auto It = Subsystems.find(detail::subsystemTypeId<T>());
        if (It == Subsystems.end())
            return nullptr;
        return static_cast<T*>(It->second.get());
    }

    /// @brief Return the subsystem of type T. Asserts in debug if the subsystem is not registered.
    template <class T>
    T& getSubsystem()
    {
        T* Ptr = findSubsystem<T>();
        assert(Ptr && "Subsystem of requested type is not registered with this Engine");
        return *Ptr;
    }

protected:
    /// @brief Whether this engine instantiates subsystems of the given category at start().
    /// @note  Default accepts Engine and Game. Override in EditorEngine to additionally accept Editor.
    virtual bool acceptsCategory(SubsystemCategory Category) const;

private:
    std::unordered_map<detail::SubsystemTypeId, std::unique_ptr<Subsystem>> Subsystems;
    std::vector<Subsystem*> InitOrder;
    std::vector<Subsystem*> TickOrder;
    bool Running = false;
};

} // namespace goleta
