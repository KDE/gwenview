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
#include <qtextedit.h>
#include <qfileinfo.h>

// KDE
#include <kfilemetainfo.h>
#include <kdeversion.h>

// Local
#include "gvpixmap.h"
#include "gvmetaedit.moc"


const char* JPEG_EXIF_DATA="Jpeg EXIF Data";
const char* JPEG_EXIF_COMMENT="Comment";


GVMetaEdit::GVMetaEdit(QWidget *parent, GVPixmap *gvp, const char *name)
: QVBox(parent, name) {
	mGVPixmap = gvp;
	mCommentEdit = new QTextEdit(this);
	mMeta = NULL;
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(slotURLChanged()) );
	slotURLChanged();
}


GVMetaEdit::~GVMetaEdit() {
	clearData();
}


void
GVMetaEdit::slotURLChanged() {
	clearData();

	KURL url = mGVPixmap->url();
	if (url.isValid()) {
		// make KFileMetaInfo object and check if file is writable
		#if KDE_VERSION_MAJOR >= 3 && KDE_VERSION_MINOR >= 2
		mMeta = new KFileMetaInfo(url);
		if (url.isLocalFile()) {
			mWritable = QFileInfo(url.path()).isWritable();
		} else {
			mWritable = false;
		}
		#else
		// KDE <= 3.1 can only get meta info from local files
		mMeta = new KFileMetaInfo(url.path());
		mWritable = QFileInfo(url.path()).isWritable();
		#endif

		// retrieve comment item
		QString mimetype = "";
		if (!mMeta->isEmpty()) {
			mimetype = mMeta->mimeType();
		}
		if (mimetype == "image/jpeg") {
			mCommentItem = (*mMeta)[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
		} else if (mimetype == "image/png") {
			// we take the first comment
			QString name = (*mMeta)[JPEG_EXIF_COMMENT].keys()[0];
			mCommentItem = (*mMeta)[JPEG_EXIF_COMMENT][name];
			mWritable = false; // not implemented in KDE
		} else {
			mCommentItem = KFileMetaInfoItem();
			mWritable = false;
		}
		
		/* Some code to debug
		QStringList k,l;
		k = mMeta->groups();
		for (uint i=0; i<k.size(); ++i) {
			l = (*mMeta)[k[i]].keys();
			for (uint j=0; j<l.size(); ++j) {
				kdDebug() << k[i] << '\t' << l[j] <<endl;
			}
		}*/
		
		// set comment in QTextEdit
		if (mCommentItem.isValid()) {
			mCommentEdit->setText(mCommentItem.string());
			mCommentEdit->setReadOnly(!mWritable);
			mCommentEdit->show();
		} else {
			mCommentEdit->hide();
		}
	} else {
		mCommentEdit->hide();
	}
}
void
GVMetaEdit::clearData() {
	if (mMeta) {
		// save changed data
		if (mWritable
			&& mCommentItem.string() != mCommentEdit->text()) {
			mCommentItem.setValue(mCommentEdit->text());
			mMeta->applyChanges();
		}
		delete mMeta;
		mMeta = NULL;
	}
}
