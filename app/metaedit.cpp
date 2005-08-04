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
#include "gvcore/document.h"
#include "metaedit.moc"
namespace Gwenview {

// FIXME: Why doesn't MetaEdit inherits from QTextEdit rather than QVBox?
MetaEdit::MetaEdit(QWidget *parent, Document *gvp, const char *name)
: QVBox(parent, name)
, mEmpty(true)
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


MetaEdit::~MetaEdit() {
}


bool MetaEdit::eventFilter(QObject*, QEvent *event) {
	if (mEmpty
		&& (mDocument->commentState()==Document::WRITABLE)
		&& (event->type()==QEvent::FocusIn || event->type()==QEvent::FocusOut) 
	) {
		setEmptyText();
	}
	return false;
}


void MetaEdit::setModified(bool m) {
	if (m && mEmpty) {
		mEmpty = false;
	}
}


void MetaEdit::updateContent() {
	if (mDocument->isNull()) {
		setMessage(i18n("No image selected."));
		return;
	}

	if (mDocument->commentState() == Document::NONE) {
		setMessage(i18n("This image cannot be commented."));
		return;
	}
	
	QString comment=mDocument->comment();
	mEmpty = comment.isEmpty();
	if (mEmpty) {
		setEmptyText();
		return;
	}
	setComment(comment);
}


void MetaEdit::updateDoc() {
	if ((mDocument->commentState()==Document::WRITABLE) && mCommentEdit->isModified()) {
		mDocument->setComment(mCommentEdit->text());
		mCommentEdit->setModified(false);
	}
}


void MetaEdit::setEmptyText() {
	Q_ASSERT(mDocument->commentState()!=Document::NONE);
	if (mDocument->commentState()==Document::WRITABLE) {
		if (mCommentEdit->hasFocus()) {
			setComment("");
		} else {
			setMessage(i18n("Type here to add a comment to this image."));
		}
	} else {
		setMessage(i18n("No comment available."));
	}
}


/**
 * Use mCommentEdit to show the comment and let the user edit it
 */
void MetaEdit::setComment(const QString& comment) {
	Q_ASSERT(mDocument->commentState()!=Document::NONE);
	mCommentEdit->setTextFormat(QTextEdit::PlainText);
	mCommentEdit->setReadOnly(mDocument->commentState()==Document::READ_ONLY);
	mCommentEdit->setText(comment);
}


/**
 * Use mCommentEdit to display a read-only message
 */
void MetaEdit::setMessage(const QString& msg) {
	mCommentEdit->setTextFormat(QTextEdit::RichText);
	mCommentEdit->setReadOnly(true);
	mCommentEdit->setText(QString("<i>%1</i>").arg(msg));
}

} // namespace
