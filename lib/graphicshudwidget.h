// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef GRAPHICSHUDWIDGET_H
#define GRAPHICSHUDWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGraphicsWidget>
#include <QWidget>

// KDE

// Local

namespace Gwenview
{

struct GraphicsHudWidgetPrivate;
class GWENVIEWLIB_EXPORT GraphicsHudWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    enum Option {
        OptionNone                 = 0,
        OptionCloseButton          = 1 << 1,
        OptionOpaque               = 1 << 2
    };
    Q_DECLARE_FLAGS(Options, Option)

    GraphicsHudWidget(QGraphicsWidget* parent = 0);
    ~GraphicsHudWidget();

    void init(QWidget*, Options options);
    void init(QGraphicsWidget*, Options options);

    void setAutoDeleteOnFadeout(bool autoDelete);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

public Q_SLOTS:
    void fadeIn();
    void fadeOut();

Q_SIGNALS:
    void closed();
    void fadedIn();
    void fadedOut();

private Q_SLOTS:
    void slotCloseButtonClicked();
    void slotFadeAnimationFinished();

private:
    GraphicsHudWidgetPrivate* const d;
};

} // namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::GraphicsHudWidget::Options)

#endif /* GRAPHICSHUDWIDGET_H */
