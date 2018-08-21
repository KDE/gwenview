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
#include "redeyereductiontool.h"

// Qt
#include <QDialogButtonBox>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRect>
#include <QDebug>

// KDE

// Local
#include <lib/documentview/rasterimageview.h>
#include "gwenviewconfig.h"
#include "paintutils.h"
#include "redeyereductionimageoperation.h"
#include "ui_redeyereductionwidget.h"

namespace Gwenview
{

struct RedEyeReductionWidget : public QWidget, public Ui_RedEyeReductionWidget
{
    RedEyeReductionWidget()
    {
        setupUi(this);
    }

    void showNotSetPage()
    {
        dialogButtonBox->button(QDialogButtonBox::Ok)->hide();
        stackedWidget->setCurrentWidget(notSetPage);
    }

    void showMainPage()
    {
        dialogButtonBox->button(QDialogButtonBox::Ok)->show();
        stackedWidget->setCurrentWidget(mainPage);
    }
};

struct RedEyeReductionToolPrivate
{
    RedEyeReductionTool* q;
    RedEyeReductionTool::Status mStatus;
    QPointF mCenter;
    int mDiameter;
    RedEyeReductionWidget* mToolWidget;

    void setupToolWidget()
    {
        mToolWidget = new RedEyeReductionWidget;
        mToolWidget->showNotSetPage();
        QObject::connect(mToolWidget->diameterSpinBox, SIGNAL(valueChanged(int)),
                         q, SLOT(setDiameter(int)));
        QObject::connect(mToolWidget->dialogButtonBox, SIGNAL(accepted()),
                         q, SLOT(slotApplyClicked()));
        QObject::connect(mToolWidget->dialogButtonBox, SIGNAL(rejected()),
                         q, SIGNAL(done()));
    }

    QRectF rectF() const
    {
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
    d->q = this;
    d->mDiameter = GwenviewConfig::redEyeReductionDiameter();
    d->mStatus = NotSet;
    d->setupToolWidget();

    view->document()->startLoadingFullImage();
}

RedEyeReductionTool::~RedEyeReductionTool()
{
    GwenviewConfig::setRedEyeReductionDiameter(d->mDiameter);
    delete d->mToolWidget;
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
    if (event->buttons() != Qt::LeftButton
        || event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }
    event->accept();
    if (d->mStatus == NotSet) {
        d->mToolWidget->diameterSpinBox->setValue(d->mDiameter);
        d->mToolWidget->showMainPage();
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

void RedEyeReductionTool::keyPressEvent(QKeyEvent* event)
{
    QDialogButtonBox *buttons = d->mToolWidget->findChild<QDialogButtonBox *>();
    switch (event->key()) {
    case Qt::Key_Escape:
        event->accept();
        buttons->rejected();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        event->accept();
        auto focusButton = static_cast<QPushButton*>(buttons->focusWidget());
        if (focusButton && buttons->buttonRole(focusButton) == QDialogButtonBox::RejectRole) {
            buttons->rejected();
        } else {
            buttons->accepted();
        }
        break;
    }
    default:
        break;
    }
}

void RedEyeReductionTool::toolActivated()
{
    imageView()->setCursor(Qt::CrossCursor);
}

void RedEyeReductionTool::slotApplyClicked()
{
    QRectF docRectF = d->rectF();
    if (!docRectF.isValid()) {
        qWarning() << "invalid rect";
        return;
    }
    RedEyeReductionImageOperation* op = new RedEyeReductionImageOperation(docRectF);
    emit imageOperationRequested(op);

    d->mStatus = NotSet;
    d->mToolWidget->showNotSetPage();
}

void RedEyeReductionTool::setDiameter(int value)
{
    d->mDiameter = value;
    imageView()->update();
}

QWidget* RedEyeReductionTool::widget() const
{
    return d->mToolWidget;
}

} // namespace
