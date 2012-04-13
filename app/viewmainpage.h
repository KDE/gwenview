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
#ifndef VIEWMAINPAGE_H
#define VIEWMAINPAGE_H

// Local
#include <lib/document/document.h>

// KDE
#include <KUrl>

// Qt
#include <QToolButton>
#include <QWidget>

class QGraphicsWidget;
class QPalette;

class KActionCollection;

namespace Gwenview
{

class DocumentView;
class RasterImageView;
class SlideShow;
class ThumbnailBarView;

struct ViewMainPagePrivate;

/**
 * Holds the active document view and associated widgetry.
 */
class ViewMainPage : public QWidget
{
    Q_OBJECT
public:
    static const int MaxViewCount;

    ViewMainPage(QWidget* parent, SlideShow*, KActionCollection*);
    ~ViewMainPage();

    ThumbnailBarView* thumbnailBar() const;

    void loadConfig();

    void saveConfig();

    /**
     * Reset the view
     */
    void reset();

    void setFullScreenMode(bool fullScreen);

    bool isFullScreenMode() const;

    void setNormalPalette(const QPalette&);

    int statusBarHeight() const;

    virtual QSize sizeHint() const;

    /**
     * Returns the url of the current document, or an invalid url if unknown
     */
    KUrl url() const;

    void openUrl(const KUrl& url);

    /**
     * Opens up to MaxViewCount urls, and set currentUrl as the current one
     */
    void openUrls(const KUrl::List& urls, const KUrl& currentUrl);

    void reload();

    Document::Ptr currentDocument() const;

    bool isEmpty() const;

    /**
     * Returns the image view, if the current adapter has one.
     */
    RasterImageView* imageView() const;

    /**
     * Returns the document view
     */
    DocumentView* documentView() const;

    /**
     * Sets a widget to show at the bottom of the panel
     */
    void setToolWidget(QWidget* widget);

    QToolButton* toggleSideBarButton() const;

    void showMessageWidget(QGraphicsWidget*, Qt::Alignment align = Qt::AlignHCenter | Qt::AlignTop);

Q_SIGNALS:

    /**
     * Emitted when the part has finished loading
     */
    void completed();

    void previousImageRequested();

    void nextImageRequested();

    void toggleFullScreenRequested();

    void goToBrowseModeRequested();

    void captionUpdateRequested(const QString&);

private Q_SLOTS:
    void setThumbnailBarVisibility(bool visible);

    void showContextMenu();

    void slotViewFocused(DocumentView*);

    void trashView(DocumentView*);
    void deselectView(DocumentView*);

private:
    friend struct ViewMainPagePrivate;
    ViewMainPagePrivate* const d;
};

} // namespace

#endif /* VIEWMAINPAGE_H */
