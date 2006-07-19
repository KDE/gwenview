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
#ifndef FILEOPOBJECT_H
#define FILEOPOBJECT_H

// Qt includes
#include <qobject.h>
#include <qstring.h>

// KDE includes
#include <kio/job.h>
#include <kurl.h>

class QWidget;


namespace Gwenview {
/**
 * This class is a base class for wrappers to KIO slaves asynchronous
 * file operations. These classes handle all steps of a file operation :
 * - asking the user what to do with a file
 * - performing the operation
 * - showing result dialogs
 *
 * All these classes are used by the @FileOperation namespace-like class
 */
class FileOpObject : public QObject {
Q_OBJECT
public:
	FileOpObject(const KURL&,QWidget* parent=0L);
	FileOpObject(const KURL::List&,QWidget* parent=0L);
	virtual void operator()()=0;

signals:
	void success();

protected slots:
	virtual void slotResult(KIO::Job*);

protected:
	QWidget* mParent;
	KURL::List mURLList;

    void polishJob(KIO::Job*);
};


class FileOpCopyToObject : public FileOpObject {
Q_OBJECT
public:
	FileOpCopyToObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpCopyToObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};

class FileOpLinkToObject : public FileOpObject {
Q_OBJECT
public:
	FileOpLinkToObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpLinkToObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};

class FileOpMoveToObject : public FileOpObject {
Q_OBJECT
public:
	FileOpMoveToObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpMoveToObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};

class FileOpMakeDirObject : public FileOpObject {
Q_OBJECT
public:
    FileOpMakeDirObject(const KURL& url, QWidget* parent=0L) : FileOpObject(url, parent) {}
	void operator()();
};

class FileOpDelObject : public FileOpObject {
Q_OBJECT
public:
	FileOpDelObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpDelObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};


class FileOpTrashObject : public FileOpObject {
Q_OBJECT
public:
	FileOpTrashObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpTrashObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};


class FileOpRealDeleteObject : public FileOpObject {
Q_OBJECT
public:
	FileOpRealDeleteObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	FileOpRealDeleteObject(const KURL::List& urlList,QWidget* parent=0L) : FileOpObject(urlList,parent) {}
	void operator()();
};


class FileOpRenameObject : public FileOpObject {
Q_OBJECT
public:
	FileOpRenameObject(const KURL& url,QWidget* parent=0L) : FileOpObject(url,parent) {}
	void operator()();

signals:
	void renamed(const QString& newName);

protected slots:
	virtual void slotResult(KIO::Job*);

private:
	QString mNewFilename;
};


} // namespace
#endif

