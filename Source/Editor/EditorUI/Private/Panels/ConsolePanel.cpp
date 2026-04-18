/// @file
/// @brief ConsolePanel implementation.

#include "Panels/ConsolePanel.h"

#include <QObject>
#include <QPlainTextEdit>

namespace goleta
{

QString ConsolePanel::panelId() const
{
    return QStringLiteral("goleta.console");
}

QString ConsolePanel::displayName() const
{
    return QObject::tr("Console");
}

QWidget* ConsolePanel::createWidget(QWidget* const Parent)
{
    auto* const Edit = new QPlainTextEdit(Parent);
    Edit->setReadOnly(true);
    Edit->setPlaceholderText(QObject::tr("Engine log output will appear here."));
    return Edit;
}

} // namespace goleta
