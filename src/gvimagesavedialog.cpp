/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

// KDE includes
#include <kdebug.h>
#include <kimageio.h>
#include <klocale.h>

// Our includes
#include "gvimagesavedialog.moc"


GVImageSaveDialog::GVImageSaveDialog(KURL& url,QString& format,QWidget* parent)
: KFileDialog(url.path(),QString::null,parent,"gvimagesavedialog",true)
, mURL(url)
, mFormat(format)
{
	setOperationMode(KFileDialog::Saving);
	// Create our KMimeTypeList
	QStringList strTypes=KImageIO::mimeTypes();
	KMimeType::List types;
	for(QStringList::const_iterator it=strTypes.begin(); it!=strTypes.end(); ++it) {
		types.append( KMimeType::mimeType(*it) );
	}

	QString strDefaultType=KImageIO::mimeType(url.path());
	KMimeType::Ptr defaultType=KMimeType::mimeType(strDefaultType);
	
	// Get file name  and format
	setFilterMimeType(i18n("Format:"),types,defaultType);

}


void GVImageSaveDialog::accept() {
	KFileDialog::accept();
	mURL=selectedURL();
	mFormat=KImageIO::typeForMime( currentMimeFilter() );
}


