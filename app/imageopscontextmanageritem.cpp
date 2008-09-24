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
#include <QPointer>

// KDE
#include <kaction.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kactioncollection.h>
#include <kactioncategory.h>

// Local
#include "contextmanager.h"
#include "documentpanel.h"
#include "gvcore.h"
#include "mainwindow.h"
#include "sidebar.h"
#include <lib/cropsidebar.h>
#include <lib/document/documentfactory.h>
#include <lib/gwenviewconfig.h>
#include <lib/imageview.h>
#include <lib/redeyereduction/redeyereductiontool.h>
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

	KAction* mRotateLeftAction;
	KAction* mRotateRightAction;
	KAction* mMirrorAction;
	KAction* mFlipAction;
	KAction* mResizeAction;
	KAction* mCropAction;
	KAction* mRedEyeReductionAction;
	QList<KAction*> mActionList;

	void setupActions() {
		KActionCollection* actionCollection = mMainWindow->actionCollection();
		KActionCategory* edit=new KActionCategory(i18nc("@title actions category - means actions changing image","Edit"), actionCollection);
		mRotateLeftAction = edit->addAction("rotate_left",that, SLOT(rotateLeft()));
		mRotateLeftAction->setText(i18n("Rotate Left"));
		mRotateLeftAction->setIcon(KIcon("object-rotate-left"));
		mRotateLeftAction->setShortcut(Qt::CTRL + Qt::Key_L);

		mRotateRightAction = edit->addAction("rotate_right",that, SLOT(rotateRight()));
		mRotateRightAction->setText(i18n("Rotate Right"));
		mRotateRightAction->setIcon(KIcon("object-rotate-right"));
		mRotateRightAction->setShortcut(Qt::CTRL + Qt::Key_R);

		mMirrorAction = edit->addAction("mirror",that, SLOT(mirror()));
		mMirrorAction->setText(i18n("Mirror"));
		mMirrorAction->setIcon(KIcon("object-flip-horizontal"));

		mFlipAction = edit->addAction("flip",that, SLOT(flip()));
		mFlipAction->setText(i18n("Flip"));
		mFlipAction->setIcon(KIcon("object-flip-vertical"));

		mResizeAction = edit->addAction("resize",that, SLOT(resizeImage()) );
		mResizeAction->setText(i18n("Resize"));
		mResizeAction->setIcon(KIcon("transform-scale"));

		mCropAction = edit->addAction("crop",that, SLOT(showCropSideBar()));
		mCropAction->setText(i18n("Crop"));
		mCropAction->setIcon(KIcon("transform-crop-and-resize"));

		mRedEyeReductionAction = edit->addAction("red_eye_reduction",that, SLOT(startRedEyeReduction()) );
		mRedEyeReductionAction->setText(i18n("Red Eye Reduction"));
		//mRedEyeReductionAction->setIcon(KIcon("transform-crop-and-resize"));

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


	bool ensureEditable() {
		KUrl url = that->contextManager()->currentUrl();
		return GvCore::ensureDocumentIsEditable(url);
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
	connect(mainWindow->documentPanel(), SIGNAL(completed()),
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
	Q_FOREACH(KAction* action, d->mActionList) {
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
	bool documentPanelIsVisible = d->mMainWindow->documentPanel()->isVisible();
	if (!documentPanelIsVisible) {
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
	d->mCropAction->setEnabled(canModify && documentPanelIsVisible);
	d->mRedEyeReductionAction->setEnabled(canModify && documentPanelIsVisible);

	if (d->mSideBar) {
		updateSideBarContent();
	}
}


void ImageOpsContextManagerItem::rotateLeft() {
	if (!d->ensureEditable()) {
		return;
	}
	TransformImageOperation* op = new TransformImageOperation(ROT_270);
	applyImageOperation(op);
}


void ImageOpsContextManagerItem::rotateRight() {
	if (!d->ensureEditable()) {
		return;
	}
	TransformImageOperation* op = new TransformImageOperation(ROT_90);
	applyImageOperation(op);
}


void ImageOpsContextManagerItem::mirror() {
	if (!d->ensureEditable()) {
		return;
	}
	TransformImageOperation* op = new TransformImageOperation(HFLIP);
	applyImageOperation(op);
}


void ImageOpsContextManagerItem::flip() {
	if (!d->ensureEditable()) {
		return;
	}
	TransformImageOperation* op = new TransformImageOperation(VFLIP);
	applyImageOperation(op);
}


void ImageOpsContextManagerItem::resizeImage() {
	if (!d->ensureEditable()) {
		return;
	}
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
	applyImageOperation(op);
}


void ImageOpsContextManagerItem::showCropSideBar() {
	if (!d->ensureEditable()) {
		return;
	}
	ImageView* imageView = d->mMainWindow->documentPanel()->imageView();
	if (!imageView) {
		kError() << "No ImageView available!";
		return;
	}
	Document::Ptr doc = DocumentFactory::instance()->load(contextManager()->currentUrl());
	CropSideBar* cropSideBar = new CropSideBar(d->mMainWindow, imageView, doc);
	connect(cropSideBar, SIGNAL(done()),
		d->mMainWindow, SLOT(hideTemporarySideBar()) );
	connect(cropSideBar, SIGNAL(imageOperationRequested(AbstractImageOperation*)),
		SLOT(applyImageOperation(AbstractImageOperation*)) );

	d->mMainWindow->showTemporarySideBar(cropSideBar);
}


void ImageOpsContextManagerItem::startRedEyeReduction() {
	if (!d->ensureEditable()) {
		return;
	}
	ImageView* imageView = d->mMainWindow->documentPanel()->imageView();
	if (!imageView) {
		kError() << "No ImageView available!";
		return;
	}
	RedEyeReductionTool* tool = new RedEyeReductionTool(imageView);
	connect(tool, SIGNAL(imageOperationRequested(AbstractImageOperation*)),
		SLOT(applyImageOperation(AbstractImageOperation*)) );
	connect(tool, SIGNAL(done()),
		SLOT(restoreDefaultImageViewTool()) );

	imageView->setCurrentTool(tool);
}


void ImageOpsContextManagerItem::applyImageOperation(AbstractImageOperation* op) {
	// For now, we only support operations on one image
	KUrl url = contextManager()->currentUrl();

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->loadFullImage();
	doc->waitUntilLoaded();
	op->setDocument(doc);
	doc->undoStack()->push(op);
}


void ImageOpsContextManagerItem::restoreDefaultImageViewTool() {
	ImageView* imageView = d->mMainWindow->documentPanel()->imageView();
	if (!imageView) {
		kError() << "No ImageView available!";
		return;
	}

	AbstractImageViewTool* tool = imageView->currentTool();
	imageView->setCurrentTool(0);
	tool->deleteLater();
}


} // namespace
