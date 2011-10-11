// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KDE
#include <kactioncollection.h>

// Local
#include <lib/document/document.h>

class KUrl;

namespace Gwenview {

class ImageView;
class SlideShow;
class ZoomWidget;

struct DocumentViewPrivate;

/**
 * This widget can display various documents, using an instance of
 * AbstractDocumentViewAdapter
 */
class GWENVIEWLIB_EXPORT DocumentView : public QWidget {
	Q_OBJECT
	Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
	Q_PROPERTY(bool zoomToFit READ zoomToFit WRITE setZoomToFit NOTIFY zoomToFitChanged)
	Q_PROPERTY(QPoint position READ position WRITE setPosition NOTIFY positionChanged)
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	enum {
		MaximumZoom = 16
	};
	DocumentView(QWidget* parent);
	~DocumentView();

	Document::Ptr document() const;

	KUrl url() const;

	void openUrl(const KUrl&);

	/**
	 * Tells the current adapter to load its config. Used when the user changed
	 * the config while the view was visible.
	 */
	void loadAdapterConfig();

	/**
	 * Unload the current adapter, if any
	 */
	void reset();

	/**
	 * Returns true if an adapter is loaded (note: adapters are also used to
	 * display error messages!)
	 */
	bool isEmpty() const;

	bool canZoom() const;

	qreal minimumZoom() const;

	qreal zoom() const;

	bool isCurrent() const;

	void setCurrent(bool);

	void setCompareMode(bool);

	bool zoomToFit() const;

	QPoint position() const;

	qreal opacity() const;

	void setOpacity(qreal value);

	/**
	 * Returns the ImageView of the current adapter, if it has one
	 */
	ImageView* imageView() const;

	void moveTo(const QRect&);
	void moveToAnimated(const QRect&);
	void fadeIn();
	void fadeOut();

public Q_SLOTS:
	void setZoom(qreal);

	void setZoomToFit(bool);

	void setPosition(const QPoint&);

Q_SIGNALS:
	/**
	 * Emitted when the part has finished loading
	 */
	void completed();

	void previousImageRequested();

	void nextImageRequested();

	void captionUpdateRequested(const QString&);

	void toggleFullScreenRequested();

	void videoFinished();

	void minimumZoomChanged(qreal);

	void zoomChanged(qreal);

	void adapterChanged();

	void focused(DocumentView*);

	void zoomToFitChanged(bool);

	void positionChanged();

	void hudTrashClicked(DocumentView*);
	void hudDeselectClicked(DocumentView*);

	void animationFinished(DocumentView*);

protected:
	virtual bool eventFilter(QObject*, QEvent* event);

	virtual void paintEvent(QPaintEvent*);
	void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
	void finishOpenUrl();
	void slotLoaded();
	void slotLoadingFailed();

	void zoomActualSize();

	void zoomIn(const QPoint& center = QPoint(-1,-1));
	void zoomOut(const QPoint& center = QPoint(-1,-1));

	void slotZoomChanged(qreal);

	void slotBusyChanged(const KUrl&, bool);

	void slotKeyPressed(Qt::Key key, bool pressed);

	void emitHudTrashClicked();
	void emitHudDeselectClicked();

	void slotAnimationFinished();

private:
	friend struct DocumentViewPrivate;
	DocumentViewPrivate* const d;

	void createAdapterForDocument();
};


} // namespace

#endif /* DOCUMENTVIEW_H */
