// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef TOOLTIPWIDGET_H
#define TOOLTIPWIDGET_H

// Qt
#include <QWidget>

// KDE

// Local

namespace Gwenview
{

struct ToolTipWidgetPrivate;

/**
 * A label which uses tooltip colors and can be faded
 */
class ToolTipWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    explicit ToolTipWidget(QWidget* parent = nullptr);
    ~ToolTipWidget() Q_DECL_OVERRIDE;

    qreal opacity() const;
    void setOpacity(qreal);

    QString text() const;
    void setText(const QString& text);

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private:
    ToolTipWidgetPrivate* const d;
};

} // namespace

#endif /* TOOLTIPWIDGET_H */
