// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
 
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
// Qt 
#include <qprogressdialog.h>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>

// Local
#include "document.h"
#include "batchmanipulator.moc"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

class BatchManipulatorPrivate {
public:
	KURL::List mURLs;
	ImageUtils::Orientation mOrientation;
	QProgressDialog* mProgressDialog;
	bool mLoaded;
};


BatchManipulator::BatchManipulator(QWidget* parent, const KURL::List& urls, ImageUtils::Orientation orientation)
: QObject(parent) {
	d=new BatchManipulatorPrivate();
	d->mURLs=urls;
	d->mOrientation=orientation;
	d->mProgressDialog=new QProgressDialog(i18n("Manipulating images..."), i18n("Close"),
		d->mURLs.size(), parent, 0, true);
}

BatchManipulator::~BatchManipulator() {
	delete d->mProgressDialog;
	delete d;
}

void BatchManipulator::apply() {
	LOG("");
	KURL::List::ConstIterator it=d->mURLs.begin();
	Document doc(0);
	connect(&doc, SIGNAL(loaded(const KURL&)),
		this, SLOT(slotImageLoaded()) );
	d->mProgressDialog->show();
	for (;it!=d->mURLs.end(); ++it) {
		LOG("Loading " << (*it).prettyURL());
		d->mLoaded=false;
		doc.setURL(*it);

		d->mProgressDialog->setProgress(d->mProgressDialog->progress()+1);
		
		// Wait for the image to load
		do {
			if (d->mProgressDialog->wasCancelled()) break;
			kapp->processEvents();
		} while (!d->mLoaded);
		if (d->mProgressDialog->wasCancelled()) break;
		
		LOG("Transforming " << (*it).prettyURL());
		doc.transform(d->mOrientation);
		
		LOG("Saving " << (*it).prettyURL());
		doc.save();

		emit imageModified(*it);
	}
	d->mProgressDialog->close();
}

void BatchManipulator::slotImageLoaded() {
	d->mLoaded=true;
}

} // namespace
