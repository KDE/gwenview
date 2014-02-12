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
// Self
#include "documentview.moc"

// Qt
#include <QAbstractScrollArea>
#include <QApplication>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWeakPointer>

// KDE
#include <KDebug>
#include <KIcon>
#include <KLocale>
#include <KUrl>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/documentview/birdeyeview.h>
#include <lib/documentview/loadingindicator.h>
#include <lib/documentview/messageviewadapter.h>
#include <lib/documentview/rasterimageview.h>
#include <lib/documentview/rasterimageviewadapter.h>
#include <lib/documentview/svgviewadapter.h>
#include <lib/documentview/videoviewadapter.h>
#include <lib/graphicshudbutton.h>
#include <lib/graphicshudwidget.h>
#include <lib/graphicswidgetfloater.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/signalblocker.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

static const qreal REAL_DELTA = 0.001;
static const qreal MAXIMUM_ZOOM_VALUE = qreal(DocumentView::MaximumZoom);

static const int COMPARE_MARGIN = 4;

const int DocumentView::MaximumZoom = 16;
const int DocumentView::AnimDuration = 250;

struct DocumentViewPrivate
{
    DocumentView* q;
    int mSortKey; // Used to sort views when displayed in compare mode
    GraphicsHudWidget* mHud;
    BirdEyeView* mBirdEyeView;
    QWeakPointer<QPropertyAnimation> mMoveAnimation;
    QWeakPointer<QPropertyAnimation> mFadeAnimation;

    LoadingIndicator* mLoadingIndicator;

    QScopedPointer<AbstractDocumentViewAdapter> mAdapter;
    QList<qreal> mZoomSnapValues;
    Document::Ptr mDocument;
    DocumentView::Setup mSetup;
    bool mCurrent;
    bool mCompareMode;
    bool mEraseBorders;

    void setCurrentAdapter(AbstractDocumentViewAdapter* adapter)
    {
        Q_ASSERT(adapter);
        mAdapter.reset(adapter);

        adapter->widget()->setParentItem(q);
        resizeAdapterWidget();

        if (adapter->canZoom()) {
            QObject::connect(adapter, SIGNAL(zoomChanged(qreal)),
                             q, SLOT(slotZoomChanged(qreal)));
            QObject::connect(adapter, SIGNAL(zoomInRequested(QPointF)),
                             q, SLOT(zoomIn(QPointF)));
            QObject::connect(adapter, SIGNAL(zoomOutRequested(QPointF)),
                             q, SLOT(zoomOut(QPointF)));
            QObject::connect(adapter, SIGNAL(zoomToFitChanged(bool)),
                             q, SIGNAL(zoomToFitChanged(bool)));
        }
        QObject::connect(adapter, SIGNAL(scrollPosChanged()),
                         q, SIGNAL(positionChanged()));
        QObject::connect(adapter, SIGNAL(previousImageRequested()),
                         q, SIGNAL(previousImageRequested()));
        QObject::connect(adapter, SIGNAL(nextImageRequested()),
                         q, SIGNAL(nextImageRequested()));
        QObject::connect(adapter, SIGNAL(toggleFullScreenRequested()),
                         q, SIGNAL(toggleFullScreenRequested()));
        QObject::connect(adapter, SIGNAL(completed()),
                         q, SLOT(slotCompleted()));

        adapter->loadConfig();

        adapter->widget()->installSceneEventFilter(q);
        if (mCurrent) {
            adapter->widget()->setFocus();
        }

        if (mSetup.valid && adapter->canZoom()) {
            adapter->setZoomToFit(mSetup.zoomToFit);
            if (!mSetup.zoomToFit) {
                adapter->setZoom(mSetup.zoom);
                adapter->setScrollPos(mSetup.position);
            }
        }
        q->adapterChanged();
        q->positionChanged();
        if (adapter->canZoom()) {
            q->zoomToFitChanged(adapter->zoomToFit());
        }
        if (adapter->rasterImageView()) {
            QObject::connect(adapter->rasterImageView(), SIGNAL(currentToolChanged(AbstractRasterImageViewTool*)),
                             q, SIGNAL(currentToolChanged(AbstractRasterImageViewTool*)));
        }
    }

    void setupLoadingIndicator()
    {
        mLoadingIndicator = new LoadingIndicator(q);
        GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(q);
        floater->setChildWidget(mLoadingIndicator);
    }

    GraphicsHudButton* createHudButton(const QString& text, const char* iconName, bool showText)
    {
        GraphicsHudButton* button = new GraphicsHudButton;
        if (showText) {
            button->setText(text);
        } else {
            button->setToolTip(text);
        }
        button->setIcon(KIcon(iconName));
        return button;
    }

    void setupHud()
    {
        GraphicsHudButton* trashButton = createHudButton(i18nc("@info:tooltip", "Trash"), "user-trash", false);
        GraphicsHudButton* deselectButton = createHudButton(i18nc("@action:button", "Deselect"), "list-remove", true);

        QGraphicsWidget* content = new QGraphicsWidget;
        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(content);
        layout->addItem(trashButton);
        layout->addItem(deselectButton);

        mHud = new GraphicsHudWidget(q);
        mHud->init(content, GraphicsHudWidget::OptionNone);
        GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(q);
        floater->setChildWidget(mHud);
        floater->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

        QObject::connect(trashButton, SIGNAL(clicked()), q, SLOT(emitHudTrashClicked()));
        QObject::connect(deselectButton, SIGNAL(clicked()), q, SLOT(emitHudDeselectClicked()));

        mHud->hide();
    }

    void setupBirdEyeView()
    {
        if (mBirdEyeView) {
            delete mBirdEyeView;
        }
        mBirdEyeView = new BirdEyeView(q);
        mBirdEyeView->setZValue(1);
    }

    void updateCaption()
    {
        QString caption;

        Document::Ptr doc = mAdapter->document();
        if (!doc) {
            emit q->captionUpdateRequested(caption);
            return;
        }

        caption = doc->url().fileName();
        QSize size = doc->size();
        if (size.isValid()) {
            caption +=
                QString(" - %1x%2")
                .arg(size.width())
                .arg(size.height());
            if (mAdapter->canZoom()) {
                int intZoom = qRound(mAdapter->zoom() * 100);
                caption += QString(" - %1%")
                           .arg(intZoom);
            }
        }
        emit q->captionUpdateRequested(caption);
    }

    void uncheckZoomToFit()
    {
        if (mAdapter->zoomToFit()) {
            mAdapter->setZoomToFit(false);
        }
    }

    void setZoom(qreal zoom, const QPointF& center = QPointF(-1, -1))
    {
        uncheckZoomToFit();
        zoom = qBound(q->minimumZoom(), zoom, MAXIMUM_ZOOM_VALUE);
        mAdapter->setZoom(zoom, center);
    }

    void updateZoomSnapValues()
    {
        qreal min = q->minimumZoom();

        mZoomSnapValues.clear();
        if (min < 1.) {
            mZoomSnapValues << min;
            for (qreal invZoom = 16.; invZoom > 1.; invZoom /= 2.) {
                qreal zoom = 1. / invZoom;
                if (zoom > min) {
                    mZoomSnapValues << zoom;
                }
            }
        }
        for (qreal zoom = 1; zoom <= MAXIMUM_ZOOM_VALUE ; zoom += 1.) {
            mZoomSnapValues << zoom;
        }

        q->minimumZoomChanged(min);
    }

    void showLoadingIndicator()
    {
        if (!mLoadingIndicator) {
            setupLoadingIndicator();
        }
        mLoadingIndicator->show();
        mLoadingIndicator->setZValue(1);
    }

    void hideLoadingIndicator()
    {
        if (!mLoadingIndicator) {
            return;
        }
        mLoadingIndicator->hide();
    }

    void resizeAdapterWidget()
    {
        QRectF rect = QRectF(QPointF(0, 0), q->boundingRect().size());
        if (mCompareMode) {
            rect.adjust(COMPARE_MARGIN, COMPARE_MARGIN, -COMPARE_MARGIN, -COMPARE_MARGIN);
        }
        mAdapter->widget()->setGeometry(rect);
    }

    void fadeTo(qreal value)
    {
        if (mFadeAnimation.data()) {
            qreal endValue = mFadeAnimation.data()->endValue().toReal();
            if (qFuzzyCompare(value, endValue)) {
                // Same end value, don't change the actual animation
                return;
            }
        }
        // Create a new fade animation
        QPropertyAnimation* anim = new QPropertyAnimation(q, "opacity");
        anim->setStartValue(q->opacity());
        anim->setEndValue(value);
        if (qFuzzyCompare(value, 1)) {
            QObject::connect(anim, SIGNAL(finished()),
                            q, SLOT(slotFadeInFinished()));
        }
        QObject::connect(anim, SIGNAL(finished()), q, SIGNAL(isAnimatedChanged()));
        anim->setDuration(DocumentView::AnimDuration);
        mFadeAnimation = anim;
        q->isAnimatedChanged();
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
};

DocumentView::DocumentView(QGraphicsScene* scene)
: d(new DocumentViewPrivate)
{
    setFlag(ItemIsFocusable);
    setFlag(ItemIsSelectable);
    setFlag(ItemClipsChildrenToShape);

    d->q = this;
    d->mLoadingIndicator = 0;
    d->mBirdEyeView = 0;
    d->mCurrent = false;
    d->mCompareMode = false;
    d->mEraseBorders = false;

    setOpacity(0);

    scene->addItem(this);

    d->setupHud();
    d->setCurrentAdapter(new EmptyAdapter);
}

DocumentView::~DocumentView()
{
    delete d;
}

void DocumentView::createAdapterForDocument()
{
    const MimeTypeUtils::Kind documentKind = d->mDocument->kind();
    if (d->mAdapter && documentKind == d->mAdapter->kind() && documentKind != MimeTypeUtils::KIND_UNKNOWN) {
        // Do not reuse for KIND_UNKNOWN: we may need to change the message
        LOG("Reusing current adapter");
        return;
    }
    AbstractDocumentViewAdapter* adapter = 0;
    switch (documentKind) {
    case MimeTypeUtils::KIND_RASTER_IMAGE:
        adapter = new RasterImageViewAdapter;
        break;
    case MimeTypeUtils::KIND_SVG_IMAGE:
        adapter = new SvgViewAdapter;
        break;
    case MimeTypeUtils::KIND_VIDEO:
        adapter = new VideoViewAdapter;
        connect(adapter, SIGNAL(videoFinished()),
                SIGNAL(videoFinished()));
        break;
    case MimeTypeUtils::KIND_UNKNOWN:
        adapter = new MessageViewAdapter;
        static_cast<MessageViewAdapter*>(adapter)->setErrorMessage(i18n("Gwenview does not know how to display this kind of document"));
        break;
    default:
        kWarning() << "should not be called for documentKind=" << documentKind;
        adapter = new MessageViewAdapter;
        break;
    }

    d->setCurrentAdapter(adapter);
}

void DocumentView::openUrl(const KUrl& url, const DocumentView::Setup& setup)
{
    if (d->mDocument) {
        if (url == d->mDocument->url()) {
            return;
        }
        disconnect(d->mDocument.data(), 0, this, 0);
    }
    d->mSetup = setup;
    d->mDocument = DocumentFactory::instance()->load(url);
    connect(d->mDocument.data(), SIGNAL(busyChanged(KUrl,bool)), SLOT(slotBusyChanged(KUrl,bool)));

    if (d->mDocument->loadingState() < Document::KindDetermined) {
        MessageViewAdapter* messageViewAdapter = qobject_cast<MessageViewAdapter*>(d->mAdapter.data());
        if (messageViewAdapter) {
            messageViewAdapter->setInfoMessage(QString());
        }
        d->showLoadingIndicator();
        connect(d->mDocument.data(), SIGNAL(kindDetermined(KUrl)),
                SLOT(finishOpenUrl()));
    } else {
        QMetaObject::invokeMethod(this, "finishOpenUrl", Qt::QueuedConnection);
    }
    d->setupBirdEyeView();
}

void DocumentView::finishOpenUrl()
{
    disconnect(d->mDocument.data(), SIGNAL(kindDetermined(KUrl)),
               this, SLOT(finishOpenUrl()));
    GV_RETURN_IF_FAIL(d->mDocument->loadingState() >= Document::KindDetermined);

    if (d->mDocument->loadingState() == Document::LoadingFailed) {
        slotLoadingFailed();
        return;
    }
    createAdapterForDocument();

    connect(d->mDocument.data(), SIGNAL(loadingFailed(KUrl)),
            SLOT(slotLoadingFailed()));
    d->mAdapter->setDocument(d->mDocument);
    d->updateCaption();
}

void DocumentView::loadAdapterConfig()
{
    d->mAdapter->loadConfig();
}

RasterImageView* DocumentView::imageView() const
{
    return d->mAdapter->rasterImageView();
}

void DocumentView::slotCompleted()
{
    d->hideLoadingIndicator();
    d->updateCaption();
    d->updateZoomSnapValues();
    if (!d->mAdapter->zoomToFit()) {
        qreal min = minimumZoom();
        if (d->mAdapter->zoom() < min) {
            d->mAdapter->setZoom(min);
        }
    }
    emit completed();
}

DocumentView::Setup DocumentView::setup() const
{
    Setup setup;
    if (d->mAdapter->canZoom()) {
        setup.valid = true;
        setup.zoomToFit = zoomToFit();
        if (!setup.zoomToFit) {
            setup.zoom = zoom();
            setup.position = position();
        }
    }
    return setup;
}

void DocumentView::slotLoadingFailed()
{
    d->hideLoadingIndicator();
    MessageViewAdapter* adapter = new MessageViewAdapter;
    adapter->setDocument(d->mDocument);
    QString message = i18n("Loading <filename>%1</filename> failed", d->mDocument->url().fileName());
    adapter->setErrorMessage(message, d->mDocument->errorString());
    d->setCurrentAdapter(adapter);
    emit completed();
}

bool DocumentView::canZoom() const
{
    return d->mAdapter->canZoom();
}

void DocumentView::setZoomToFit(bool on)
{
    if (on == d->mAdapter->zoomToFit()) {
        return;
    }
    d->mAdapter->setZoomToFit(on);
    if (!on) {
        d->mAdapter->setZoom(1.);
    }
}

bool DocumentView::zoomToFit() const
{
    return d->mAdapter->zoomToFit();
}

void DocumentView::zoomActualSize()
{
    d->uncheckZoomToFit();
    d->mAdapter->setZoom(1.);
}

void DocumentView::zoomIn(const QPointF& center)
{
    qreal currentZoom = d->mAdapter->zoom();

    Q_FOREACH(qreal zoom, d->mZoomSnapValues) {
        if (zoom > currentZoom + REAL_DELTA) {
            d->setZoom(zoom, center);
            return;
        }
    }
}

void DocumentView::zoomOut(const QPointF& center)
{
    qreal currentZoom = d->mAdapter->zoom();

    QListIterator<qreal> it(d->mZoomSnapValues);
    it.toBack();
    while (it.hasPrevious()) {
        qreal zoom = it.previous();
        if (zoom < currentZoom - REAL_DELTA) {
            d->setZoom(zoom, center);
            return;
        }
    }
}

void DocumentView::slotZoomChanged(qreal zoom)
{
    d->updateCaption();
    zoomChanged(zoom);
}

void DocumentView::setZoom(qreal zoom)
{
    d->setZoom(zoom);
}

qreal DocumentView::zoom() const
{
    return d->mAdapter->zoom();
}

void DocumentView::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (d->mAdapter->canZoom() && event->modifiers() & Qt::ControlModifier) {
        // Ctrl + wheel => zoom in or out
        if (event->delta() > 0) {
            zoomIn(event->pos());
        } else {
            zoomOut(event->pos());
        }
        return;
    }
    if (GwenviewConfig::mouseWheelBehavior() == MouseWheelBehavior::Browse
        && event->modifiers() == Qt::NoModifier) {
        // Browse with mouse wheel
        if (event->delta() > 0) {
            previousImageRequested();
        } else {
            nextImageRequested();
        }
        return;
    }
    // Scroll
    qreal dx = 0;
    // 16 = pixels for one line
    // 120: see QWheelEvent::delta() doc
    qreal dy = -qApp->wheelScrollLines() * 16 * event->delta() / 120;
    if (event->orientation() == Qt::Horizontal) {
        qSwap(dx, dy);
    }
    d->mAdapter->setScrollPos(d->mAdapter->scrollPos() + QPointF(dx, dy));
}

void DocumentView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Filter out context menu if Ctrl is down to avoid showing it when
    // zooming out with Ctrl + Right button
    if (event->modifiers() != Qt::ControlModifier) {
        contextMenuRequested();
    }
}

void DocumentView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    QRectF visibleRect = mapRectFromItem(d->mAdapter->widget(), d->mAdapter->visibleDocumentRect());
    if (d->mEraseBorders) {
        QRegion borders = QRegion(boundingRect().toRect())
            - QRegion(visibleRect.toRect());
        Q_FOREACH(const QRect& rect, borders.rects()) {
            painter->eraseRect(rect);
        }
    }

    if (d->mCompareMode && d->mCurrent) {
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(palette().highlight().color(), 2));
        painter->setRenderHint(QPainter::Antialiasing);
        QRectF selectionRect = visibleRect.adjusted(-2, -2, 2, 2);
        painter->drawRoundedRect(selectionRect, 3, 3);
        painter->restore();
    }
}

void DocumentView::slotBusyChanged(const KUrl&, bool busy)
{
    if (busy) {
        d->showLoadingIndicator();
    } else {
        d->hideLoadingIndicator();
    }
}

qreal DocumentView::minimumZoom() const
{
    // There is no point zooming out less than zoomToFit, but make sure it does
    // not get too small either
    return qBound(qreal(0.001), d->mAdapter->computeZoomToFit(), qreal(1.));
}

void DocumentView::setCompareMode(bool compare)
{
    d->mCompareMode = compare;
    d->resizeAdapterWidget();
    if (compare) {
        d->mHud->show();
        d->mHud->setZValue(1);
    } else {
        d->mHud->hide();
    }
}

void DocumentView::setCurrent(bool value)
{
    d->mCurrent = value;
    if (value) {
        d->mAdapter->widget()->setFocus();
    }
    update();
}

bool DocumentView::isCurrent() const
{
    return d->mCurrent;
}

QPoint DocumentView::position() const
{
    return d->mAdapter->scrollPos().toPoint();
}

void DocumentView::setPosition(const QPoint& pos)
{
    d->mAdapter->setScrollPos(pos);
}

Document::Ptr DocumentView::document() const
{
    return d->mDocument;
}

KUrl DocumentView::url() const
{
    Document::Ptr doc = d->mDocument;
    return doc ? doc->url() : KUrl();
}

void DocumentView::emitHudDeselectClicked()
{
    hudDeselectClicked(this);
}

void DocumentView::emitHudTrashClicked()
{
    hudTrashClicked(this);
}

void DocumentView::emitFocused()
{
    focused(this);
}

void DocumentView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);
    d->resizeAdapterWidget();
    if (d->mBirdEyeView) {
        d->mBirdEyeView->slotZoomOrSizeChanged();
    }
}

void DocumentView::moveTo(const QRect& rect)
{
    if (d->mMoveAnimation) {
        d->mMoveAnimation.data()->setEndValue(rect);
    } else {
        setGeometry(rect);
    }
}

void DocumentView::moveToAnimated(const QRect& rect)
{
    QPropertyAnimation* anim = new QPropertyAnimation(this, "geometry");
    anim->setStartValue(geometry());
    anim->setEndValue(rect);
    anim->setDuration(DocumentView::AnimDuration);
    connect(anim, SIGNAL(finished()), SIGNAL(isAnimatedChanged()));
    d->mMoveAnimation = anim;
    isAnimatedChanged();
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation* DocumentView::fadeIn()
{
    d->fadeTo(1);
    return d->mFadeAnimation.data();
}

void DocumentView::fadeOut()
{
    d->fadeTo(0);
}

void DocumentView::slotFadeInFinished()
{
    fadeInFinished(this);
}

bool DocumentView::isAnimated() const
{
    return d->mMoveAnimation || d->mFadeAnimation;
}

bool DocumentView::sceneEventFilter(QGraphicsItem*, QEvent* event)
{
    if (event->type() == QEvent::GraphicsSceneMousePress) {
        QMetaObject::invokeMethod(this, "emitFocused", Qt::QueuedConnection);
    } else if (event->type() == QEvent::GraphicsSceneHoverMove) {
        if (d->mBirdEyeView) {
            d->mBirdEyeView->onMouseMoved();
        }
    }
    return false;
}

AbstractRasterImageViewTool* DocumentView::currentTool() const
{
    return imageView() ? imageView()->currentTool() : 0;
}

int DocumentView::sortKey() const
{
    return d->mSortKey;
}

void DocumentView::setSortKey(int sortKey)
{
    d->mSortKey = sortKey;
}

void DocumentView::setEraseBorders(bool value)
{
    d->mEraseBorders = value;
}

void DocumentView::hideAndDeleteLater()
{
    hide();
    deleteLater();
}

} // namespace
