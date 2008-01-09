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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

// Qt
#include <QStackedWidget>

class KStatusBar;
class KUrl;

namespace KParts { class Part; }

namespace Gwenview {

class DocumentViewPrivate;
class ImageViewPart;

/**
 * Holds the active document view, or show a message if there is no active
 * document
 */
class DocumentView : public QStackedWidget {
	Q_OBJECT
public:
	DocumentView(QWidget* parent);
	~DocumentView();

	/**
	 * Reset the view
	 */
	void reset();

	KStatusBar* statusBar() const;

	virtual QSize sizeHint() const;

	/**
	 * Returns the url of the current document, or an invalid url if unknown
	 */
	KUrl url() const;

	/**
	 * Opens url. Returns true on success
	 */
	bool openUrl(const KUrl& url);

	bool currentDocumentIsRasterImage() const;

	bool isEmpty() const;

	void setViewBackgroundColor(const QColor&);

	/**
	 * Returns the image view part, if the current part really is an
	 * ImageViewPart.
	 */
	ImageViewPart* imageViewPart() const;

Q_SIGNALS:
	/**
	 * Emitted whenever the part changes. Main window should call createGui on
	 * it.
	 */
	void partChanged(KParts::Part*);

	/**
	 * Emitted when the part has finished loading
	 */
	void completed();

	void resizeRequested(const QSize&);

private:
	DocumentViewPrivate* const d;

	void createPartForUrl(const KUrl& url);
};

} // namespace

#endif /* DOCUMENTVIEW_H */
