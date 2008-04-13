// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "imageopscontextmanageritem.moc"

// Qt

// KDE
#include <kaction.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kactioncollection.h>

// Local
#include "contextmanager.h"
#include "documentview.h"
#include "mainwindow.h"
#include "sidebar.h"
#include <lib/cropsidebar.h>
#include <lib/document/documentfactory.h>
#include <lib/gwenviewconfig.h>
#include <lib/imageviewpart.h>
#include <lib/resizeimageoperation.h>
#include <lib/transformimageoperation.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

struct ImageOpsContextManagerItem::Private {
	ImageOpsContextManagerItem* that;
	MainWindow* mMainWindow;
	SideBar* mSideBar;
	SideBarGroup* mGroup;

	QAction* mRotateLeftAction;
	QAction* mRotateRightAction;
	QAction* mMirrorAction;
	QAction* mFlipAction;
	QAction* mResizeAction;
	QAction* mCropAction;
	QList<QAction*> mActionList;

	void setupActions() {
		KActionCollection* actionCollection = mMainWindow->actionCollection();
		mRotateLeftAction = actionCollection->addAction("rotate_left");
		mRotateLeftAction->setText(i18n("Rotate Left"));
		mRotateLeftAction->setIcon(KIcon("object-rotate-left"));
		mRotateLeftAction->setShortcut(Qt::CTRL + Qt::Key_L);
		connect(mRotateLeftAction, SIGNAL(triggered()),
			that, SLOT(rotateLeft()) );

		mRotateRightAction = actionCollection->addAction("rotate_right");
		mRotateRightAction->setText(i18n("Rotate Right"));
		mRotateRightAction->setIcon(KIcon("object-rotate-right"));
		mRotateRightAction->setShortcut(Qt::CTRL + Qt::Key_R);
		connect(mRotateRightAction, SIGNAL(triggered()),
			that, SLOT(rotateRight()) );

		mMirrorAction = actionCollection->addAction("mirror");
		mMirrorAction->setText(i18n("Mirror"));
		connect(mMirrorAction, SIGNAL(triggered()),
			that, SLOT(mirror()) );

		mFlipAction = actionCollection->addAction("flip");
		mFlipAction->setText(i18n("Flip"));
		connect(mFlipAction, SIGNAL(triggered()),
			that, SLOT(flip()) );

		mResizeAction = actionCollection->addAction("resize");
		mResizeAction->setText(i18n("Resize"));
		connect(mResizeAction, SIGNAL(triggered()),
			that, SLOT(resizeImage()) );

		mCropAction = actionCollection->addAction("crop");
		mCropAction->setText(i18n("Crop"));
		connect(mCropAction, SIGNAL(triggered()),
			that, SLOT(crop()) );

		mActionList
			<< mRotateLeftAction
			<< mRotateRightAction
			<< mMirrorAction
			<< mFlipAction
			<< mResizeAction
			<< mCropAction;
	}


	void applyImageOperation(AbstractImageOperation* op) {
		// For now, we only support operations on one image
		KUrl url = that->contextManager()->currentUrl();

		Document::Ptr doc = DocumentFactory::instance()->load(url);
		doc->loadFullImage();
		doc->waitUntilLoaded();
		op->setDocument(doc);
		doc->undoStack()->push(op);
	}
};


ImageOpsContextManagerItem::ImageOpsContextManagerItem(ContextManager* manager, MainWindow* mainWindow)
: AbstractContextManagerItem(manager)
, d(new Private) {
	d->that = this;
	d->mMainWindow = mainWindow;
	d->mSideBar = 0;
	d->mGroup = 0;
	d->setupActions();
	updateActions();
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateActions()) );
	connect(mainWindow, SIGNAL(viewModeChanged()),
		SLOT(updateActions()) );
	connect(mainWindow->documentView(), SIGNAL(completed()),
		SLOT(updateActions()) );
}


ImageOpsContextManagerItem::~ImageOpsContextManagerItem() {
	delete d;
}


void ImageOpsContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("Image Operations"));
}


void ImageOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	d->mGroup->clear();
	bool notEmpty = false;
	Q_FOREACH(QAction* action, d->mActionList) {
		if (action->isEnabled()) {
			d->mGroup->addAction(action);
			notEmpty = true;
		}
	}

	if (notEmpty) {
		d->mGroup->show();
	} else {
		d->mGroup->hide();
	}
}


void ImageOpsContextManagerItem::updateActions() {
	bool canModify = d->mMainWindow->currentDocumentIsRasterImage();
	bool documentViewIsVisible = d->mMainWindow->documentView()->isVisible();
	if (!documentViewIsVisible) {
		// Since we only support image operations on one image for now,
		// disable actions if several images are selected and the document
		// view is not visible.
		if (contextManager()->selection().count() != 1) {
			canModify = false;
		}
	}

	d->mRotateLeftAction->setEnabled(canModify);
	d->mRotateRightAction->setEnabled(canModify);
	d->mMirrorAction->setEnabled(canModify);
	d->mFlipAction->setEnabled(canModify);
	d->mResizeAction->setEnabled(canModify);
	d->mCropAction->setEnabled(canModify && documentViewIsVisible);

	if (d->mSideBar) {
		updateSideBarContent();
	}
}


void ImageOpsContextManagerItem::rotateLeft() {
	TransformImageOperation* op = new TransformImageOperation(ROT_270);
	d->applyImageOperation(op);
}


void ImageOpsContextManagerItem::rotateRight() {
	TransformImageOperation* op = new TransformImageOperation(ROT_90);
	d->applyImageOperation(op);
}


void ImageOpsContextManagerItem::mirror() {
	TransformImageOperation* op = new TransformImageOperation(HFLIP);
	d->applyImageOperation(op);
}


void ImageOpsContextManagerItem::flip() {
	TransformImageOperation* op = new TransformImageOperation(VFLIP);
	d->applyImageOperation(op);
}


void ImageOpsContextManagerItem::resizeImage() {
	Document::Ptr doc = DocumentFactory::instance()->load(contextManager()->currentUrl());
	doc->loadFullImage();
	doc->waitUntilLoaded();
	int size = GwenviewConfig::imageResizeLastSize();
	if (size == -1) {
		size = qMax(doc->width(), doc->height());
	}
	bool ok = false;
	size = KInputDialog::getInteger(
		i18n("Image Resizing"),
		i18n("Enter the new size of the image:"),
		size, 0, 100000, 10 /* step */,
		&ok,
		d->mMainWindow);
	if (!ok) {
		return;
	}
	GwenviewConfig::setImageResizeLastSize(size);
	ResizeImageOperation* op = new ResizeImageOperation(size);
	d->applyImageOperation(op);
}


void ImageOpsContextManagerItem::crop() {
	ImageViewPart* imageViewPart = d->mMainWindow->documentView()->imageViewPart();
	if (!imageViewPart) {
		kError() << "No ImageViewPart available!";
		return;
	}
	Document::Ptr doc = DocumentFactory::instance()->load(contextManager()->currentUrl());
	CropSideBar* cropSideBar = new CropSideBar(d->mMainWindow, imageViewPart->imageView(), doc);
	connect(cropSideBar, SIGNAL(done()),
		d->mMainWindow, SLOT(hideTemporarySideBar()) );

	d->mMainWindow->showTemporarySideBar(cropSideBar);
}


} // namespace
