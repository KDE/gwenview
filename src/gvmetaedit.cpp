// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Copyright (c) 2003 Jos van den Oever

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
#include <qlabel.h>
#include <qtextedit.h>
#include <qfileinfo.h>

// KDE
#include <kdeversion.h>
#include <kfilemetainfo.h>
#include <klocale.h>

// Local
#include "gvpixmap.h"
#include "gvmetaedit.moc"


const char* JPEG_EXIF_DATA="Jpeg EXIF Data";
const char* JPEG_EXIF_COMMENT="Comment";
const char* PNG_COMMENT="Comment";


GVMetaEdit::GVMetaEdit(QWidget *parent, GVPixmap *gvp, const char *name)
: QVBox(parent, name)
, mGVPixmap(gvp)
, mMetaInfo(0L)
{
	mCommentEdit=new QTextEdit(this);
	mCommentEdit->installEventFilter(this);
	connect(mCommentEdit, SIGNAL(modificationChanged(bool)),
		this, SLOT(setModified(bool)));
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(updateContent()) );
	updateContent();
}


GVMetaEdit::~GVMetaEdit() {
	clearData();
}


bool GVMetaEdit::eventFilter(QObject *o, QEvent *e) {
	if (o == mCommentEdit && mEmpty && mWritable) {
		if (e->type() == QEvent::FocusIn) {
			mCommentEdit->setTextFormat(QTextEdit::PlainText);
			mCommentEdit->setText("");
		} else if (e->type() == QEvent::FocusOut) {
			setEmptyText();
		}
	}
	return false;
}


void GVMetaEdit::setModified(bool m) {
	if (m && mEmpty) {
		mEmpty = false;
	}
}


void GVMetaEdit::updateContent() {
	clearData();

	if (mGVPixmap->isNull()) {
		mCommentEdit->setTextFormat(QTextEdit::RichText);
		mCommentEdit->setText(i18n("<i>No image selected.</i>"));
		//mCommentEdit->setEnabled(false);
		mEmpty = true;
		mWritable = false;
		return;
	}

	KURL url = mGVPixmap->url();
	// make KFileMetaInfo object and check if file is writable
	#if KDE_VERSION_MAJOR >= 3 && KDE_VERSION_MINOR >= 2
	mMetaInfo = new KFileMetaInfo(url);
	if (url.isLocalFile()) {
		mWritable = QFileInfo(url.path()).isWritable();
	} else {
		mWritable = false;
	}
	#else
	// KDE <= 3.1 can only get meta info from local files
	mMetaInfo = new KFileMetaInfo(url.path());
	mWritable = QFileInfo(url.path()).isWritable();
	#endif

	// retrieve comment item
	QString mimetype = "";
	if (!mMetaInfo->isEmpty()) {
		mimetype = mMetaInfo->mimeType();
	}
	if (mimetype == "image/jpeg") {
		mCommentItem = (*mMetaInfo)[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
	} else if (mimetype == "image/png") {
		// we take the first comment
		QString name = (*mMetaInfo)[PNG_COMMENT].keys()[0];
		mCommentItem = (*mMetaInfo)[PNG_COMMENT][name];
		mWritable = false; // not implemented in KDE
	} else {
		mCommentItem = KFileMetaInfoItem();
		mWritable = false;
	}
	
	/* Some code to debug
	QStringList k,l;
	k = mMetaInfo->groups();
	for (uint i=0; i<k.size(); ++i) {
		l = (*mMetaInfo)[k[i]].keys();
		for (uint j=0; j<l.size(); ++j) {
			kdDebug() << k[i] << '\t' << l[j] <<endl;
		}
	}*/
	
	// set comment in QTextEdit
	if (mCommentItem.isValid()) {
		QString comment = QString::fromUtf8( mCommentItem.string().ascii());
		mEmpty = comment.isEmpty();
		if (mEmpty) {
			setEmptyText();
		} else {
			mCommentEdit->setTextFormat(QTextEdit::PlainText);
			mCommentEdit->setText(comment);
		}
	} else {
		mCommentEdit->setTextFormat(QTextEdit::RichText);
		mCommentEdit->setText("<i>This image can't be commented.</i>");
	}
	mCommentEdit->setReadOnly(!mWritable);
	mCommentEdit->setEnabled(mWritable);
}


void GVMetaEdit::clearData() {
	if (!mMetaInfo) return;

	// save changed data
	if (mWritable && mCommentEdit->isModified()) {
		mCommentItem.setValue(mCommentEdit->text());
		mMetaInfo->applyChanges();
		mCommentEdit->setModified(false);
	}
	delete mMetaInfo;
	mMetaInfo = 0L;
}


void GVMetaEdit::setEmptyText() {
	QString comment;
	mCommentEdit->setTextFormat(QTextEdit::RichText);
	if (mWritable) {
		comment=i18n("<i>Type here to add a comment to this image.</i>");
	} else {
		comment=i18n("<i>No comment available.</i>");
	}
	mCommentEdit->setText(comment);
}
