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
#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGraphicsWidget>

// KDE

// Local
#include <lib/document/document.h>

class QPropertyAnimation;
class QUrl;

namespace Gwenview
{

class AbstractRasterImageViewTool;
class RasterImageView;

struct DocumentViewPrivate;

/**
 * This widget can display various documents, using an instance of
 * AbstractDocumentViewAdapter
 */
class GWENVIEWLIB_EXPORT DocumentView : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(bool zoomToFit READ zoomToFit WRITE setZoomToFit NOTIFY zoomToFitChanged)
    Q_PROPERTY(bool zoomToFill READ zoomToFill WRITE setZoomToFill NOTIFY zoomToFillChanged)
    Q_PROPERTY(QPoint position READ position WRITE setPosition NOTIFY positionChanged)
public:
    static const int MaximumZoom;

    struct Setup {
        Setup()
        : valid(false)
        , zoomToFit(true)
        , zoomToFill(false)
        , zoom(0)
        {}
        bool valid:1;
        bool zoomToFit:1;
        bool zoomToFill:1;
        qreal zoom;
        QPointF position;
    };

    enum AnimationMethod {
        NoAnimation,
        SoftwareAnimation,
        GLAnimation
    };

    /**
     * Create a new view attached to scene. We need the scene to be able to
     * install scene event filters.
     */
    explicit DocumentView(QGraphicsScene* scene);
    ~DocumentView() override;

    Document::Ptr document() const;

    QUrl url() const;

    void openUrl(const QUrl&, const Setup&);

    Setup setup() const;

    /**
     * Tells the current adapter to load its config. Used when the user changed
     * the config while the view was visible.
     */
    void loadAdapterConfig();

    bool canZoom() const;

    qreal minimumZoom() const;

    qreal zoom() const;

    bool isCurrent() const;

    void setCurrent(bool);

    void setCompareMode(bool);

    bool zoomToFit() const;

    bool zoomToFill() const;

    QPoint position() const;

    /**
     * Returns the RasterImageView of the current adapter, if it has one
     */
    RasterImageView* imageView() const;

    AbstractRasterImageViewTool* currentTool() const;

    void moveTo(const QRect&);
    void moveToAnimated(const QRect&);
    QPropertyAnimation* fadeIn();
    void fadeOut();
    void fakeFadeOut();

    void setGeometry(const QRectF& rect) override;

    int sortKey() const;
    void setSortKey(int sortKey);

    bool isAnimated() const;

    /**
     * Sets the opacity on the installed QGraphicsOpacityEffect.
     * Use this instead of setOpacity().
     */
    void setGraphicsEffectOpacity(qreal opacity);

public Q_SLOTS:
    void setZoom(qreal);

    void setZoomToFit(bool);
    void toggleZoomToFit();

    void setZoomToFill(bool);
    void toggleZoomToFill();

    void setPosition(const QPoint&);

    void hideAndDeleteLater();

Q_SIGNALS:
    /**
     * Emitted when the part has finished loading
     */
    void completed();

    void previousImageRequested();

    void nextImageRequested();

    void openUrlRequested(const QUrl&);

    void openDirUrlRequested(const QUrl&);

    void captionUpdateRequested(const QString&);

    void toggleFullScreenRequested();

    void videoFinished();

    void minimumZoomChanged(qreal);

    void zoomChanged(qreal);

    void adapterChanged();

    void focused(DocumentView*);

    void zoomToFitChanged(bool);

    void zoomToFillChanged(bool);

    void positionChanged();

    void hudTrashClicked(DocumentView*);
    void hudDeselectClicked(DocumentView*);

    void fadeInFinished(DocumentView*);

    void contextMenuRequested();

    void currentToolChanged(AbstractRasterImageViewTool*);

    void isAnimatedChanged();

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void resizeEvent(QGraphicsSceneResizeEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void wheelEvent(QGraphicsSceneWheelEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    bool sceneEventFilter(QGraphicsItem*, QEvent*) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;

private Q_SLOTS:
    void finishOpenUrl();
    void slotCompleted();
    void slotLoadingFailed();

    void zoomActualSize();

    void zoomIn(QPointF center = QPointF(-1, -1));
    void zoomOut(QPointF center = QPointF(-1, -1));

    void slotZoomChanged(qreal);

    void slotBusyChanged(const QUrl&, bool);

    void emitHudTrashClicked();
    void emitHudDeselectClicked();
    void emitFocused();

    void slotFadeInFinished();

    void dragThumbnailLoaded(const KFileItem&, const QPixmap&);
    void dragThumbnailLoadingFailed(const KFileItem&);

private:
    friend struct DocumentViewPrivate;
    DocumentViewPrivate* const d;

    void createAdapterForDocument();
};

} // namespace

#endif /* DOCUMENTVIEW_H */
