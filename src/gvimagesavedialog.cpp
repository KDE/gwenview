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
// Qt includes
#include <qtimer.h>

// KDE includes
#include <kdebug.h>
#include <kdeversion.h>
#include <kfilefiltercombo.h>
#include <kimageio.h>
#include <klocale.h>
#include <kurlcombobox.h>

// Our includes
#include "gvimagesavedialog.moc"


static int findFormatInFilterList(const QStringList& filters, const QString& format) {
	int pos=0;
	for(QStringList::const_iterator it=filters.begin(); it!=filters.end(); ++it,++pos) {
		QStringList list=QStringList::split("|",*it);
		if ( list[1].startsWith(format) ) return pos;
	}
	return -1;
}


#if KDE_VERSION < 0x30100
// Dumb replacement function for KImageIO::typeForMime() which does not exist
// in KDE 3.0
static QString typeForMime(const QString& mimeType) {
	QMap<QString,QString> map;
	map["image/gif"]="GIF";
	map["image/jpeg"]="JPEG";
	map["image/png"]="PNG";
	map["image/x-bmp"]="BMP";
	map["image/x-eps"]="EPS";
	map["image/x-ico"]="ICO";
	map["image/x-krl"]="KRL";
	map["image/x-portable-bitmap"]="PBM";
	map["image/x-portable-pixmap"]="PNM";
	map["image/x-xbm"]="XBM";
	map["image/x-xpm"]="XPM";
	return map[mimeType];
}
#endif

GVImageSaveDialog::GVImageSaveDialog(KURL& url,QString& imageFormat,QWidget* parent)
: KFileDialog(url.path(),QString::null,parent,"gvimagesavedialog",true)
, mURL(url)
, mImageFormat(imageFormat)
{
	setOperationMode(KFileDialog::Saving);

	// FIXME: Ugly code to define the filter combo label.
	KMimeType::List types;
	setFilterMimeType(i18n("Format:"),types,KMimeType::mimeType(""));
	
	QStringList filters;

	// Create our filter list
	QStringList mimeTypes=KImageIO::mimeTypes();
	for(QStringList::const_iterator it=mimeTypes.begin(); it!=mimeTypes.end(); ++it) {
		#if KDE_VERSION < 0x30100
		QString format=typeForMime(*it);
		#else
		QString format=KImageIO::typeForMime(*it);
		#endif
		
		// Create the pattern part of the filter string
		KMimeType::Ptr mt=KMimeType::mimeType(*it);
		QStringList patterns;
		for (QStringList::const_iterator patIt=mt->patterns().begin();patIt!=mt->patterns().end();++patIt) {
			QString pattern=(*patIt).lower();
			if (!patterns.contains(pattern)) patterns.append(pattern);
		}
		if (patterns.isEmpty()) {
			patterns.append( QString("*.%1").arg(format.lower()) );
		}
		QString patternString=patterns.join(" ");

		// Create the filter string
		QString filter=QString("%1|%2 - %3 (%4)").arg(patternString).arg(format).arg(mt->comment()).arg(patternString);
		
		// Add it to our list
		filters.append(filter);
	}
	
	qHeapSort(filters);
	setFilter(filters.join("\n"));
	
	// Select the default image format
	int pos=findFormatInFilterList(filters,mImageFormat);
	if (pos==-1) pos=findFormatInFilterList(filters,"PNG");
	
	filterWidget->setCurrentItem(pos);

	// Tweak the filter widget
	filterWidget->setEditable(false);
	
	connect(filterWidget,SIGNAL(activated(const QString&)),
		this,SLOT(updateImageFormat(const QString&)) );
	
	// Call slotFilterChanged() to get the list filtered by the filter we
	// selected. If we don't use a single shot, it leads to a crash :-/
	QTimer::singleShot(0,this,SLOT(slotFilterChanged()));
}


void GVImageSaveDialog::accept() {
	KFileDialog::accept();
	mURL=selectedURL();
}


void GVImageSaveDialog::updateImageFormat(const QString& text) {
	QStringList list=QStringList::split(" ",text);
	mImageFormat=list[0];
	
	QString name=locationEdit->currentText();
	QString suffix=KImageIO::suffix(mImageFormat);
	int dotPos=name.findRev('.');
	if (dotPos>-1) {
		name=name.left(dotPos);
	} 
	name.append('.').append(suffix);
	locationEdit->setCurrentText(name);
}

