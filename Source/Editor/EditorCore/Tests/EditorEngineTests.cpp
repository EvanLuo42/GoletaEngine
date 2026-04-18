/// @file
/// @brief Unit tests for EditorEngine.

#include <gtest/gtest.h>

#include "EditorCore.h"

namespace
{

using namespace goleta;

class EditorCategoryProbe : public Subsystem
{
public:
    int InitCount = 0;
    void initialize(Engine& /*Owner*/) override { ++InitCount; }
};

class EditorEngineGameProbe : public GameSubsystem
{
public:
    int InitCount = 0;
    void initialize(Engine& /*Owner*/) override { ++InitCount; }
};

class EditorEngineEngineProbe : public EngineSubsystem
{
public:
    int InitCount = 0;
    void initialize(Engine& /*Owner*/) override { ++InitCount; }
};

} // namespace

GOLETA_REGISTER_SUBSYSTEM(EditorCategoryProbe, ::goleta::SubsystemCategory::Editor)
GOLETA_REGISTER_SUBSYSTEM(EditorEngineGameProbe, ::goleta::SubsystemCategory::Game)
GOLETA_REGISTER_SUBSYSTEM(EditorEngineEngineProbe, ::goleta::SubsystemCategory::Engine)

TEST(EditorEngineTest, AcceptsAllThreeCategories)
{
    EditorEngine E;
    E.start();

    auto* Editor = E.findSubsystem<EditorCategoryProbe>();
    auto* Game = E.findSubsystem<EditorEngineGameProbe>();
    auto* EngineS = E.findSubsystem<EditorEngineEngineProbe>();

    ASSERT_NE(Editor, nullptr);
    ASSERT_NE(Game, nullptr);
    ASSERT_NE(EngineS, nullptr);

    EXPECT_EQ(Editor->InitCount, 1);
    EXPECT_EQ(Game->InitCount, 1);
    EXPECT_EQ(EngineS->InitCount, 1);
}

TEST(EditorEngineTest, PlainEngineStillRejectsEditorCategory)
{
    Engine E;
    E.start();
    EXPECT_EQ(E.findSubsystem<EditorCategoryProbe>(), nullptr);
}

TEST(EditorEngineTest, DestructorImplicitlyStops)
{
    {
        EditorEngine E;
        E.start();
        ASSERT_TRUE(E.isRunning());
    }
    SUCCEED();
}

TEST(EditorEngineTest, TickAfterStartDoesNotCrash)
{
    EditorEngine E;
    E.start();
    E.tick(0.016f);
    E.tick(0.016f);
    SUCCEED();
}
