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
#include "imageopscontextmanageritem.h"
#include "dialogguard.h"

// Qt
#include <QApplication>
#include <QAction>
#include <QDebug>

// KDE
#include <KLocalizedString>
#include <KMessageBox>
#include <KActionCollection>
#include <KActionCategory>

// Local
#include "viewmainpage.h"
#include "gvcore.h"
#include "mainwindow.h"
#include "sidebar.h"
#include <lib/contextmanager.h>
#include <lib/crop/croptool.h>
#include <lib/document/documentfactory.h>
#include <lib/documentview/rasterimageview.h>
#include <lib/eventwatcher.h>
#include <lib/redeyereduction/redeyereductiontool.h>
#include <lib/gwenviewconfig.h>
#include <lib/resize/resizeimageoperation.h>
#include <lib/resize/resizeimagedialog.h>
#include <lib/transformimageoperation.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qDebug() << x
#else
#define LOG(x) ;
#endif

struct ImageOpsContextManagerItem::Private
{
    ImageOpsContextManagerItem* q;
    MainWindow* mMainWindow;
    SideBarGroup* mGroup;

    QAction * mRotateLeftAction;
    QAction * mRotateRightAction;
    QAction * mMirrorAction;
    QAction * mFlipAction;
    QAction * mResizeAction;
    QAction * mCropAction;
    QAction * mRedEyeReductionAction;
    QList<QAction *> mActionList;

    void setupActions()
    {
        KActionCollection* actionCollection = mMainWindow->actionCollection();
        KActionCategory* edit = new KActionCategory(i18nc("@title actions category - means actions changing image", "Edit"), actionCollection);

        mRotateLeftAction = edit->addAction("rotate_left", q, SLOT(rotateLeft()));
        mRotateLeftAction->setText(i18n("Rotate Left"));
        mRotateLeftAction->setToolTip(i18nc("@info:tooltip", "Rotate image to the left"));
        mRotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left"));
        actionCollection->setDefaultShortcut(mRotateLeftAction, Qt::CTRL + Qt::Key_L);

        mRotateRightAction = edit->addAction("rotate_right", q, SLOT(rotateRight()));
        mRotateRightAction->setText(i18n("Rotate Right"));
        mRotateRightAction->setToolTip(i18nc("@info:tooltip", "Rotate image to the right"));
        mRotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right"));
        actionCollection->setDefaultShortcut(mRotateRightAction, Qt::CTRL + Qt::Key_R);

        mMirrorAction = edit->addAction("mirror", q, SLOT(mirror()));
        mMirrorAction->setText(i18n("Mirror"));
        mMirrorAction->setIcon(QIcon::fromTheme("object-flip-horizontal"));

        mFlipAction = edit->addAction("flip", q, SLOT(flip()));
        mFlipAction->setText(i18n("Flip"));
        mFlipAction->setIcon(QIcon::fromTheme("object-flip-vertical"));

        mResizeAction = edit->addAction("resize", q, SLOT(resizeImage()));
        mResizeAction->setText(i18n("Resize"));
        mResizeAction->setIcon(QIcon::fromTheme("transform-scale"));
        actionCollection->setDefaultShortcut(mResizeAction, Qt::SHIFT + Qt::Key_R);

        mCropAction = edit->addAction("crop", q, SLOT(crop()));
        mCropAction->setText(i18n("Crop"));
        mCropAction->setIcon(QIcon::fromTheme("transform-crop-and-resize"));
        actionCollection->setDefaultShortcut(mCropAction, Qt::SHIFT + Qt::Key_C);

        mRedEyeReductionAction = edit->addAction("red_eye_reduction", q, SLOT(startRedEyeReduction()));
        mRedEyeReductionAction->setText(i18n("Red Eye Reduction"));
        //mRedEyeReductionAction->setIcon(QIcon::fromTheme("transform-crop-and-resize"));

        mActionList
                << mRotateLeftAction
                << mRotateRightAction
                << mMirrorAction
                << mFlipAction
                << mResizeAction
                << mCropAction
                << mRedEyeReductionAction
                ;
    }

    bool ensureEditable()
    {
        QUrl url = q->contextManager()->currentUrl();
        Document::Ptr doc = DocumentFactory::instance()->load(url);
        doc->startLoadingFullImage();
        doc->waitUntilLoaded();
        if (doc->isEditable()) {
            return true;
        }

        KMessageBox::sorry(
            QApplication::activeWindow(),
            i18nc("@info", "Gwenview cannot edit this kind of image.")
        );
        return false;
    }
};

ImageOpsContextManagerItem::ImageOpsContextManagerItem(ContextManager* manager, MainWindow* mainWindow)
: AbstractContextManagerItem(manager)
, d(new Private)
{
    d->q = this;
    d->mMainWindow = mainWindow;
    d->mGroup = new SideBarGroup(i18n("Image Operations"));
    setWidget(d->mGroup);
    EventWatcher::install(d->mGroup, QEvent::Show, this, SLOT(updateSideBarContent()));
    d->setupActions();
    updateActions();
    connect(contextManager(), SIGNAL(selectionChanged()),
            SLOT(updateActions()));
    connect(mainWindow, SIGNAL(viewModeChanged()),
            SLOT(updateActions()));
    connect(mainWindow->viewMainPage(), SIGNAL(completed()),
            SLOT(updateActions()));
}

ImageOpsContextManagerItem::~ImageOpsContextManagerItem()
{
    delete d;
}

void ImageOpsContextManagerItem::updateSideBarContent()
{
    if (!d->mGroup->isVisible()) {
        return;
    }

    d->mGroup->clear();
    Q_FOREACH(QAction * action, d->mActionList) {
        if (action->isEnabled() && action->priority() != QAction::LowPriority) {
            d->mGroup->addAction(action);
        }
    }
}

void ImageOpsContextManagerItem::updateActions()
{
    bool canModify = contextManager()->currentUrlIsRasterImage();
    bool viewMainPageIsVisible = d->mMainWindow->viewMainPage()->isVisible();
    if (!viewMainPageIsVisible) {
        // Since we only support image operations on one image for now,
        // disable actions if several images are selected and the document
        // view is not visible.
        if (contextManager()->selectedFileItemList().count() != 1) {
            canModify = false;
        }
    }

    d->mRotateLeftAction->setEnabled(canModify);
    d->mRotateRightAction->setEnabled(canModify);
    d->mMirrorAction->setEnabled(canModify);
    d->mFlipAction->setEnabled(canModify);
    d->mResizeAction->setEnabled(canModify);
    d->mCropAction->setEnabled(canModify && viewMainPageIsVisible);
    d->mRedEyeReductionAction->setEnabled(canModify && viewMainPageIsVisible);

    updateSideBarContent();
}

void ImageOpsContextManagerItem::rotateLeft()
{
    TransformImageOperation* op = new TransformImageOperation(ROT_270);
    applyImageOperation(op);
}

void ImageOpsContextManagerItem::rotateRight()
{
    TransformImageOperation* op = new TransformImageOperation(ROT_90);
    applyImageOperation(op);
}

void ImageOpsContextManagerItem::mirror()
{
    TransformImageOperation* op = new TransformImageOperation(HFLIP);
    applyImageOperation(op);
}

void ImageOpsContextManagerItem::flip()
{
    TransformImageOperation* op = new TransformImageOperation(VFLIP);
    applyImageOperation(op);
}

void ImageOpsContextManagerItem::resizeImage()
{
    if (!d->ensureEditable()) {
        return;
    }
    Document::Ptr doc = DocumentFactory::instance()->load(contextManager()->currentUrl());
    doc->startLoadingFullImage();
    DialogGuard<ResizeImageDialog> dialog(d->mMainWindow);
    dialog->setOriginalSize(doc->size());
    if (!dialog->exec()) {
        return;
    }
    ResizeImageOperation* op = new ResizeImageOperation(dialog->size());
    applyImageOperation(op);
}

void ImageOpsContextManagerItem::crop()
{
    if (!d->ensureEditable()) {
        return;
    }
    RasterImageView* imageView = d->mMainWindow->viewMainPage()->imageView();
    if (!imageView) {
        qCritical() << "No ImageView available!";
        return;
    }
    CropTool* tool = new CropTool(imageView);
    connect(tool, &CropTool::imageOperationRequested, this, &ImageOpsContextManagerItem::applyImageOperation);
    connect(tool, &CropTool::done, this, &ImageOpsContextManagerItem::restoreDefaultImageViewTool);

    d->mMainWindow->setDistractionFreeMode(true);
    imageView->setCurrentTool(tool);
}

void ImageOpsContextManagerItem::startRedEyeReduction()
{
    if (!d->ensureEditable()) {
        return;
    }
    RasterImageView* view = d->mMainWindow->viewMainPage()->imageView();
    if (!view) {
        qCritical() << "No RasterImageView available!";
        return;
    }
    RedEyeReductionTool* tool = new RedEyeReductionTool(view);
    connect(tool, &RedEyeReductionTool::imageOperationRequested, this, &ImageOpsContextManagerItem::applyImageOperation);
    connect(tool, &RedEyeReductionTool::done, this, &ImageOpsContextManagerItem::restoreDefaultImageViewTool);

    d->mMainWindow->setDistractionFreeMode(true);
    view->setCurrentTool(tool);
}

void ImageOpsContextManagerItem::applyImageOperation(AbstractImageOperation* op)
{
    // For now, we only support operations on one image
    QUrl url = contextManager()->currentUrl();

    Document::Ptr doc = DocumentFactory::instance()->load(url);
    op->applyToDocument(doc);
}

void ImageOpsContextManagerItem::restoreDefaultImageViewTool()
{
    RasterImageView* imageView = d->mMainWindow->viewMainPage()->imageView();
    if (!imageView) {
        qCritical() << "No RasterImageView available!";
        return;
    }

    AbstractRasterImageViewTool* tool = imageView->currentTool();
    imageView->setCurrentTool(0);
    tool->deleteLater();
    d->mMainWindow->setDistractionFreeMode(false);
}

} // namespace
