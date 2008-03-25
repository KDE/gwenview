// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef LOADINGTHREAD_H
#define LOADINGTHREAD_H

// Exiv2
#include <exiv2/image.hpp>

// Qt
#include <QSize>
#include <QThread>

// KDE

// Local

class QImage;

namespace Gwenview {

class JpegContent;

class LoadingThreadPrivate;
class LoadingThread : public QThread {
	Q_OBJECT
public:
	enum Task {
		TaskLoadMetaData=1,
		TaskLoadImage=2
	};
	Q_DECLARE_FLAGS(Tasks, Task);

	LoadingThread();
	~LoadingThread();

	void cancel();

	void setData(const QByteArray&);

	void setTasks(Tasks);

	const QByteArray& format() const;

	const QImage& image() const;

	QSize size() const;

	Exiv2::Image::AutoPtr popExiv2Image();

	JpegContent* popJpegContent();

Q_SIGNALS:
	void metaDataLoaded();

	/**
	 * This signal is emitted if the loader was not able to determine image
	 * size without loading the whole image
	 */
	void sizeUpdated();

protected:
	virtual void run();

private:
	LoadingThreadPrivate* const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LoadingThread::Tasks);


} // namespace

#endif /* LOADINGTHREAD_H */
