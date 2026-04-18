#pragma once

/// @file
/// @brief Top-level editor main window: materialises panels from a registry, drives engine tick.

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include "EditorCore.h"
#include "EditorPanelRegistry.h"
#include "EditorUIExport.h"

namespace goleta
{

/// @brief Main editor window. Takes a panel registry by reference, builds a QDockWidget per
///        IEditorPanel with role Dock, installs the single Central panel as the central widget,
///        and exposes toggles under the View menu. Owns the EditorEngine and ticks it on a QTimer.
/// @note  Requires a live QApplication on the calling thread. Not unit-testable without a
///        Qt event loop; covered by manual smoke runs of EditorApp and by direct tests of
///        EditorPanelRegistry.
class EDITORUI_API EditorMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EditorMainWindow(const EditorPanelRegistry& Registry, QWidget* Parent = nullptr);
    ~EditorMainWindow() override;

protected:
    void closeEvent(QCloseEvent* Event) override;

private:
    void installPanels(const EditorPanelRegistry& Registry);
    void onTick();

    EditorEngine Engine;
    QTimer TickTimer;
    QElapsedTimer FrameTimer;
};

} // namespace goleta
