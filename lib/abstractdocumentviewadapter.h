// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef ABSTRACTDOCUMENTVIEWADAPTER_H
#define ABSTRACTDOCUMENTVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>
#include <QPoint>

// KDE

// Local
#include <lib/document/document.h>

class QSize;
class QWidget;

class KUrl;

namespace Gwenview {


class ImageView;


/**
 * Classes inherit from this class so that they can be used inside the
 * DocumentPanel.
 */
class GWENVIEWLIB_EXPORT AbstractDocumentViewAdapter : public QObject {
	Q_OBJECT
public:
	AbstractDocumentViewAdapter(QWidget*);
	virtual ~AbstractDocumentViewAdapter();

	QWidget* widget() const { return mWidget; }

	virtual ImageView* imageView() const { return 0; }

	/**
	 * @defgroup zooming functions
	 * @{
	 */
	virtual bool canZoom() const { return false; }

	virtual void setZoomToFit(bool) {}

	virtual bool zoomToFit() const { return false; }

	virtual qreal zoom() const { return 0; }

	virtual void setZoom(qreal /*zoom*/, const QPoint& /*center*/ = QPoint(-1, -1)) {}
	/** @} */

	virtual Document::Ptr document() const = 0;
	// FIXME: Replace with "document" property
	virtual void openUrl(const KUrl&) = 0;

	virtual void loadConfig() {}

protected:
	void setWidget(QWidget* widget) { mWidget = widget; }

Q_SIGNALS:
	void resizeRequested(const QSize&);
	void previousImageRequested();
	void nextImageRequested();
	void completed();

private:
	QWidget* mWidget;
};


} // namespace

#endif /* ABSTRACTDOCUMENTVIEWADAPTER_H */
