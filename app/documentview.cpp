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
#include <kparts/mainwindow.h>
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
	KParts::MainWindow* mMainWindow;
	ContextManager* mContextManager;
	QLabel* mNoDocumentLabel;
	QWidget* mViewContainer;
	QVBoxLayout* mViewContainerLayout;
	KStatusBar* mStatusBar;

	KParts::ReadOnlyPart* mPart;
	QString mPartLibrary;
};

DocumentView::DocumentView(QWidget* parent, KParts::MainWindow* mainWindow)
: QStackedWidget(parent)
, d(new DocumentViewPrivate)
{
	d->mMainWindow = mainWindow;
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

	d->mViewContainer = new QWidget(this);
	addWidget(d->mViewContainer);
	d->mStatusBar = new KStatusBar(d->mViewContainer);
	d->mStatusBar->hide();

	d->mViewContainerLayout = new QVBoxLayout(d->mViewContainer);
	d->mViewContainerLayout->addWidget(d->mStatusBar);
	d->mViewContainerLayout->setMargin(0);
	d->mViewContainerLayout->setSpacing(0);
}


DocumentView::~DocumentView() {
	delete d;
}


void DocumentView::setContextManager(ContextManager* manager) {
	d->mContextManager = manager;
}


void DocumentView::setView(QWidget* view) {
	if (view) {
		// Insert the widget above the status bar
		d->mViewContainerLayout->insertWidget(0 /* position */, view, 1 /* stretch */);
		setCurrentWidget(d->mViewContainer);
	} else {
		setCurrentWidget(d->mNoDocumentLabel);
	}
}

QWidget* DocumentView::viewContainer() const {
	return d->mViewContainer;
}

KStatusBar* DocumentView::statusBar() const {
	return d->mStatusBar;
}

QSize DocumentView::sizeHint() const {
	return QSize(400, 300);
}


KParts::ReadOnlyPart* DocumentView::part() const {
	return d->mPart;
}


void DocumentView::reset() {
	if (!d->mPart) {
		return;
	}
	setView(0);
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
		viewContainer() /*parentWidget*/,
		viewContainer() /*parent*/);
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
	setView(part->widget());
	partChanged(part);

	// Make sure our file list is filled when the part is done.
	// FIXME: REFACTOR
	connect(part, SIGNAL(completed()), d->mMainWindow, SLOT(slotPartCompleted()) );

	// Delete the old part, don't do it before mMainWindow->createGUI(),
	// otherwise some UI elements from the old part won't be properly
	// removed. 
	delete d->mPart;
	d->mPart = part;

	d->mPartLibrary = library;
}

} // namespace
