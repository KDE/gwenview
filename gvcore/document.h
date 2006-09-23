// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2006 Aurelien Gateau
 
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
#ifndef DOCUMENT_H
#define DOCUMENT_H

// Qt 
#include <qcstring.h>
#include <qobject.h>
#include <qimage.h>

// KDE 
#include <kurl.h>
#include <kprinter.h>

// Local 
#include "imageutils/orientation.h"
#include "mimetypeutils.h"
#include "libgwenview_export.h"
namespace KIO { class Job; }

namespace Gwenview {
class DocumentPrivate;
class DocumentImpl;

/**
 * The application document.
 * It knows what the current url is and will emit signals when
 * loading/loaded/modified...
 * 
 * The ordering of loading() and loaded() signals is:
 * - setURL() is called
 * - URL is stated
 * - loading() is emitted (may be skipped if no loading is needed, e.g. wrong URL)
 * - image is being loaded
 * - loaded() is emitted
 */
class LIBGWENVIEW_EXPORT Document : public QObject {
Q_OBJECT
public:
	enum CommentState { NONE=0, READ_ONLY=1, WRITABLE=2 };
	
	Document(QObject*);
	~Document();

	// Properties
	const QImage& image() const;
	KURL url() const;
	KURL dirURL() const;
	QString filename() const;
	const QCString& imageFormat() const;
	int fileSize() const;
	MimeTypeUtils::Kind urlKind() const;

	// Convenience methods
	bool isNull() const { return image().isNull(); }
	int width() const { return image().width(); }
	int height() const { return image().height(); }

	Document::CommentState commentState() const;
	QString comment() const;
	void setComment(const QString&);

	int duration() const;
	
public slots:
	void setURL(const KURL&);
	void setDirURL(const KURL&);
	void reload();

	/**
	 * Save to the current file.
	 */
	void save();
	void saveAs();
	
	/** print the selected file */
	void print(KPrinter *pPrinter);
	
	/**
	 * If the image has been modified, prompt the user to save the changes.
	 */
	void saveBeforeClosing();

	// "Image manipulation"
	void transform(ImageUtils::Orientation);

signals:
	/**
	 * Emitted when the class starts to load the image.
	 */
	void loading();

	/**
	 * Emitted when the class has finished loading the image.
	 * Also emitted if the image could not be loaded.
	 */
	void loaded(const KURL& url);

	/**
	 * Emitted when the image has been modified.
	 */
	void modified();

	/**
	 * Emitted when the image has been saved on disk.
	 */
	void saved(const KURL& url);

	/**
	 * Emitted when the image has been reloaded.
	 */ 
	void reloaded(const KURL& url);

	/**
	 * Emitted to show a part of the image must be refreshed
	 */
	void rectUpdated(const QRect& rect);

	/**
	 * Emitted when the size is known
	 */
	void sizeUpdated();

	/**
	 * Emitted when something goes wrong, like when save fails
	 */
	void errorHappened(const QString& message);

private slots:
	void slotStatResult(KIO::Job*); 
	void slotFinished(bool success);
	void slotLoading();
	void slotLoaded();
	
private:
	friend class DocumentImpl;
	friend class DocumentPrivate;

	DocumentPrivate* d;

	// These methods are used by DocumentImpl and derived
	void switchToImpl(DocumentImpl*);
	void setImage(QImage);
	void setImageFormat(const QCString&);
	void setFileSize(int); 
	
	void reset();
	void load();
	void doPaint(KPrinter *pPrinter, QPainter *p);

	/**
	 * The returned string is null if the image was successfully saved,
	 * otherwise it's the translated error message.
	 */
	QString saveInternal(const KURL& url, const QCString& format);

	Document(const Document&);
	Document &operator=(const Document&);
};


} // namespace
#endif

