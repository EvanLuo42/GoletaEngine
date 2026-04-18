/// @file
/// @brief EditorMainWindow implementation.

#include "EditorMainWindow.h"

#include <cassert>

#include <QCloseEvent>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

#include "IEditorPanel.h"

namespace goleta
{

EditorMainWindow::EditorMainWindow(const EditorPanelRegistry& Registry, QWidget* const Parent)
    : QMainWindow(Parent)
{
    setWindowTitle(tr("Goleta Editor"));
    resize(1280, 720);
    setDockNestingEnabled(true);

    installPanels(Registry);

    statusBar()->showMessage(tr("Engine: initializing..."));

    Engine.start();
    FrameTimer.start();
    statusBar()->showMessage(tr("Engine: running"));

    connect(&TickTimer, &QTimer::timeout, this, &EditorMainWindow::onTick);
    TickTimer.start(16);
}

EditorMainWindow::~EditorMainWindow()
{
    TickTimer.stop();
    if (Engine.isRunning())
    {
        Engine.stop();
    }
}

void EditorMainWindow::closeEvent(QCloseEvent* const Event)
{
    TickTimer.stop();
    if (Engine.isRunning())
    {
        Engine.stop();
    }
    QMainWindow::closeEvent(Event);
}

void EditorMainWindow::installPanels(const EditorPanelRegistry& Registry)
{
    QMenu* const ViewMenu = menuBar()->addMenu(tr("&View"));
    bool HasCentral = false;

    for (const auto& PanelBox : Registry.all())
    {
        IEditorPanel& Panel = *PanelBox;

        if (Panel.role() == PanelRole::Central)
        {
            QWidget* const Widget = Panel.createWidget(this);
            assert(Widget);
            assert(!HasCentral && "EditorMainWindow: multiple central panels registered");
            setCentralWidget(Widget);
            HasCentral = true;
            continue;
        }

        auto* const Dock = new QDockWidget(Panel.displayName(), this);
        Dock->setObjectName(Panel.panelId());
        QWidget* const Widget = Panel.createWidget(Dock);
        assert(Widget);
        Dock->setWidget(Widget);
        addDockWidget(Panel.defaultArea(), Dock);
        ViewMenu->addAction(Dock->toggleViewAction());
    }
}

void EditorMainWindow::onTick()
{
    const qint64 NanosSinceLast = FrameTimer.nsecsElapsed();
    FrameTimer.restart();
    const float DeltaSeconds = static_cast<float>(NanosSinceLast) * 1.0e-9f;
    Engine.tick(DeltaSeconds);
}

} // namespace goleta
