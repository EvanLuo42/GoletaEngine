/// @file
/// @brief ViewportPanel implementation.

#include "Panels/ViewportPanel.h"

#include <QLabel>
#include <QObject>

namespace goleta
{

QString ViewportPanel::panelId() const
{
    return QStringLiteral("goleta.viewport");
}

QString ViewportPanel::displayName() const
{
    return QObject::tr("Viewport");
}

QWidget* ViewportPanel::createWidget(QWidget* const Parent)
{
    auto* const Label = new QLabel(QObject::tr("Viewport placeholder"), Parent);
    Label->setAlignment(Qt::AlignCenter);
    return Label;
}

} // namespace goleta
