// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "redeyereductiontool.moc"

// Qt
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QToolButton>
#include <QRect>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include <lib/documentview/rasterimageview.h>
#include <lib/graphicswidgetfloater.h>
#include <lib/graphicshudwidget.h>
#include "gwenviewconfig.h"
#include "paintutils.h"
#include "redeyereductionimageoperation.h"
#include "ui_redeyereductionhud.h"

namespace Gwenview
{

struct RedEyeReductionHud : public QWidget, public Ui_RedEyeReductionHud {
    RedEyeReductionHud() {
        setupUi(this);
        setCursor(Qt::ArrowCursor);
    }
};

struct RedEyeReductionToolPrivate {
    RedEyeReductionTool* mRedEyeReductionTool;
    RedEyeReductionTool::Status mStatus;
    QPointF mCenter;
    int mDiameter;
    GraphicsHudWidget* mHudWidget;
    GraphicsWidgetFloater* mFloater;

    void showNotSetHudWidget()
    {
        QLabel* label = new QLabel(i18n("Click on the red eye you want to fix."));
        label->show();
        label->adjustSize();
        createHudWidgetForWidget(label);
    }

    void showAdjustingHudWidget()
    {
        RedEyeReductionHud* hud = new RedEyeReductionHud();

        hud->diameterSpinBox->setValue(mDiameter);
        QObject::connect(hud->applyButton, SIGNAL(clicked()),
                         mRedEyeReductionTool, SLOT(slotApplyClicked()));
        QObject::connect(hud->diameterSpinBox, SIGNAL(valueChanged(int)),
                         mRedEyeReductionTool, SLOT(setDiameter(int)));

        createHudWidgetForWidget(hud);
    }

    void createHudWidgetForWidget(QWidget* widget)
    {
        mHudWidget->deleteLater();
        mHudWidget = new GraphicsHudWidget(mRedEyeReductionTool->imageView());
        mHudWidget->init(widget, GraphicsHudWidget::OptionCloseButton);
        mHudWidget->adjustSize();
        QObject::connect(mHudWidget, SIGNAL(closed()),
                         mRedEyeReductionTool, SIGNAL(done()));
        mFloater->setChildWidget(mHudWidget);
    }

    void hideHud()
    {
        mHudWidget->hide();
    }

    QRectF rectF() const {
        if (mStatus == RedEyeReductionTool::NotSet) {
            return QRectF();
        }
        return QRectF(mCenter.x() - mDiameter / 2, mCenter.y() - mDiameter / 2, mDiameter, mDiameter);
    }
};

RedEyeReductionTool::RedEyeReductionTool(RasterImageView* view)
: AbstractRasterImageViewTool(view)
, d(new RedEyeReductionToolPrivate)
{
    d->mRedEyeReductionTool = this;
    d->mDiameter = GwenviewConfig::redEyeReductionDiameter();
    d->mStatus = NotSet;
    d->mHudWidget = 0;

    d->mFloater = new GraphicsWidgetFloater(imageView());
    d->mFloater->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    d->mFloater->setVerticalMargin(
        KDialog::marginHint()
        + imageView()->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
    );
    d->showNotSetHudWidget();

    view->document()->startLoadingFullImage();
}

RedEyeReductionTool::~RedEyeReductionTool()
{
    GwenviewConfig::setRedEyeReductionDiameter(d->mDiameter);
    delete d;
}

void RedEyeReductionTool::paint(QPainter* painter)
{
    if (d->mStatus == NotSet) {
        return;
    }
    QRectF docRectF = d->rectF();
    imageView()->document()->waitUntilLoaded();

    QRect docRect = PaintUtils::containingRect(docRectF);
    QImage img = imageView()->document()->image().copy(docRect);
    QRectF imgRectF(
        docRectF.left() - docRect.left(),
        docRectF.top()  - docRect.top(),
        docRectF.width(),
        docRectF.height()
    );
    RedEyeReductionImageOperation::apply(&img, imgRectF);

    const QRectF viewRectF = imageView()->mapToView(docRectF);
    painter->drawImage(viewRectF, img, imgRectF);
}

void RedEyeReductionTool::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    if (d->mStatus == NotSet) {
        d->showAdjustingHudWidget();
        d->mStatus = Adjusting;
    }
    d->mCenter = imageView()->mapToImage(event->pos());
    imageView()->update();
}

void RedEyeReductionTool::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    if (event->buttons() == Qt::NoButton) {
        return;
    }
    d->mCenter = imageView()->mapToImage(event->pos());
    imageView()->update();
}

void RedEyeReductionTool::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    // Just prevent the event from reaching the image view
    event->accept();
}

void RedEyeReductionTool::toolActivated()
{
    imageView()->setCursor(Qt::CrossCursor);
}

void RedEyeReductionTool::toolDeactivated()
{
    d->mHudWidget->deleteLater();
}

void RedEyeReductionTool::slotApplyClicked()
{
    QRectF docRectF = d->rectF();
    if (!docRectF.isValid()) {
        kWarning() << "invalid rect";
        return;
    }
    RedEyeReductionImageOperation* op = new RedEyeReductionImageOperation(docRectF);
    emit imageOperationRequested(op);

    d->mStatus = NotSet;
    d->showNotSetHudWidget();
}

void RedEyeReductionTool::setDiameter(int value)
{
    d->mDiameter = value;
    imageView()->update();
}

} // namespace
