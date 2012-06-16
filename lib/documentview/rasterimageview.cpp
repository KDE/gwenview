// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>
Copyright 2012 Martin Gräßlin <mgraesslin@kde.org>

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
#include "rasterimageview.moc"

// Local
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/imagescaler.h>

// KDE
#include <KDebug>

// Qt
#include <QGraphicsSceneMouseEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QTimer>
#include <QWeakPointer>
// OpenGL
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLShaderProgram>

namespace Gwenview
{

struct RasterImageViewPrivate
{
    RasterImageView* q;
    ImageScaler* mScaler;
    QPixmap mBackgroundTexture;
    bool mEmittedCompleted;

    // Config
    RasterImageView::AlphaBackgroundMode mAlphaBackgroundMode;
    QColor mAlphaBackgroundColor;
    bool mEnlargeSmallerImages;
    // /Config

    bool mBufferIsEmpty;
    QPixmap mCurrentBuffer;
    // The alternate buffer is useful when scrolling: existing content is copied
    // to mAlternateBuffer and buffers are swapped. This avoids allocating a new
    // QPixmap every time the image is scrolled.
    QPixmap mAlternateBuffer;

    QTimer* mUpdateTimer;

    QWeakPointer<AbstractRasterImageViewTool> mTool;
    QWeakPointer<QGLShaderProgram> mShader;
    GLuint mTexture;
    /**
     * @c true if OpenGL has been initialized.
     * Used to determine whether we have to create shaders.
     **/
    bool mOpenGLInitialized;
    /**
     * @c true if OpenGL has been initialized correctly.
     * @c false otherwise and if OpenGL has not been initialized yet.
     * Use @link mOpenGLInitialized to determine whether OpenGL is initialized.
     **/
    bool mOpenGLValid;

    void createBackgroundTexture()
    {
        mBackgroundTexture = QPixmap(32, 32);
        QPainter painter(&mBackgroundTexture);
        painter.fillRect(mBackgroundTexture.rect(), QColor(128, 128, 128));
        QColor light = QColor(192, 192, 192);
        painter.fillRect(0, 0, 16, 16, light);
        painter.fillRect(16, 16, 16, 16, light);
    }

    void setupUpdateTimer()
    {
        mUpdateTimer = new QTimer(q);
        mUpdateTimer->setInterval(500);
        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, SIGNAL(timeout()),
                         q, SLOT(updateBuffer()));
    }

    void startAnimationIfNecessary()
    {
        if (q->document() && q->isVisible()) {
            q->document()->startAnimation();
        }
    }

    QRectF mapViewportToZoomedImage(const QRectF& viewportRect) const
    {
        return QRectF(
                   viewportRect.topLeft() - q->imageOffset() + q->scrollPos(),
                   viewportRect.size()
               );
    }

    void setScalerRegionToVisibleRect()
    {
        QRectF rect = mapViewportToZoomedImage(q->boundingRect());
        mScaler->setDestinationRegion(QRegion(rect.toRect()));
    }

    void resizeBuffer()
    {
        QSize size = q->visibleImageSize().toSize();
        if (size == mCurrentBuffer.size()) {
            return;
        }
        if (!size.isValid()) {
            mAlternateBuffer = QPixmap();
            mCurrentBuffer = QPixmap();
            return;
        }

        mAlternateBuffer = QPixmap(size);
        mAlternateBuffer.fill(Qt::transparent);
        {
            QPainter painter(&mAlternateBuffer);
            painter.drawPixmap(0, 0, mCurrentBuffer);
        }
        qSwap(mAlternateBuffer, mCurrentBuffer);

        mAlternateBuffer = QPixmap();
    }

    void drawAlphaBackground(QPainter* painter, const QRect& viewportRect, const QPoint& zoomedImageTopLeft)
    {
        if (mAlphaBackgroundMode == RasterImageView::AlphaBackgroundCheckBoard) {
            QPoint textureOffset(
                zoomedImageTopLeft.x() % mBackgroundTexture.width(),
                zoomedImageTopLeft.y() % mBackgroundTexture.height()
            );
            painter->drawTiledPixmap(
                viewportRect,
                mBackgroundTexture,
                textureOffset);
        } else {
            painter->fillRect(viewportRect, mAlphaBackgroundColor);
        }
    }
};

RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate)
{
    d->q = this;
    d->mEmittedCompleted = false;

    d->mAlphaBackgroundMode = AlphaBackgroundCheckBoard;
    d->mAlphaBackgroundColor = Qt::black;
    d->mEnlargeSmallerImages = false;

    d->mBufferIsEmpty = true;
    d->mScaler = new ImageScaler(this);
    connect(d->mScaler, SIGNAL(scaledRect(int, int, QImage)),
            SLOT(updateFromScaler(int, int, QImage)));

    // OpenGL
    d->mTexture = 0;
    d->mOpenGLInitialized = false;
    d->mOpenGLValid = false;

    d->createBackgroundTexture();
    d->setupUpdateTimer();
}

RasterImageView::~RasterImageView()
{
    const_cast<QGLContext*>(QGLContext::currentContext())->deleteTexture(d->mTexture);
    delete d;
}

void RasterImageView::setAlphaBackgroundMode(AlphaBackgroundMode mode)
{
    d->mAlphaBackgroundMode = mode;
    if (document() && document()->hasAlphaChannel()) {
        d->mCurrentBuffer = QPixmap();
        updateBuffer();
    }
}

void RasterImageView::setAlphaBackgroundColor(const QColor& color)
{
    d->mAlphaBackgroundColor = color;
    if (document() && document()->hasAlphaChannel()) {
        d->mCurrentBuffer = QPixmap();
        updateBuffer();
    }
}

void RasterImageView::loadFromDocument()
{
    Document::Ptr doc = document();
    connect(doc.data(), SIGNAL(metaInfoLoaded(KUrl)),
            SLOT(slotDocumentMetaInfoLoaded()));
    connect(doc.data(), SIGNAL(isAnimatedUpdated()),
            SLOT(slotDocumentIsAnimatedUpdated()));

    const Document::LoadingState state = doc->loadingState();
    if (state == Document::MetaInfoLoaded || state == Document::Loaded) {
        slotDocumentMetaInfoLoaded();
    }
}

void RasterImageView::slotDocumentMetaInfoLoaded()
{
    if (document()->size().isValid()) {
        finishSetDocument();
    } else {
        // Could not retrieve image size from meta info, we need to load the
        // full image now.
        connect(document().data(), SIGNAL(loaded(KUrl)),
                SLOT(finishSetDocument()));
        document()->startLoadingFullImage();
    }
}

void RasterImageView::finishSetDocument()
{
    if (!document()->size().isValid()) {
        kError() << "No valid image size available, this should not happen!";
        return;
    }

    d->resizeBuffer();
    // TODO: proper way to differentiate between OpenGL and no GL
#if 0
    d->mScaler->setDocument(document());
#else
    document()->startLoadingFullImage();
#endif

    connect(document().data(), SIGNAL(imageRectUpdated(QRect)),
            SLOT(updateImageRect(QRect)));

    if (zoomToFit()) {
        // Force the update otherwise if computeZoomToFit() returns 1, setZoom()
        // will think zoom has not changed and won't update the image
        setZoom(computeZoomToFit(), QPointF(-1, -1), ForceUpdate);
    } else {
        QRect rect(QPoint(0, 0), document()->size());
        updateImageRect(rect);
    }

    d->startAnimationIfNecessary();
    update();
}

void RasterImageView::updateImageRect(const QRect& imageRect)
{
    QRectF viewRect = mapToView(imageRect);
    if (!viewRect.intersects(boundingRect())) {
        return;
    }

    if (zoomToFit()) {
        setZoom(computeZoomToFit());
    }
    d->setScalerRegionToVisibleRect();
    update();
}

void RasterImageView::slotDocumentIsAnimatedUpdated()
{
    d->startAnimationIfNecessary();
}

void RasterImageView::updateFromScaler(int zoomedImageLeft, int zoomedImageTop, const QImage& image)
{
    d->resizeBuffer();
    int viewportLeft = zoomedImageLeft - scrollPos().x();
    int viewportTop = zoomedImageTop - scrollPos().y();
    d->mBufferIsEmpty = false;
    {
        QPainter painter(&d->mCurrentBuffer);
        if (document()->hasAlphaChannel()) {
            d->drawAlphaBackground(
                &painter, QRect(viewportLeft, viewportTop, image.width(), image.height()),
                QPoint(zoomedImageLeft, zoomedImageTop)
            );
        } else {
            painter.setCompositionMode(QPainter::CompositionMode_Source);
        }
        painter.drawImage(viewportLeft, viewportTop, image);
    }
    update();

    if (!d->mEmittedCompleted) {
        d->mEmittedCompleted = true;
        completed();
    }
}

void RasterImageView::onZoomChanged()
{
    // If we zoom more than twice, then assume the user wants to see the real
    // pixels, for example to fine tune a crop operation
    if (zoom() < 2.) {
        d->mScaler->setTransformationMode(Qt::SmoothTransformation);
    } else {
        d->mScaler->setTransformationMode(Qt::FastTransformation);
    }
    if (!d->mUpdateTimer->isActive()) {
        updateBuffer();
    }
}

void RasterImageView::onImageOffsetChanged()
{
    update();
}

void RasterImageView::onScrollPosChanged(const QPointF& oldPos)
{
    QPointF delta = scrollPos() - oldPos;

    // Scroll existing
    {
        if (d->mAlternateBuffer.size() != d->mCurrentBuffer.size()) {
            d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
        }
        QPainter painter(&d->mAlternateBuffer);
        painter.drawPixmap(-delta, d->mCurrentBuffer);
    }
    qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

    // Scale missing parts
    QRegion bufferRegion = QRegion(d->mCurrentBuffer.rect().translated(scrollPos().toPoint()));
    QRegion updateRegion = bufferRegion - bufferRegion.translated(-delta.toPoint());
    updateBuffer(updateRegion);
    update();
}

bool RasterImageView::paintGL(QPainter* painter)
{
    // TODO: find better way to figure out whether we are on OpenGL
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL && painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        return false;
    }
    if (d->mOpenGLInitialized && !d->mOpenGLValid) {
        // OpenGL not functional
        return false;
    }
    painter->beginNativePainting();
    if (d->mShader.isNull() || !d->mOpenGLInitialized) {
        d->mOpenGLInitialized = true;
        // shader not yet created
        d->mShader = new QGLShaderProgram(QGLContext::currentContext(), this);
        const QByteArray vertexShader(
"uniform mat4 modelviewProjection;\n"
"attribute vec4 vertex;\n"
"varying vec2 texCoords;\n"
"void main() {\n"
"    texCoords = vertex.zw;\n"
"    gl_Position = modelviewProjection*vec4(vertex.xy, 0.0, 1.0);\n"
"}");
        const QByteArray fragmentShader(
"uniform sampler2D texture;\n"
"varying vec2 texCoords;\n"
"void main() {\n"
"    gl_FragColor = texture2D(texture, texCoords);\n"
"}");
        if (!d->mShader.data()->addShaderFromSourceCode(QGLShader::Vertex, vertexShader)) {
            kDebug() << d->mShader.data()->log();
            return false;
        }
        if (!d->mShader.data()->addShaderFromSourceCode(QGLShader::Fragment, fragmentShader)) {
            kDebug() << d->mShader.data()->log();
            return false;
        }
        if (!d->mShader.data()->link()) {
            kDebug() << d->mShader.data()->log();
            return false;
        }
        int textureLocation = d->mShader.data()->uniformLocation("texture");
        d->mShader.data()->setUniformValue(textureLocation, 0);
        // everything initialized correctly
        d->mOpenGLValid = true;
    }

    const qreal width = documentSize().width();
    const qreal height = documentSize().height();
    // each vertex consists of x/y and texX/texY
    float vertices[16] = {
        width, 0.0, 1.0, 1.0,
        0.0, 0.0, 0.0, 1.0,
        0.0, height, 0.0, 0.0,
        width, height, 1.0, 0.0
    };

    if (!d->mShader.data()->bind()) {
        // that's bad, better disable OpenGL
        d->mOpenGLValid = false;
        return false;
    }
    if (!d->mTexture) {
        d->mTexture = const_cast<QGLContext*>(QGLContext::currentContext())->bindTexture(document()->image());
    }
    int vertexLocation = d->mShader.data()->attributeLocation("vertex");
    int matrixLocation = d->mShader.data()->uniformLocation("modelviewProjection");
    glBindTexture(GL_TEXTURE_2D, d->mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_BLEND);
    QMatrix4x4 modelviewProjection;
    modelviewProjection.ortho(x(), size().width(), size().height(), y(), 0, 65535);
    modelviewProjection.translate(imageOffset().x(), imageOffset().y());
    modelviewProjection.translate(-scrollPos().x(), -scrollPos().y());
    modelviewProjection.scale(zoom(), zoom());
    glViewport(x(), y(), size().width(), size().height());
    d->mShader.data()->setUniformValue(matrixLocation, modelviewProjection);
    d->mShader.data()->enableAttributeArray(vertexLocation);
    d->mShader.data()->setAttributeArray(vertexLocation, vertices, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    d->mShader.data()->disableAttributeArray(vertexLocation);
    painter->endNativePainting();
    return true;
}

void RasterImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    if (paintGL(painter)) {
        if (d->mTool) {
            d->mTool.data()->paint(painter);
        }
        return;
    }

    QPointF topLeft = imageOffset();
    if (zoomToFit()) {
        // In zoomToFit mode, scale crudely the buffer to fit the screen. This
        // provide an approximate rendered which will be replaced when the scheduled
        // proper scale is ready.
        QSizeF size = documentSize() * zoom();
        painter->drawPixmap(topLeft.x(), topLeft.y(), size.width(), size.height(), d->mCurrentBuffer);
    } else {
        painter->drawPixmap(topLeft, d->mCurrentBuffer);
    }

    if (d->mTool) {
        d->mTool.data()->paint(painter);
    }

    // Debug
#if 0
    QSizeF visibleSize = documentSize() * zoom();
    painter->setPen(Qt::red);
    painter->drawRect(topLeft.x(), topLeft.y(), visibleSize.width() - 1, visibleSize.height() - 1);

    painter->setPen(Qt::blue);
    painter->drawRect(topLeft.x(), topLeft.y(), d->mCurrentBuffer.width() - 1, d->mCurrentBuffer.height() - 1);
#endif
}

void RasterImageView::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    // If we are in zoomToFit mode and have something in our buffer, delay the
    // update: paint() will paint a scaled version of the buffer until resizing
    // is done. This is much faster than rescaling the whole image for each
    // resize event we receive.
    // mUpdateTimer must be started before calling AbstractImageView::resizeEvent()
    // because AbstractImageView::resizeEvent() will call onZoomChanged(), which
    // will trigger an immediate update unless the mUpdateTimer is active.
    if (zoomToFit() && !d->mBufferIsEmpty) {
        d->mUpdateTimer->start();
    }
    AbstractImageView::resizeEvent(event);
    if (!zoomToFit()) {
        // Only update buffer if we are not in zoomToFit mode: if we are
        // onZoomChanged() will have already updated the buffer.
        updateBuffer();
    }
}

void RasterImageView::updateBuffer(const QRegion& region)
{
    d->mUpdateTimer->stop();
    d->mScaler->setZoom(zoom());
    if (region.isEmpty()) {
        d->setScalerRegionToVisibleRect();
    } else {
        d->mScaler->setDestinationRegion(region);
    }
}

void RasterImageView::setCurrentTool(AbstractRasterImageViewTool* tool)
{
    if (d->mTool) {
        d->mTool.data()->toolDeactivated();
        d->mTool.data()->deleteLater();
    }
    d->mTool = tool;
    if (d->mTool) {
        d->mTool.data()->toolActivated();
    }
    updateCursor();
    currentToolChanged(tool);
    update();
}

AbstractRasterImageViewTool* RasterImageView::currentTool() const
{
    return d->mTool.data();
}

void RasterImageView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mousePressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mousePressEvent(event);
}

void RasterImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mouseMoveEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseMoveEvent(event);
}

void RasterImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mouseReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseReleaseEvent(event);
}

void RasterImageView::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->wheelEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::wheelEvent(event);
}

void RasterImageView::keyPressEvent(QKeyEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->keyPressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyPressEvent(event);
}

void RasterImageView::keyReleaseEvent(QKeyEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->keyReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyReleaseEvent(event);
}

void RasterImageView::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->hoverMoveEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::hoverMoveEvent(event);
}

} // namespace
