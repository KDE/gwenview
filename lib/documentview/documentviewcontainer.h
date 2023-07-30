// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef DOCUMENTVIEWCONTAINER_H
#define DOCUMENTVIEWCONTAINER_H

#include <lib/gwenviewlib_export.h>

// Local
#include <lib/documentview/documentview.h>

// KF

// Qt
#include <QGraphicsView>
#include <QUrl>

namespace Gwenview
{
class DocumentView;

struct DocumentViewContainerPrivate;
/**
 * A container for DocumentViews which will arrange them to make best use of
 * available space.
 *
 * All creations and deletions of DocumentViews/images use this class even if
 * only a single image is viewed.
 */
class GWENVIEWLIB_EXPORT DocumentViewContainer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit DocumentViewContainer(QWidget *parent = nullptr);
    ~DocumentViewContainer() override;

    /**
     * Create a DocumentView in the DocumentViewContainer scene
     */
    DocumentView *createView();

    /**
     * Delete view. Note that the view will first be faded to black before
     * being destroyed.
     */
    void deleteView(DocumentView *view);

    /**
     * Immediately delete all views
     */
    void reset();

    /**
     * Returns saved Setup configuration for a previously viewed document
     */
    DocumentView::Setup savedSetup(const QUrl &url) const;

    /**
     * Updates setupForUrl hash with latest setup values
     */
    void updateSetup(DocumentView *view);

    void showMessageWidget(QGraphicsWidget *, Qt::Alignment);

    /**
     * Set palette on this and all document views
     */
    void applyPalette(const QPalette &palette);

public Q_SLOTS:
    void updateLayout();

protected:
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    friend class ViewItem;
    DocumentViewContainerPrivate *const d;

private Q_SLOTS:
    void slotFadeInFinished(DocumentView *);
    void pretendFadeInFinished();
    void slotConfigChanged();
};

} // namespace

#endif /* DOCUMENTVIEWCONTAINER_H */
