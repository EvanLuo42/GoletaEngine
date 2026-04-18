#pragma once

/// @file
/// @brief Subsystem base classes, categories, dependencies, tick stages, and auto-registration macro.

#include <cstdint>
#include <memory>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

#include "EngineExport.h"

namespace goleta
{

class Engine;

/// @brief Lifecycle category that determines when an Engine instantiates a subsystem.
enum class SubsystemCategory : uint8_t
{
    Engine,
    Game,
    Editor,
};

/// @brief Numeric tick priority. Lower values run earlier; within an equal priority, the engine
///        preserves dependency-topological order (dependencies tick before dependents).
/// @note  Defined as a 16-bit numeric priority so downstream code can slot custom stages between
///        the canonical ones (e.g. `TickStage{150}` between PreUpdate and Update) without the
///        engine having to bake every stage into an enumeration up-front.
enum class TickStage : uint16_t
{
    PreUpdate = 100,
    Update = 200,
    PostUpdate = 300,
    Render = 400,
};

/// @brief Base class for all subsystems managed by Engine.
class ENGINE_API Subsystem
{
public:
    virtual ~Subsystem() = default;

    Subsystem(const Subsystem&) = delete;
    Subsystem& operator=(const Subsystem&) = delete;

    /// @brief Called once after construction, after declared dependencies are available.
    virtual void initialize(Engine& Owner);

    /// @brief Called once before destruction, in reverse of initialize() order.
    virtual void deinitialize();

    /// @brief Called every frame during tickStage() if shouldTick() returns true.
    virtual void tick(float DeltaSeconds);

    /// @brief Whether this subsystem participates in the engine tick loop.
    virtual bool shouldTick() const;

    /// @brief Which tick stage this subsystem runs in. Defaults to TickStage::Update.
    virtual TickStage tickStage() const;

protected:
    Subsystem() = default;
};

/// @brief Base for subsystems scoped to the full Engine lifetime.
class ENGINE_API EngineSubsystem : public Subsystem
{
};

/// @brief Base for subsystems scoped to a GameInstance / World.
class ENGINE_API GameSubsystem : public Subsystem
{
};

namespace detail
{

/// @brief Identity for a Subsystem class at registration and lookup time.
/// @note  Backed by std::type_index so the same T resolves to the same id across DSO
///        boundaries. Using the address of a function-local static char works within a single
///        shared library but silently diverges when two modules each instantiate the template,
///        which is fatal for plugin-style loading.
using SubsystemTypeId = std::type_index;

struct SubsystemFactoryEntry
{
    SubsystemTypeId TypeId;
    SubsystemCategory Category;
    /// @note Name must reference storage with static lifetime (typically the stringised class
    ///       name from GOLETA_REGISTER_SUBSYSTEM). std::string_view does not own its bytes.
    std::string_view Name;
    std::unique_ptr<Subsystem> (*Factory)();
    std::vector<SubsystemTypeId> Dependencies;
};

/// @brief Append an entry to the global subsystem registry. Called from macro expansions at static-init time.
ENGINE_API void registerSubsystemFactory(SubsystemFactoryEntry Entry);

template <class T>
SubsystemTypeId subsystemTypeId()
{
    return SubsystemTypeId{typeid(T)};
}

template <class T>
std::vector<SubsystemTypeId> gatherSubsystemDependencies()
{
    if constexpr (requires { T::dependencies(); })
    {
        auto Result = T::dependencies();
        return std::vector<SubsystemTypeId>(Result.begin(), Result.end());
    }
    else
    {
        return {};
    }
}

template <class T>
struct SubsystemRegistrar
{
    SubsystemRegistrar(const SubsystemCategory Category, const std::string_view Name)
    {
        registerSubsystemFactory(SubsystemFactoryEntry{
            subsystemTypeId<T>(),
            Category,
            Name,
            []() -> std::unique_ptr<Subsystem> { return std::make_unique<T>(); },
            gatherSubsystemDependencies<T>(),
        });
    }
};

} // namespace detail

/// @brief Helper to declare a subsystem's static dependency list.
/// @note  Use as: `static auto dependencies() { return goleta::dependsOn<AssetSubsystem, InputSubsystem>(); }`
template <class... Deps>
std::vector<detail::SubsystemTypeId> dependsOn()
{
    std::vector<detail::SubsystemTypeId> Out;
    Out.reserve(sizeof...(Deps));
    (Out.push_back(detail::subsystemTypeId<Deps>()), ...);
    return Out;
}

} // namespace goleta

#define GOLETA_CONCAT_INNER(A, B) A##B
#define GOLETA_CONCAT(A, B)       GOLETA_CONCAT_INNER(A, B)

/// @brief Register a subsystem class with the global registry at static-init time.
/// @note Place in a .cpp file. T must be default-constructible and derive from Subsystem.
///       When the owning module is linked statically, use whole-archive so static initializers
///       are not stripped.
#define GOLETA_REGISTER_SUBSYSTEM(Type, Category)                                                       \
    namespace                                                                                           \
    {                                                                                                   \
    static const ::goleta::detail::SubsystemRegistrar<Type> GOLETA_CONCAT(GoletaSubsystemRegistrar_,    \
                                                                          __LINE__){(Category), #Type}; \
    }
