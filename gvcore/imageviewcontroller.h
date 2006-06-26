// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
#ifndef IMAGEVIEWCONTROLLER_H
#define IMAGEVIEWCONTROLLER_H   

// Qt
#include <qobject.h>

// KDE
#include <kaction.h>

// Local
#include "libgwenview_export.h"

class QPoint;
class QWidget;

class KToolBar;

namespace Gwenview {


class Document;
class ImageView;


class LIBGWENVIEW_EXPORT ImageViewController : public QObject {
Q_OBJECT
public:
	ImageViewController(QWidget* parent, Document*, KActionCollection*);
	~ImageViewController();
	
	QWidget* widget() const;
	
	void setImageViewActions(const KActionPtrList&);

	void setFullScreen(bool);
	void setNormalCommonActions(const KActionPtrList&);
	void setFullScreenCommonActions(const KActionPtrList&);
	void setFocus();

protected:
	virtual bool eventFilter(QObject*, QEvent*);

public slots:
	void updateFromSettings();

signals:
	void requestHintDisplay(const QString&);
	void selectPrevious();
	void selectNext();
	void doubleClicked();

private slots:
	void slotLoaded();
	void openImageViewContextMenu(const QPoint&);
	void slotAutoHide();

private:
	struct Private;
	Private* d;
};

} // namespace

#endif /* IMAGEVIEWCONTROLLER_H */
