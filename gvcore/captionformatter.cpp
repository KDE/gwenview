// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
#include "captionformatter.h"

// Qt
#include <qregexp.h>

// KDE
#include <klocale.h>

// Local
#include "fileviewstack.h"
#include "document.h"

namespace Gwenview {

	
CaptionFormatter::CaptionFormatter(FileViewStack* fileView, Document* document)
: mFileView(fileView), mDocument(document) {
}


QString CaptionFormatter::operator()(const QString& format) {
	QString str=format;
	
	QString path=mDocument->url().path();
	QString fileName=mDocument->filename();
	QString comment=mDocument->comment();
	if (comment.isNull()) {
		comment=i18n("(No comment)");
	}
	QString resolution = QString( "%1x%2" ).arg( mDocument->width()).arg( mDocument->height());

	int position=mFileView->shownFilePosition()+1;
	int count=mFileView->fileCount();
	
	str.replace("\\n", "\n");
	str.replace(QRegExp("%f"), fileName);
	str.replace(QRegExp("%p"), path);
	str.replace(QRegExp("%c"), comment);
	str.replace(QRegExp("%r"), resolution);
	str.replace(QRegExp("%n"), QString::number(position));
	str.replace(QRegExp("%N"), QString::number(count));
	return str;
}


} // namespace
