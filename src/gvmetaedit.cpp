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
#include <klocale.h>

// Local
#include "gvdocument.h"
#include "gvmetaedit.moc"

// FIXME: Why doesn't GVMetaEdit inherits from QTextEdit rather than QVBox?
GVMetaEdit::GVMetaEdit(QWidget *parent, GVDocument *gvp, const char *name)
: QVBox(parent, name)
, mDocument(gvp)
{
	mCommentEdit=new QTextEdit(this);
	mCommentEdit->installEventFilter(this);
	connect(mCommentEdit, SIGNAL(modificationChanged(bool)),
		this, SLOT(setModified(bool)));
	connect(mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(updateContent()) );
	connect(mCommentEdit, SIGNAL(textChanged()),
		this, SLOT(updateDoc()) );
	updateContent();
	mCommentEdit->setMinimumHeight(int (mCommentEdit->fontMetrics().height() * 1.5) );
}


GVMetaEdit::~GVMetaEdit() {
}


bool GVMetaEdit::eventFilter(QObject *o, QEvent *e) {
	if (o == mCommentEdit && mEmpty && (mDocument->commentState()==GVDocument::WRITABLE)) {
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
	if (mDocument->isNull()) {
		mCommentEdit->setTextFormat(QTextEdit::RichText);
		mCommentEdit->setText(i18n("<i>No image selected.</i>"));
		mEmpty = true;
		return;
	}

	QString comment=mDocument->comment();

	if (mDocument->commentState() & GVDocument::VALID) {
		mEmpty = comment.isEmpty();
		if (mEmpty) {
			setEmptyText();
		} else {
			mCommentEdit->setTextFormat(QTextEdit::PlainText);
			mCommentEdit->setText(comment);
		}
	} else {
		mCommentEdit->setTextFormat(QTextEdit::RichText);
		mCommentEdit->setText(i18n("<i>This image cannot be commented.</i>"));
	}
	bool writable=mDocument->commentState()==GVDocument::WRITABLE;
	mCommentEdit->setReadOnly(!writable);
	mCommentEdit->setEnabled(writable);
}


void GVMetaEdit::updateDoc() {
	if ((mDocument->commentState()==GVDocument::WRITABLE) && mCommentEdit->isModified()) {
		mDocument->setComment(mCommentEdit->text());
		mCommentEdit->setModified(false);
	}
}


void GVMetaEdit::setEmptyText() {
	QString comment;
	mCommentEdit->setTextFormat(QTextEdit::RichText);
	if (mDocument->commentState()==GVDocument::WRITABLE) {
		comment=i18n("<i>Type here to add a comment to this image.</i>");
	} else {
		comment=i18n("<i>No comment available.</i>");
	}
	mCommentEdit->setText(comment);
}
