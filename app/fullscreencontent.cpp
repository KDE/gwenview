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
// Self
#include "fullscreencontent.moc"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QWidgetAction>

// KDE
#include <KActionCollection>
#include <KActionMenu>
#include <KLocale>
#include <KMenu>
#include <KToolBar>

// Local
#include "imagemetainfodialog.h"
#include "ui_fullscreenconfigdialog.h"
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/eventwatcher.h>
#include <lib/fullscreenbar.h>
#include <lib/fullscreentheme.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/slideshow.h>

namespace Gwenview
{

class FullScreenConfigDialog : public QWidget, public Ui_FullScreenConfigDialog
{
public:
    FullScreenConfigDialog(QWidget* parent=0)
    : QWidget(parent)
    {
        setupUi(this);
    }
};

struct FullScreenContentPrivate
{
    FullScreenContent* q;
    FullScreenBar* mAutoHideContainer;
    SlideShow* mSlideShow;
    QWidget* mContent;
    KToolBar* mToolBar;
    ThumbnailBarView* mThumbnailBar;
    QLabel* mInformationLabel;
    Document::Ptr mCurrentDocument;
    QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
    QPointer<FullScreenConfigDialog> mFullScreenConfigDialog;
    KActionMenu* mOptionsAction;

    bool mFullScreenMode;
    bool mViewPageVisible;

    void createOptionsAction()
    {
        mOptionsAction = new KActionMenu(q);
        mOptionsAction->setPriority(QAction::LowPriority);
        mOptionsAction->setDelayed(false);
        mOptionsAction->setIcon(KIcon("configure"));
        mOptionsAction->setToolTip(i18nc("@info:tooltip", "Configure Full Screen Mode"));
        QObject::connect(mOptionsAction->menu(), SIGNAL(aboutToShow()), q, SLOT(slotAboutToShowOptionsMenu()));
    }

    void adjustBarWidth()
    {
        if (GwenviewConfig::showFullScreenThumbnails()) {
            return;
        }
        q->adjustSize();
    }

    void updateContainerAppearance()
    {
        bool wasAutoHide = mContent->parentWidget() == mAutoHideContainer;
        if (!mFullScreenMode) {
            if (wasAutoHide) {
                q->layout()->addWidget(mContent);
            }
            q->hide();
            return;
        }

        mThumbnailBar->setVisible(mViewPageVisible && GwenviewConfig::showFullScreenThumbnails());
        if (mViewPageVisible) {
            mAutoHideContainer->layout()->addWidget(mContent);
            mContent->show();
            mAutoHideContainer->adjustSize();
            q->hide();
        } else {
            q->show();
            q->layout()->addWidget(mContent);
        }
        mAutoHideContainer->setActivated(mViewPageVisible);
    }
};

FullScreenContent::FullScreenContent(QWidget* parent)
: QWidget(parent)
, d(new FullScreenContentPrivate)
{
    d->q = this;
    d->mFullScreenMode = false;
    d->mViewPageVisible = false;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

FullScreenContent::~FullScreenContent()
{
    delete d;
}

void FullScreenContent::init(KActionCollection* actionCollection, QWidget* autoHideParentWidget, SlideShow* slideShow)
{
    d->mSlideShow = slideShow;
    connect(actionCollection->action("view"), SIGNAL(toggled(bool)),
        SLOT(slotViewModeActionToggled(bool)));

    // mAutoHideContainer
    d->mAutoHideContainer = new FullScreenBar(autoHideParentWidget);
    d->mAutoHideContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout* layout = new QVBoxLayout(d->mAutoHideContainer);
    layout->setMargin(0);
    layout->setSpacing(0);

    // mContent
    d->mContent = new QWidget;
    d->mContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->mContent->setAutoFillBackground(true);
    EventWatcher::install(d->mContent, QEvent::Show, this, SLOT(updateCurrentUrlWidgets()));
    layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->mContent);

    // mInformationLabel
    d->mInformationLabel = new QLabel;
    d->mInformationLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->mInformationLabel->setWordWrap(true);

    d->createOptionsAction();

    // mToolBar
    d->mToolBar = new KToolBar(d->mContent);
    d->mToolBar->setIconDimensions(KIconLoader::SizeMedium);
    QStringList actions = QStringList()
        << "go_start_page"
        << "-"
        << "browse"
        << "view"
        << "-"
        << "go_previous"
        << "toggle_slideshow"
        << "go_next"
        << "-"
        << "rotate_left"
        << "rotate_right"
        << "-"
        << "label"
        << "configure"
        << "leave_fullscreen"
        ;
    Q_FOREACH(const QString& name, actions) {
        if (name == "-") {
            d->mToolBar->addSeparator();
        } else if (name == "label") {
            d->mToolBar->addWidget(d->mInformationLabel);
        } else if (name == "configure") {
            d->mToolBar->addAction(d->mOptionsAction);
            d->mOptionsAction->associatedWidgets().first();
        } else {
            d->mToolBar->addAction(actionCollection->action(name));
        }
    }

    // Thumbnail bar
    d->mThumbnailBar = new ThumbnailBarView(d->mContent);
    ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
    d->mThumbnailBar->setItemDelegate(delegate);
    d->mThumbnailBar->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Content Layout
    layout = new QVBoxLayout(d->mContent);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->mToolBar);
    layout->addWidget(d->mThumbnailBar);

    d->updateContainerAppearance();
}

ThumbnailBarView* FullScreenContent::thumbnailBar() const
{
    return d->mThumbnailBar;
}

void FullScreenContent::setCurrentUrl(const KUrl& url)
{
    d->mCurrentDocument = DocumentFactory::instance()->load(url);
    connect(d->mCurrentDocument.data(), SIGNAL(metaInfoUpdated()),
            SLOT(updateCurrentUrlWidgets()));
    updateCurrentUrlWidgets();
}

void FullScreenContent::updateInformationLabel()
{
    if (!d->mCurrentDocument) {
        kWarning() << "No document";
        return;
    }

    if (!d->mInformationLabel->isVisible()) {
        return;
    }

    ImageMetaInfoModel* model = d->mCurrentDocument->metaInfo();

    QStringList valueList;
    Q_FOREACH(const QString & key, GwenviewConfig::fullScreenPreferredMetaInfoKeyList()) {
        const QString value = model->getValueForKey(key);
        if (value.length() > 0) {
            valueList << value;
        }
    }
    QString text = valueList.join(i18nc("@item:intext fullscreen meta info separator", ", "));

    d->mInformationLabel->setText(text);

    d->adjustBarWidth();
}

void FullScreenContent::setCurrentFullScreenTheme(const QString& themeName)
{
    FullScreenTheme::setCurrentThemeName(themeName);
}

void FullScreenContent::updateCurrentUrlWidgets()
{
    updateInformationLabel();
    updateMetaInfoDialog();
}

void FullScreenContent::showImageMetaInfoDialog()
{
    if (!d->mImageMetaInfoDialog) {
        d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mInformationLabel);
        // Do not let the fullscreen theme propagate to this dialog for now,
        // it's already quite complicated to create a theme
        d->mImageMetaInfoDialog->setStyle(QApplication::style());
        d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(d->mImageMetaInfoDialog, SIGNAL(preferredMetaInfoKeyListChanged(QStringList)),
                SLOT(slotPreferredMetaInfoKeyListChanged(QStringList)));
        connect(d->mImageMetaInfoDialog, SIGNAL(destroyed()),
                SLOT(slotImageMetaInfoDialogClosed()));
    }
    if (d->mCurrentDocument) {
        d->mImageMetaInfoDialog->setMetaInfo(d->mCurrentDocument->metaInfo(), GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
    }
    d->mAutoHideContainer->setAutoHidingEnabled(false);
    d->mImageMetaInfoDialog->show();
}

void FullScreenContent::slotPreferredMetaInfoKeyListChanged(const QStringList& list)
{
    GwenviewConfig::setFullScreenPreferredMetaInfoKeyList(list);
    GwenviewConfig::self()->writeConfig();
    updateInformationLabel();
}

void FullScreenContent::updateMetaInfoDialog()
{
    if (!d->mImageMetaInfoDialog) {
        return;
    }
    ImageMetaInfoModel* model = d->mCurrentDocument ? d->mCurrentDocument->metaInfo() : 0;
    d->mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
}

static QString formatSlideShowIntervalText(int value)
{
    return i18ncp("Slideshow interval in seconds", "%1 sec", "%1 secs", value);
}

void FullScreenContent::updateSlideShowIntervalLabel()
{
    Q_ASSERT(d->mFullScreenConfigDialog);
    int value = d->mFullScreenConfigDialog->mSlideShowIntervalSlider->value();
    QString text = formatSlideShowIntervalText(value);
    d->mFullScreenConfigDialog->mSlideShowIntervalLabel->setText(text);
}

void FullScreenContent::setFullScreenBarHeight(int value)
{
    d->mAutoHideContainer->setFixedHeight(value);
    GwenviewConfig::setFullScreenBarHeight(value);
}

void FullScreenContent::slotAboutToShowOptionsMenu()
{
    FullScreenConfigDialog* dialog;
    if (!d->mFullScreenConfigDialog) {
        d->mFullScreenConfigDialog = new FullScreenConfigDialog;
        dialog = d->mFullScreenConfigDialog;
        connect(dialog->mSlideShowLoopCheckBox, SIGNAL(toggled(bool)),
                d->mSlideShow->loopAction(), SLOT(trigger()));
        connect(dialog->mSlideShowRandomCheckBox, SIGNAL(toggled(bool)),
                d->mSlideShow->randomAction(), SLOT(trigger()));
        connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
                d->mSlideShow, SLOT(setInterval(int)));
        connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
                SLOT(updateSlideShowIntervalLabel()));
        connect(dialog->mConfigureDisplayedInformationButton, SIGNAL(clicked()),
                SLOT(showImageMetaInfoDialog()));
        connect(dialog->mShowThumbnailsCheckBox, SIGNAL(toggled(bool)),
                SLOT(slotShowThumbnailsToggled(bool)));
        connect(dialog->mHeightSlider, SIGNAL(valueChanged(int)),
                this, SLOT(setFullScreenBarHeight(int)));

        QWidgetAction* action = new QWidgetAction(d->mOptionsAction->menu());
        action->setDefaultWidget(dialog);
        d->mOptionsAction->menu()->addAction(action);
    } else {
        dialog = d->mFullScreenConfigDialog;
    }

    // Checkboxes
    dialog->mSlideShowLoopCheckBox->setChecked(d->mSlideShow->loopAction()->isChecked());
    dialog->mSlideShowRandomCheckBox->setChecked(d->mSlideShow->randomAction()->isChecked());

    // Interval slider
    dialog->mSlideShowIntervalSlider->setValue(int(GwenviewConfig::interval()));

    // Interval label
    QString text = formatSlideShowIntervalText(88);
    int width = dialog->mSlideShowIntervalLabel->fontMetrics().width(text);
    dialog->mSlideShowIntervalLabel->setFixedWidth(width);
    updateSlideShowIntervalLabel();

    // Thumbnails check box
    dialog->mShowThumbnailsCheckBox->setChecked(GwenviewConfig::showFullScreenThumbnails());

    // Height slider
    dialog->mHeightSlider->setValue(height());
}

void FullScreenContent::setFullScreenMode(bool fullScreenMode)
{
    d->mFullScreenMode = fullScreenMode;
    d->updateContainerAppearance();
}

void FullScreenContent::setDistractionFreeMode(bool distractionFreeMode)
{
    d->mAutoHideContainer->setAutoHidingEnabled(!distractionFreeMode);
}

void FullScreenContent::slotImageMetaInfoDialogClosed()
{
    d->mAutoHideContainer->setAutoHidingEnabled(true);
}

void FullScreenContent::slotShowThumbnailsToggled(bool value)
{
    GwenviewConfig::setShowFullScreenThumbnails(value);
    GwenviewConfig::self()->writeConfig();
    if (d->mViewPageVisible) {
        d->mThumbnailBar->setVisible(value);
        d->mAutoHideContainer->adjustSize();
    }
}

void FullScreenContent::slotViewModeActionToggled(bool value)
{
    d->mViewPageVisible = value;
    d->updateContainerAppearance();
}

} // namespace
