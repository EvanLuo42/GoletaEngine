/// @file
/// @brief Unit tests for EditorPanelRegistry and IEditorPanel defaults.

#include <gtest/gtest.h>

#include <memory>

#include <QString>

#include "EditorPanelRegistry.h"

namespace
{

using namespace goleta;

class FakePanel : public IEditorPanel
{
public:
    FakePanel(QString InId, QString InName) : Id(std::move(InId)), Name(std::move(InName)) {}

    QString panelId() const override { return Id; }
    QString displayName() const override { return Name; }
    QWidget* createWidget(QWidget* /*Parent*/) override { return nullptr; }

    QString Id;
    QString Name;
};

class CentralFakePanel : public FakePanel
{
public:
    using FakePanel::FakePanel;
    PanelRole role() const override { return PanelRole::Central; }
};

} // namespace

TEST(EditorPanelRegistry, StartsEmpty)
{
    EditorPanelRegistry R;
    EXPECT_EQ(R.all().size(), 0u);
}

TEST(EditorPanelRegistry, AddAppendsInRegistrationOrder)
{
    EditorPanelRegistry R;
    R.add(std::make_unique<FakePanel>(QStringLiteral("a"), QStringLiteral("Alpha")));
    R.add(std::make_unique<FakePanel>(QStringLiteral("b"), QStringLiteral("Beta")));

    ASSERT_EQ(R.all().size(), 2u);
    EXPECT_EQ(R.all()[0]->panelId(), QStringLiteral("a"));
    EXPECT_EQ(R.all()[1]->panelId(), QStringLiteral("b"));
}

TEST(EditorPanelRegistry, PanelMetadataIsReadBackUnchanged)
{
    EditorPanelRegistry R;
    R.add(std::make_unique<FakePanel>(QStringLiteral("x"), QStringLiteral("X Pane")));
    EXPECT_EQ(R.all()[0]->displayName(), QStringLiteral("X Pane"));
}

TEST(IEditorPanelDefaults, DockRoleAndRightArea)
{
    FakePanel F(QStringLiteral("f"), QStringLiteral("F"));
    EXPECT_EQ(F.role(), PanelRole::Dock);
    EXPECT_EQ(F.defaultArea(), Qt::RightDockWidgetArea);
}

TEST(IEditorPanelDefaults, RoleOverrideTakesEffect)
{
    CentralFakePanel C(QStringLiteral("c"), QStringLiteral("C"));
    EXPECT_EQ(C.role(), PanelRole::Central);
}
