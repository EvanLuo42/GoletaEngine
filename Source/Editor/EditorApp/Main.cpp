/// @file
/// @brief Editor executable entry point.

#include <memory>

#include <QApplication>

#include "EditorMainWindow.h"
#include "EditorPanelRegistry.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ViewportPanel.h"

int main(int ArgCount, char** ArgValues)
{
    QApplication App(ArgCount, ArgValues);
    QCoreApplication::setOrganizationName(QStringLiteral("Goleta"));
    QCoreApplication::setApplicationName(QStringLiteral("GoletaEditor"));

    goleta::EditorPanelRegistry Registry;
    Registry.add(std::make_unique<goleta::ViewportPanel>());
    Registry.add(std::make_unique<goleta::ConsolePanel>());

    goleta::EditorMainWindow Window(Registry);
    Window.show();

    return QApplication::exec();
}
