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

// KF

// Qt
#include <QToolButton>
#include <QUrl>
#include <QWidget>

class QGraphicsWidget;

class KActionCollection;

namespace Gwenview
{
class AbstractRasterImageViewTool;
class DocumentView;
class GvCore;
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

    ViewMainPage(QWidget *parent, SlideShow *, KActionCollection *, GvCore *);
    ~ViewMainPage() override;

    ThumbnailBarView *thumbnailBar() const;

    void loadConfig();

    void saveConfig();

    /**
     * Reset the view
     */
    void reset();

    void setFullScreenMode(bool fullScreen);

    int statusBarHeight() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /**
     * Returns the url of the current document, or an invalid url if unknown
     */
    QUrl url() const;

    void openUrl(const QUrl &url);

    /**
     * Opens up to MaxViewCount urls, and set currentUrl as the current one
     */
    void openUrls(const QList<QUrl> &urls, const QUrl &currentUrl);

    void reload();

    Document::Ptr currentDocument() const;

    bool isEmpty() const;

    /**
     * Returns the image view, if the current adapter has one.
     */
    RasterImageView *imageView() const;

    /**
     * Returns the document view
     */
    DocumentView *documentView() const;

    QToolButton *toggleSideBarButton() const;

    void showMessageWidget(QGraphicsWidget *, Qt::Alignment align = Qt::AlignHCenter | Qt::AlignTop);

Q_SIGNALS:

    /**
     * Emitted when the part has finished loading
     */
    void completed();

    void previousImageRequested();

    void nextImageRequested();

    void openUrlRequested(const QUrl &);

    void openDirUrlRequested(const QUrl &);

    void toggleFullScreenRequested();

    void goToBrowseModeRequested();

    void captionUpdateRequested(const QString &);

public Q_SLOTS:
    void setStatusBarVisible(bool);

private Q_SLOTS:
    void setThumbnailBarVisibility(bool visible);

    void showContextMenu();

    void slotViewFocused(DocumentView *);

    void slotEnterPressed();

    void trashView(DocumentView *);
    void deselectView(DocumentView *);

    void slotDirModelItemsAddedOrRemoved();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    friend struct ViewMainPagePrivate;
    ViewMainPagePrivate *const d;

    void updateFocus(const AbstractRasterImageViewTool *tool);
};

} // namespace

#endif /* VIEWMAINPAGE_H */
