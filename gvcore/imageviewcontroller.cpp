// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <imageviewcontroller.moc>

// Qt
#include <qwidgetstack.h>

// KDE
#include <kdebug.h>
#include <kmimetype.h>
#include <kuserprofile.h>
#include <kparts/componentfactory.h>

// Local
#include <document.h>
#include <imageview.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

struct ImageViewController::Private {
	Document* mDocument;
	KActionCollection* mActionCollection;
	QWidgetStack* mStack;
	ImageView* mImageView;
	
	QString mPlayerLibrary;
	KParts::ReadOnlyPart* mPlayerPart;

	void createPlayerPart(void) {
		QString mimeType=KMimeType::findByURL(mDocument->url())->name();
		KService::Ptr service = KServiceTypeProfile::preferredService(mimeType, "KParts/ReadOnlyPart");
		if (!service) {
			mPlayerLibrary=QString::null;
			delete mPlayerPart;
			mPlayerPart=0;
			return;
		}

		QString library=service->library();
		Q_ASSERT(!library.isNull());
		LOG("Library:" << library);
		if (library==mPlayerLibrary) {
			LOG("Reusing library");
			return;
		}
		mPlayerLibrary=library;
		
		mPlayerPart = KParts::ComponentFactory::createPartInstanceFromService<KParts::ReadOnlyPart>(service, mStack, 0, mStack, 0);
		if (!mPlayerPart) return;
		mStack->addWidget(mPlayerPart->widget());
	}
	

	void showPlayerPart(void) {
		LOG("");
		createPlayerPart();
		if (!mPlayerPart) return;
		mStack->raiseWidget(mPlayerPart->widget());
		mPlayerPart->openURL(mDocument->url());
	}

	
	void showImageView(void) {
		LOG("");
		mStack->raiseWidget(mImageView);
	}
};


ImageViewController::ImageViewController(QWidget* parent, Document* document, KActionCollection* actionCollection)
: QObject(parent) {
	d=new Private;
	d->mDocument=document;
	d->mActionCollection=actionCollection;

	d->mStack=new QWidgetStack(parent);
	d->mImageView=new ImageView(d->mStack, document, actionCollection);
	d->mStack->addWidget(d->mImageView);
	d->mPlayerPart=0;
	
	connect(d->mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotLoaded()) );
}


ImageViewController::~ImageViewController() {
	delete d;
}

void ImageViewController::slotLoaded() {
	LOG("");
	if (d->mDocument->fileType()==Document::FILE_OTHER) {
		d->showPlayerPart();
	} else {
		d->showImageView();
	}
}

QWidget* ImageViewController::widget() const {
	return d->mStack;
}

ImageView* ImageViewController::imageView() const {
	return d->mImageView;
}

} // namespace
