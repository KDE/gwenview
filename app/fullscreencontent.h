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
#include <QPointer>
#include <QWidget>

// KDE

// Local
#include "ui_fullscreenconfigwidget.h"
#include "imagemetainfodialog.h"
#include <lib/document/document.h>

class QStringList;

class KActionCollection;
class QUrl;

namespace Gwenview
{

class FullScreenBar;
class FullScreenToolBar;
class ShadowFilter;
class SlideShow;
class GvCore;
class ThumbnailBarView;


class FullScreenConfigWidget : public QWidget, public Ui_FullScreenConfigWidget
{
public:
    FullScreenConfigWidget(QWidget* parent=0)
    : QWidget(parent)
    {
        setupUi(this);
    }
};


/**
 * The content of the fullscreen bar
 */
class FullScreenContent : public QObject
{
    Q_OBJECT
public:
    FullScreenContent(QObject* parent, GvCore* gvCore);

    void init(KActionCollection*, QWidget* autoHideParentWidget, SlideShow*);

    ThumbnailBarView* thumbnailBar() const;

    void setDistractionFreeMode(bool);

    void setFullScreenMode(bool);

public Q_SLOTS:
    void setCurrentUrl(const QUrl&);

private Q_SLOTS:
    void updateCurrentUrlWidgets();
    void updateInformationLabel();
    void updateMetaInfoDialog();
    void showImageMetaInfoDialog();
    void slotImageMetaInfoDialogClosed();
    void slotPreferredMetaInfoKeyListChanged(const QStringList& list);
    void showOptionsMenu();
    void updateSlideShowIntervalLabel();
    void setFullScreenBarHeight(int value);
    void slotShowThumbnailsToggled(bool value);
    void slotViewModeActionToggled(bool value);
    void adjustSize();
    void updateDocumentCountLabel();

private:
    KActionCollection* mActionCollection;
    FullScreenBar* mAutoHideContainer;
    SlideShow* mSlideShow;
    QWidget* mContent;
    FullScreenToolBar* mToolBar;
    FullScreenToolBar* mRightToolBar;
    ThumbnailBarView* mThumbnailBar;
    QLabel* mInformationLabel;
    QLabel* mDocumentCountLabel;
    QWidget* mInformationContainer;
    ShadowFilter* mToolBarShadow;
    ShadowFilter* mRightToolBarShadow;
    ShadowFilter* mInformationContainerShadow;
    Document::Ptr mCurrentDocument;
    QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
    QPointer<FullScreenConfigWidget> mConfigWidget;
    QAction * mOptionsAction;
    GvCore* mGvCore;
    int mMinimumThumbnailBarHeight;

    bool mViewPageVisible;

    void createOptionsAction();
    void updateContainerAppearance();
    void updateLayout();
    void setupThumbnailBarStyleSheet();
};

} // namespace

#endif /* FULLSCREENCONTENT_H */
