/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include <QVBoxLayout>

// KDE
#include <klocale.h>
#include <kmimetype.h>
#include <kparts/componentfactory.h>
#include <kparts/statusbarextension.h>
#include <kstatusbar.h>

// Local
#include "contextmanager.h"
#include <lib/imageviewpart.h>


namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

struct DocumentViewPrivate {
	DocumentView* mView;
	ContextManager* mContextManager;
	QLabel* mNoDocumentLabel;
	QWidget* mPartContainer;
	QVBoxLayout* mPartContainerLayout;
	KStatusBar* mStatusBar;

	KParts::ReadOnlyPart* mPart;
	QString mPartLibrary;

	void setPartWidget(QWidget* partWidget) {
		if (partWidget) {
			// Insert the widget above the status bar
			mPartContainerLayout->insertWidget(0 /* position */, partWidget, 1 /* stretch */);
			mView->setCurrentWidget(mPartContainer);
		} else {
			mView->setCurrentWidget(mNoDocumentLabel);
		}
	}
};

DocumentView::DocumentView(QWidget* parent)
: QStackedWidget(parent)
, d(new DocumentViewPrivate)
{
	d->mView = this;
	d->mContextManager = 0;
	d->mPart = 0;

	d->mNoDocumentLabel = new QLabel(this);
	addWidget(d->mNoDocumentLabel);
	d->mNoDocumentLabel->setText(i18n("No document selected"));
	d->mNoDocumentLabel->setAlignment(Qt::AlignCenter);
	d->mNoDocumentLabel->setAutoFillBackground(true);
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, Qt::white);
	d->mNoDocumentLabel->setPalette(palette);

	d->mPartContainer = new QWidget(this);
	addWidget(d->mPartContainer);
	d->mStatusBar = new KStatusBar(d->mPartContainer);
	d->mStatusBar->hide();

	d->mPartContainerLayout = new QVBoxLayout(d->mPartContainer);
	d->mPartContainerLayout->addWidget(d->mStatusBar);
	d->mPartContainerLayout->setMargin(0);
	d->mPartContainerLayout->setSpacing(0);
}


DocumentView::~DocumentView() {
	delete d;
}


void DocumentView::setContextManager(ContextManager* manager) {
	d->mContextManager = manager;
}


KStatusBar* DocumentView::statusBar() const {
	return d->mStatusBar;
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
	d->mContextManager->setImageView(0);
	partChanged(0);
	delete d->mPart;
	d->mPartLibrary = QString();
	d->mPart=0;
}


void DocumentView::createPartForUrl(const KUrl& url) {
	QString mimeType=KMimeType::findByUrl(url)->name();

	// Get a list of possible parts
	const KService::List offers = KMimeTypeTrader::self()->query( mimeType, QLatin1String("KParts/ReadOnlyPart"));
	if (offers.isEmpty()) {
		kWarning() << "Couldn't find a KPart for " << mimeType << endl;
		reset();
		return;
	}

	// Check if we are already using it
	KService::Ptr service = offers.first();
	QString library=service->library();
	Q_ASSERT(!library.isNull());
	if (library == d->mPartLibrary) {
		LOG("Reusing current part");
		return;
	}

	// Load new part
	LOG("Loading part from library: " << library);
	KParts::ReadOnlyPart* part = KParts::ComponentFactory::createPartInstanceFromService<KParts::ReadOnlyPart>(
		service,
		d->mPartContainer /*parentWidget*/,
		d->mPartContainer /*parent*/);
	if (!part) {
		kWarning() << "Failed to instantiate KPart from library " << library << endl;
		return;
	}

	// Handle statusbar extension otherwise a statusbar will get created in
	// the main window.
	KParts::StatusBarExtension* extension = KParts::StatusBarExtension::childObject(part);
	if (extension) {
		extension->setStatusBar(statusBar());
		statusBar()->show();
	} else {
		statusBar()->hide();
	}

	ImageViewPart* ivPart = dynamic_cast<ImageViewPart*>(part);
	d->mContextManager->setImageView(ivPart);
	d->setPartWidget(part->widget());
	partChanged(part);

	connect(part, SIGNAL(completed()), SIGNAL(completed()) );

	// Delete the old part, don't do it before mMainWindow->createGUI(),
	// otherwise some UI elements from the old part won't be properly
	// removed. 
	delete d->mPart;
	d->mPart = part;

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


} // namespace
