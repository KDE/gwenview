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
#ifndef FULLSCREENWIDGET_H
#define FULLSCREENWIDGET_H

// Qt
#include <QWidget>

// KDE

// Local

class QLabel;
class QStringList;

class KActionCollection;
class KUrl;

namespace Gwenview {

class SlideShow;

class ThumbnailBarView;


class FullScreenContentPrivate;
/**
 * The content of the fullscreen bar
 */
class FullScreenContent : public QObject {
	Q_OBJECT
public:
	FullScreenContent(QWidget* parent, KActionCollection*, SlideShow*);
	~FullScreenContent();

	ThumbnailBarView* thumbnailBar() const;

	void setCurrentUrl(const KUrl&);

protected:
	bool eventFilter(QObject*, QEvent*);

private Q_SLOTS:
	void updateInformationLabel();
	void updateMetaInfoDialog();
	void configureInformationLabel();
	void slotPreferredMetaInfoKeyListChanged(const QStringList& list);
	void showFullScreenConfigDialog();
	void updateSlideShowIntervalLabel();

private:
	FullScreenContentPrivate* const d;
};


} // namespace

#endif /* FULLSCREENWIDGET_H */
