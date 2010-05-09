// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "resizeimageoperation.h"

// Qt
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QtConcurrentRun>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "document/abstractdocumenteditor.h"
#include "document/document.h"
#include "document/documentjob.h"

namespace Gwenview {


struct ResizeImageOperationPrivate {
	int mSize;
	QImage mOriginalImage;
};


class ResizeJob : public DocumentJob {
public:
	ResizeJob(int size)
	: mSize(size)
	{}

	void threadedStart() {
		if (!document()->editor()) {
			setError(UserDefinedError + 1);
			setErrorText("!document->editor()");
			return;
		}
		QImage image = document()->image();
		image = image.scaled(mSize, mSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		document()->editor()->setImage(image);
		setError(NoError);
	}

protected:
	virtual void doStart() {
		QFuture<void> future = QtConcurrent::run(this, &ResizeJob::threadedStart);
		QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
		watcher->setFuture(future);
		connect(watcher, SIGNAL(finished()), SLOT(emitResult()));
	}

private:
	int mSize;
};


ResizeImageOperation::ResizeImageOperation(int size)
: d(new ResizeImageOperationPrivate) {
	d->mSize = size;
	setText(i18n("Resize"));
}


ResizeImageOperation::~ResizeImageOperation() {
	delete d;
}


void ResizeImageOperation::redo() {
	d->mOriginalImage = document()->image();
	document()->enqueueJob(new ResizeJob(d->mSize));
}


void ResizeImageOperation::undo() {
	if (!document()->editor()) {
		kWarning() << "!document->editor()";
		return;
	}
	document()->editor()->setImage(d->mOriginalImage);
}


} // namespace
