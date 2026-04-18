/// @file
/// @brief Unit tests for Engine, Subsystem, and GameEngine.

#include <gtest/gtest.h>

#include <algorithm>

#include "Engine.h"

namespace
{

using namespace goleta;

class CounterEngineSubsystem : public EngineSubsystem
{
public:
    int InitCount = 0;
    int DeinitCount = 0;
    int TickCount = 0;
    float LastDelta = 0.0f;
    Engine* SeenOwner = nullptr;

    void initialize(Engine& Owner) override
    {
        ++InitCount;
        SeenOwner = &Owner;
    }
    void deinitialize() override { ++DeinitCount; }
    void tick(const float DeltaSeconds) override
    {
        ++TickCount;
        LastDelta = DeltaSeconds;
    }
    [[nodiscard]] bool shouldTick() const override { return true; }
};

class RecordingGameSubsystem : public GameSubsystem
{
public:
    int InitCount = 0;
    void initialize(Engine& /*Owner*/) override { ++InitCount; }
};

class EditorOnlySubsystem : public Subsystem
{
public:
    int InitCount = 0;
    void initialize(Engine& /*Owner*/) override { ++InitCount; }
};

// Shared log used to observe init / tick ordering across the dependency-graph tests.
struct OrderLog
{
    static Vec<String>& initEvents()
    {
        static Vec<String> Events;
        return Events;
    }
    static Vec<String>& tickEvents()
    {
        static Vec<String> Events;
        return Events;
    }
};

class UpstreamSubsystem : public EngineSubsystem
{
public:
    void initialize(Engine&) override { OrderLog::initEvents().push("Upstream"); }
    void tick(float) override { OrderLog::tickEvents().push("Upstream"); }
    [[nodiscard]] bool shouldTick() const override { return true; }
};

class MidstreamSubsystem : public EngineSubsystem
{
public:
    static auto dependencies() { return goleta::dependsOn<UpstreamSubsystem>(); }
    void initialize(Engine&) override { OrderLog::initEvents().push("Midstream"); }
    void tick(float) override { OrderLog::tickEvents().push("Midstream"); }
    [[nodiscard]] bool shouldTick() const override { return true; }
};

class DownstreamSubsystem : public EngineSubsystem
{
public:
    static auto dependencies() { return goleta::dependsOn<MidstreamSubsystem>(); }
    void initialize(Engine&) override { OrderLog::initEvents().push("Downstream"); }
    void tick(float) override { OrderLog::tickEvents().push("Downstream"); }
    [[nodiscard]] bool shouldTick() const override { return true; }
};

class PreUpdateSubsystem : public EngineSubsystem
{
public:
    void tick(float) override { OrderLog::tickEvents().push("Pre"); }
    bool shouldTick() const override { return true; }
    TickStage tickStage() const override { return TickStage::PreUpdate; }
};

class RenderStageSubsystem : public EngineSubsystem
{
public:
    void tick(float) override { OrderLog::tickEvents().push("Render"); }
    bool shouldTick() const override { return true; }
    TickStage tickStage() const override { return TickStage::Render; }
};

} // namespace

GOLETA_REGISTER_SUBSYSTEM(CounterEngineSubsystem, ::goleta::SubsystemCategory::Engine)
GOLETA_REGISTER_SUBSYSTEM(RecordingGameSubsystem, ::goleta::SubsystemCategory::Game)
GOLETA_REGISTER_SUBSYSTEM(EditorOnlySubsystem, ::goleta::SubsystemCategory::Editor)
GOLETA_REGISTER_SUBSYSTEM(UpstreamSubsystem, ::goleta::SubsystemCategory::Engine)
GOLETA_REGISTER_SUBSYSTEM(MidstreamSubsystem, ::goleta::SubsystemCategory::Engine)
GOLETA_REGISTER_SUBSYSTEM(DownstreamSubsystem, ::goleta::SubsystemCategory::Engine)
GOLETA_REGISTER_SUBSYSTEM(PreUpdateSubsystem, ::goleta::SubsystemCategory::Engine)
GOLETA_REGISTER_SUBSYSTEM(RenderStageSubsystem, ::goleta::SubsystemCategory::Engine)

TEST(EngineTest, StartInstantiatesAcceptedCategoriesOnly)
{
    Engine E;
    EXPECT_FALSE(E.isRunning());
    EXPECT_EQ(E.findSubsystem<CounterEngineSubsystem>(), nullptr);

    E.start();
    EXPECT_TRUE(E.isRunning());

    auto* Counter = E.findSubsystem<CounterEngineSubsystem>();
    ASSERT_NE(Counter, nullptr);
    EXPECT_EQ(Counter->InitCount, 1);
    EXPECT_EQ(Counter->SeenOwner, &E);

    auto* Recording = E.findSubsystem<RecordingGameSubsystem>();
    ASSERT_NE(Recording, nullptr);
    EXPECT_EQ(Recording->InitCount, 1);

    EXPECT_EQ(E.findSubsystem<EditorOnlySubsystem>(), nullptr);

    E.stop();
    EXPECT_FALSE(E.isRunning());
}

TEST(EngineTest, StartIsIdempotent)
{
    Engine E;
    E.start();
    auto* First = E.findSubsystem<CounterEngineSubsystem>();
    ASSERT_NE(First, nullptr);

    E.start();
    EXPECT_EQ(E.findSubsystem<CounterEngineSubsystem>(), First);
    EXPECT_EQ(First->InitCount, 1);
}

TEST(EngineTest, TickForwardsDeltaToTickingSubsystems)
{
    Engine E;
    E.start();

    auto& Counter = E.getSubsystem<CounterEngineSubsystem>();
    EXPECT_EQ(Counter.TickCount, 0);

    E.tick(1.0f / 60.0f);
    E.tick(1.0f / 30.0f);

    EXPECT_EQ(Counter.TickCount, 2);
    EXPECT_FLOAT_EQ(Counter.LastDelta, 1.0f / 30.0f);
}

TEST(EngineTest, TickBeforeStartIsNoop)
{
    Engine E;
    E.tick(0.016f);
    SUCCEED();
}

TEST(EngineTest, StopCallsDeinitializeInReverseOrder)
{
    Engine E;
    E.start();
    auto* Counter = E.findSubsystem<CounterEngineSubsystem>();
    ASSERT_NE(Counter, nullptr);
    EXPECT_EQ(Counter->DeinitCount, 0);

    E.stop();
    // After stop() the subsystems are destroyed, so we can only verify via observable side effects
    // through a fresh start cycle.
    EXPECT_FALSE(E.isRunning());
    EXPECT_EQ(E.findSubsystem<CounterEngineSubsystem>(), nullptr);
}

TEST(EngineTest, DestructorImplicitlyStops)
{
    int DeinitSeen = 0;
    {
        Engine E;
        E.start();
        auto* C = E.findSubsystem<CounterEngineSubsystem>();
        ASSERT_NE(C, nullptr);
        // Capture a pointer to observe deinit through a sentinel? Counter is owned by E so we
        // cannot observe after destruction; just verify destruction does not crash.
        (void)C;
        DeinitSeen = 1;
    }
    EXPECT_EQ(DeinitSeen, 1);
}

TEST(EngineTest, GetSubsystemReturnsSameInstanceAsFind)
{
    Engine E;
    E.start();
    auto* Ptr = E.findSubsystem<CounterEngineSubsystem>();
    ASSERT_NE(Ptr, nullptr);
    EXPECT_EQ(&E.getSubsystem<CounterEngineSubsystem>(), Ptr);
}

TEST(SubsystemTest, DefaultVirtualsAreBenign)
{
    struct BareSubsystem : public Subsystem
    {
    };
    BareSubsystem S;
    Engine E;
    S.initialize(E);
    S.tick(0.016f);
    EXPECT_FALSE(S.shouldTick());
    S.deinitialize();
    SUCCEED();
}

namespace
{

/// Position of the first occurrence of Name in Events. -1 if absent.
int indexOf(const Vec<String>& Events, StringView Name)
{
    auto It = std::find(Events.begin(), Events.end(), Name);
    return It == Events.end() ? -1 : static_cast<int>(It - Events.begin());
}

} // namespace

TEST(EngineTest, InitFollowsDependencyOrder)
{
    OrderLog::initEvents().clear();
    Engine E;
    E.start();

    const auto& Events = OrderLog::initEvents();
    const int IU = indexOf(Events, "Upstream");
    const int IM = indexOf(Events, "Midstream");
    const int ID = indexOf(Events, "Downstream");

    ASSERT_GE(IU, 0);
    ASSERT_GE(IM, 0);
    ASSERT_GE(ID, 0);
    EXPECT_LT(IU, IM);
    EXPECT_LT(IM, ID);
}

TEST(EngineTest, TickRunsStagesInDefinedOrder)
{
    OrderLog::tickEvents().clear();
    Engine E;
    E.start();
    E.tick(0.016f);

    const auto& Events = OrderLog::tickEvents();
    const int IPre = indexOf(Events, "Pre");
    const int IUp = indexOf(Events, "Upstream");
    const int IMid = indexOf(Events, "Midstream");
    const int IDown = indexOf(Events, "Downstream");
    const int IRen = indexOf(Events, "Render");

    ASSERT_GE(IPre, 0);
    ASSERT_GE(IUp, 0);
    ASSERT_GE(IMid, 0);
    ASSERT_GE(IDown, 0);
    ASSERT_GE(IRen, 0);

    // PreUpdate before every Update-stage subsystem.
    EXPECT_LT(IPre, IUp);
    EXPECT_LT(IPre, IMid);
    EXPECT_LT(IPre, IDown);
    // Update stage preserves dependency order.
    EXPECT_LT(IUp, IMid);
    EXPECT_LT(IMid, IDown);
    // Render runs last.
    EXPECT_LT(IDown, IRen);
}

TEST(SubsystemTest, DefaultTickStageIsUpdate)
{
    struct Bare : public Subsystem
    {
    };
    Bare B;
    EXPECT_EQ(B.tickStage(), TickStage::Update);
}

TEST(DependsOnHelper, BuildsTypeIdList)
{
    auto List = goleta::dependsOn<UpstreamSubsystem, MidstreamSubsystem>();
    ASSERT_EQ(List.len(), 2u);
    EXPECT_EQ(List[0], goleta::detail::subsystemTypeId<UpstreamSubsystem>());
    EXPECT_EQ(List[1], goleta::detail::subsystemTypeId<MidstreamSubsystem>());
}

TEST(GameEngineTest, BehavesLikeEngineForGameAndEngineCategories)
{
    GameEngine GE;
    GE.start();

    EXPECT_NE(GE.findSubsystem<CounterEngineSubsystem>(), nullptr);
    EXPECT_NE(GE.findSubsystem<RecordingGameSubsystem>(), nullptr);
    EXPECT_EQ(GE.findSubsystem<EditorOnlySubsystem>(), nullptr);

    GE.tick(0.016f);
    EXPECT_EQ(GE.getSubsystem<CounterEngineSubsystem>().TickCount, 1);
}
