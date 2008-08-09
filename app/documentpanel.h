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
#ifndef DOCUMENTPANEL_H
#define DOCUMENTPANEL_H

// Qt
#include <QWidget>

class QPalette;

class KActionCollection;
class KUrl;

namespace Gwenview {

class DocumentPanelPrivate;
class ImageView;
class ThumbnailBarView;

/**
 * Holds the active document view and associated widgetry.
 */
class DocumentPanel : public QWidget {
	Q_OBJECT
public:
	DocumentPanel(QWidget* parent, KActionCollection*);
	~DocumentPanel();

	ThumbnailBarView* thumbnailBar() const;

	void loadConfig();

	void saveConfig();

	/**
	 * Reset the view
	 */
	void reset();

	void setFullScreenMode(bool fullScreen);

	void setNormalPalette(const QPalette&);

	void setStatusBarHeight(int);

	int statusBarHeight() const;

	virtual QSize sizeHint() const;

	/**
	 * Returns the url of the current document, or an invalid url if unknown
	 */
	KUrl url() const;

	void openUrl(const KUrl& url);

	bool currentDocumentIsRasterImage() const;

	bool isEmpty() const;

	/**
	 * Returns the image view, if the current adapter has one.
	 */
	ImageView* imageView() const;

Q_SIGNALS:

	/**
	 * Emitted when the part has finished loading
	 */
	void completed();

	void resizeRequested(const QSize&);

	void previousImageRequested();

	void nextImageRequested();

	void enterFullScreenRequested();

	void captionUpdateRequested(const QString&);

private Q_SLOTS:
	void setThumbnailBarVisibility(bool visible);

	void showContextMenu();

private:
	friend class DocumentPanelPrivate;
	DocumentPanelPrivate* const d;

	void createAdapterForUrl(const KUrl& url);
};

} // namespace

#endif /* DOCUMENTPANEL_H */
