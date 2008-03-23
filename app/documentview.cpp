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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "documentview.moc"

// Qt
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kparts/componentfactory.h>
#include <kparts/statusbarextension.h>
#include <kstatusbar.h>
#include <ktoggleaction.h>

// Local
#include "thumbnailbarview.h"
#include <lib/imageview.h>
#include <lib/imageviewpart.h>
#include <lib/mimetypeutils.h>


namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

struct DocumentViewPrivate {
	DocumentView* mView;
	QLabel* mNoDocumentLabel;
	QWidget* mPartContainer;
	QVBoxLayout* mPartContainerLayout;
	QToolButton* mToggleThumbnailBarButton;
	KStatusBar* mStatusBar;
	ThumbnailBarView* mThumbnailBar;
	KToggleAction* mToggleThumbnailBarAction;
	bool mFullScreenMode;
	QPalette mNormalPalette;
	QPalette mFullScreenPalette;
	bool mThumbnailBarVisibleBeforeFullScreen;

	KParts::ReadOnlyPart* mPart;
	QString mPartLibrary;

	void setupStatusBar(QWidget* container) {
		mStatusBar = new KStatusBar;
		mToggleThumbnailBarButton = new QToolButton;
		mToggleThumbnailBarButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mToggleThumbnailBarButton->setAutoRaise(true);
		mToggleThumbnailBarButton->setFocusPolicy(Qt::NoFocus);
		QHBoxLayout* layout = new QHBoxLayout(container);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(mToggleThumbnailBarButton, 0, Qt::AlignLeft);
		layout->addWidget(mStatusBar);
	}

	void setPartWidget(QWidget* partWidget) {
		if (partWidget) {
			// Insert the widget above the status bar
			mPartContainerLayout->insertWidget(0 /* position */, partWidget, 1 /* stretch */);
			mView->setCurrentWidget(mPartContainer);
		} else {
			mView->setCurrentWidget(mNoDocumentLabel);
		}
	}

	void applyPalette() {
		QPalette palette = mFullScreenMode ? mFullScreenPalette : mNormalPalette;
		mView->setPalette(palette);

		if (!mPart) {
			return;
		}

		QPalette partPalette = mPart->widget()->palette();
		partPalette.setBrush(mPart->widget()->backgroundRole(), palette.base());
		partPalette.setBrush(mPart->widget()->foregroundRole(), palette.text());
		mPart->widget()->setPalette(partPalette);
	}
};


DocumentView::DocumentView(QWidget* parent, KActionCollection* actionCollection)
: QStackedWidget(parent)
, d(new DocumentViewPrivate)
{
	d->mView = this;
	d->mPart = 0;
	d->mFullScreenMode = false;
	d->mThumbnailBarVisibleBeforeFullScreen = false;
	d->mFullScreenPalette = QPalette(palette());
	d->mFullScreenPalette.setColor(QPalette::Base, Qt::black);
	d->mFullScreenPalette.setColor(QPalette::Text, Qt::white);

	d->mNoDocumentLabel = new QLabel(this);
	addWidget(d->mNoDocumentLabel);
	d->mNoDocumentLabel->setText(i18n("No document selected"));
	d->mNoDocumentLabel->setAlignment(Qt::AlignCenter);
	d->mNoDocumentLabel->setAutoFillBackground(true);
	d->mNoDocumentLabel->setBackgroundRole(QPalette::Base);
	d->mNoDocumentLabel->setForegroundRole(QPalette::Text);

	d->mPartContainer = new QWidget(this);
	addWidget(d->mPartContainer);

	QWidget* statusBarContainer = new QWidget;
	d->setupStatusBar(statusBarContainer);

	d->mThumbnailBar = new ThumbnailBarView(d->mPartContainer);
	ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
	d->mThumbnailBar->setItemDelegate(delegate);
	d->mThumbnailBar->hide();

	d->mPartContainerLayout = new QVBoxLayout(d->mPartContainer);
	d->mPartContainerLayout->addWidget(d->mThumbnailBar);
	d->mPartContainerLayout->addWidget(statusBarContainer);
	d->mPartContainerLayout->setMargin(0);
	d->mPartContainerLayout->setSpacing(0);

	d->mToggleThumbnailBarAction = actionCollection->add<KToggleAction>("toggle_thumbnailbar");
	d->mToggleThumbnailBarAction->setText(i18n("Show Thumbnail Bar"));
	d->mToggleThumbnailBarAction->setIcon(KIcon("folder-image"));
	d->mToggleThumbnailBarAction->setShortcut(Qt::CTRL | Qt::Key_T);
	connect(d->mToggleThumbnailBarAction, SIGNAL(triggered(bool)),
		d->mThumbnailBar, SLOT(setVisible(bool)));
	d->mToggleThumbnailBarButton->setDefaultAction(d->mToggleThumbnailBarAction);
}


DocumentView::~DocumentView() {
	delete d;
}


KStatusBar* DocumentView::statusBar() const {
	return d->mStatusBar;
}


void DocumentView::setFullScreenMode(bool fullScreenMode) {
	d->mFullScreenMode = fullScreenMode;
	d->mStatusBar->setVisible(!fullScreenMode);
	d->applyPalette();
	if (fullScreenMode) {
		d->mThumbnailBarVisibleBeforeFullScreen = d->mToggleThumbnailBarAction->isChecked();
		if (d->mThumbnailBarVisibleBeforeFullScreen) {
			d->mToggleThumbnailBarAction->trigger();
		}
	} else if (d->mThumbnailBarVisibleBeforeFullScreen) {
		d->mToggleThumbnailBarAction->trigger();
	}
	d->mToggleThumbnailBarAction->setEnabled(!fullScreenMode);
}


ThumbnailBarView* DocumentView::thumbnailBar() const {
	return d->mThumbnailBar;
}


QSize DocumentView::sizeHint() const {
	return QSize(400, 300);
}


KUrl DocumentView::url() const {
	if (!d->mPart) {
		return KUrl();
	}

	return d->mPart->url();
}


void DocumentView::reset() {
	if (!d->mPart) {
		return;
	}
	d->setPartWidget(0);
	partChanged(0);
	delete d->mPart;
	d->mPartLibrary = QString();
	d->mPart=0;
}


void DocumentView::createPartForUrl(const KUrl& url) {
	QString mimeType = MimeTypeUtils::urlMimeType(url);

	QString library;
	QVariantList partArgs;
	if (MimeTypeUtils::rasterImageMimeTypes().contains(mimeType)) {
		LOG("Enforcing use of Gwenview part");
		library = "gvpart";
		partArgs << QVariant("gwenviewHost");
	} else {
		LOG("Query system for available parts for" << mimeType);
		// Get a list of possible parts
		const KService::List offers = KMimeTypeTrader::self()->query( mimeType, QLatin1String("KParts/ReadOnlyPart"));
		if (offers.isEmpty()) {
			kWarning() << "Couldn't find a KPart for " << mimeType ;
			reset();
			return;
		}

		// Check if we are already using it
		KService::Ptr service = offers.first();
		library=service->library();
	}
	Q_ASSERT(!library.isNull());
	if (library == d->mPartLibrary) {
		LOG("Reusing current part");
		return;
	}

	// Load new part
	LOG("Loading part from library" << library);
	KPluginFactory* factory = KPluginLoader(library).factory();
	if (!factory) {
		kWarning() << "Failed to load library" << library;
		return;
	}
	LOG("Loading part from library" << library);
	KParts::ReadOnlyPart* part = factory->create<KParts::ReadOnlyPart>(d->mPartContainer, partArgs);
	if (!part) {
		kWarning() << "Failed to instantiate KPart from library" << library ;
		return;
	}

	ImageViewPart* ivPart = dynamic_cast<ImageViewPart*>(part);
	if (ivPart) {
		connect(ivPart, SIGNAL(resizeRequested(const QSize&)),
			d->mView, SIGNAL(resizeRequested(const QSize&)) );
		connect(ivPart, SIGNAL(previousImageRequested()),
			d->mView, SIGNAL(previousImageRequested()) );
		connect(ivPart, SIGNAL(nextImageRequested()),
			d->mView, SIGNAL(nextImageRequested()) );
	}

	// Handle statusbar extension otherwise a statusbar will get created in
	// the main window.
	KParts::StatusBarExtension* extension = KParts::StatusBarExtension::childObject(part);
	if (extension) {
		extension->setStatusBar(statusBar());
		statusBar()->setVisible(!d->mFullScreenMode);
	} else {
		statusBar()->hide();
	}

	d->setPartWidget(part->widget());
	partChanged(part);

	connect(part, SIGNAL(completed()), SIGNAL(completed()) );

	// Delete the old part, don't do it before mMainWindow->createGUI(),
	// otherwise some UI elements from the old part won't be properly
	// removed. 
	delete d->mPart;
	d->mPart = part;

	d->applyPalette();

	d->mPartLibrary = library;
}


bool DocumentView::openUrl(const KUrl& url) {
	createPartForUrl(url);
	if (!d->mPart) {
		return false;
	}
	d->mPart->openUrl(url);
	return true;
}


bool DocumentView::currentDocumentIsRasterImage() const {
	// If the document view is visible, we assume we have a raster
	// image if and only if we are using the ImageViewPart. This avoids
	// having to determine the mimetype a second time.
	return dynamic_cast<ImageViewPart*>(d->mPart) != 0;
}


bool DocumentView::isEmpty() const {
	return !d->mPart;
}


ImageViewPart* DocumentView::imageViewPart() const {
	return dynamic_cast<ImageViewPart*>(d->mPart);
}


void DocumentView::setNormalPalette(const QPalette& palette) {
	d->mNormalPalette = palette;
	d->applyPalette();
}


} // namespace
