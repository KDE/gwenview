/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SLIDECONTAINER_H
#define SLIDECONTAINER_H

// Qt
#include <QWeakPointer>
#include <QFrame>

#include <lib/gwenviewlib_export.h>

class QPropertyAnimation;

namespace Gwenview
{

/**
 * This widget is design to contain one child widget, the "content" widget.
 * It will start hidden by default. Calling slideIn() will slide in the content
 * widget from the top border. Calling slideOut() will slide it out.
 */
class GWENVIEWLIB_EXPORT SlideContainer : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(int slideHeight READ slideHeight WRITE setSlideHeight)
public:
    SlideContainer(QWidget* parent = 0);

    /**
     * Returns the content widget
     */
    QWidget* content() const;

    /**
     * Defines the content widget
     */
    void setContent(QWidget* content);

    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

    virtual QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    int slideHeight() const;

    Q_INVOKABLE void setSlideHeight(int height);

public Q_SLOTS:
    /**
     * Slides the content widget in.
     * Calling it multiple times won't cause the animation to be replayed.
     */
    void slideIn();

    /**
     * Slides the content widget out.
     * Calling it multiple times won't cause the animation to be replayed.
     */
    void slideOut();

Q_SIGNALS:
    void slidedIn();
    void slidedOut();

protected:
    void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
    bool eventFilter(QObject*, QEvent* event) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotAnimFinished();

private:
    QWidget* mContent;
    QWeakPointer<QPropertyAnimation> mAnim;
    bool mSlidingOut;

    void adjustContentGeometry();

    void animTo(int height);
};

} /* namespace */

#endif /* SLIDECONTAINER_H */
