// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#ifndef FULLSCREENCONTENT_H
#define FULLSCREENCONTENT_H

// Qt
#include <QWidget>

// KDE

// Local

class QStringList;

class KActionCollection;
class KUrl;

namespace Gwenview
{

class FullScreenBar;
class SlideShow;

class ThumbnailBarView;

struct FullScreenContentPrivate;
/**
 * The content of the fullscreen bar
 */
class FullScreenContent : public QObject
{
    Q_OBJECT
public:
    FullScreenContent(QObject* parent);
    ~FullScreenContent();

    void init(KActionCollection*, QWidget* autoHideParentWidget, SlideShow*);

    ThumbnailBarView* thumbnailBar() const;

    void setDistractionFreeMode(bool);

    void setFullScreenMode(bool);

public Q_SLOTS:
    void setCurrentUrl(const KUrl&);

private Q_SLOTS:
    void updateCurrentUrlWidgets();
    void updateInformationLabel();
    void updateMetaInfoDialog();
    void showImageMetaInfoDialog();
    void slotPaletteChanged();
    void slotImageMetaInfoDialogClosed();
    void slotPreferredMetaInfoKeyListChanged(const QStringList& list);
    void showOptionsMenu();
    void updateSlideShowIntervalLabel();
    void setFullScreenBarHeight(int value);
    void slotShowThumbnailsToggled(bool value);
    void slotViewModeActionToggled(bool value);

private:
    FullScreenContentPrivate* const d;
};

} // namespace

#endif /* FULLSCREENCONTENT_H */
