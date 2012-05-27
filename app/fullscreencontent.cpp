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
#include <QToolButton>
#include <QWidgetAction>

// KDE
#include <KActionCollection>
#include <KActionMenu>
#include <KLocale>
#include <KMenu>
#include <KToolBar>

// Local
#include "imagemetainfodialog.h"
#include "ui_fullscreenconfigwidget.h"
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/eventwatcher.h>
#include <lib/fullscreenbar.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/slideshow.h>

namespace Gwenview
{

class FullScreenConfigWidget : public QWidget, public Ui_FullScreenConfigWidget
{
public:
    FullScreenConfigWidget(QWidget* parent=0)
    : QWidget(parent)
    {
        setupUi(this);
    }
};

struct FullScreenContentPrivate
{
    FullScreenContent* q;
    KActionCollection* mActionCollection;
    FullScreenBar* mAutoHideContainer;
    SlideShow* mSlideShow;
    QWidget* mContent;
    KToolBar* mToolBar;
    KToolBar* mRightToolBar;
    ThumbnailBarView* mThumbnailBar;
    QLabel* mInformationLabel;
    Document::Ptr mCurrentDocument;
    QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
    QPointer<FullScreenConfigWidget> mConfigWidget;
    KActionMenu* mOptionsAction;

    bool mFullScreenMode;
    bool mViewPageVisible;

    void createOptionsAction()
    {
        mOptionsAction = new KActionMenu(q);
        mOptionsAction->setPriority(QAction::LowPriority);
        mOptionsAction->setDelayed(false);
        mOptionsAction->setIcon(KIcon("fs-configure"));
        mOptionsAction->setToolTip(i18nc("@info:tooltip", "Configure full screen mode"));
        QObject::connect(mOptionsAction->menu(), SIGNAL(aboutToShow()), q, SLOT(slotAboutToShowOptionsMenu()));
    }

    void updateContainerAppearance()
    {
        if (!mFullScreenMode || !mViewPageVisible) {
            mAutoHideContainer->setActivated(false);
            return;
        }

        mThumbnailBar->setVisible(GwenviewConfig::showFullScreenThumbnails());
        mAutoHideContainer->adjustSize();
        mAutoHideContainer->setActivated(true);
    }

    void updateLayout()
    {
        delete mContent->layout();

        if (GwenviewConfig::showFullScreenThumbnails()) {
            mInformationLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            mRightToolBar->setOrientation(Qt::Vertical);

            QGridLayout* layout = new QGridLayout(mContent);
            layout->setMargin(0);
            layout->setSpacing(0);
            layout->addWidget(mToolBar, 0, 0);
            layout->addWidget(mThumbnailBar, 0, 1, 2, 1);
            layout->addWidget(mRightToolBar, 0, 2, 2, 1);
            layout->addWidget(mInformationLabel, 1, 0);

            mThumbnailBar->setFixedHeight(GwenviewConfig::fullScreenBarHeight());
            mAutoHideContainer->setFixedHeight(GwenviewConfig::fullScreenBarHeight());
        } else {
            mInformationLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            mRightToolBar->setOrientation(Qt::Horizontal);

            QHBoxLayout* layout = new QHBoxLayout(mContent);
            layout->setMargin(0);
            layout->setSpacing(0);
            layout->addWidget(mToolBar);
            layout->addWidget(mInformationLabel);
            layout->addWidget(mRightToolBar);

            mAutoHideContainer->setFixedHeight(mContent->sizeHint().height());
        }
    }
};

FullScreenContent::FullScreenContent(QObject* parent)
: QObject(parent)
, d(new FullScreenContentPrivate)
{
    d->q = this;
    d->mFullScreenMode = false;
    d->mViewPageVisible = false;
}

FullScreenContent::~FullScreenContent()
{
    delete d;
}

void FullScreenContent::init(KActionCollection* actionCollection, QWidget* autoHideParentWidget, SlideShow* slideShow)
{
    d->mSlideShow = slideShow;
    d->mActionCollection = actionCollection;
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
    layout->addWidget(d->mContent);

    // mInformationLabel
    d->mInformationLabel = new QLabel;
    d->mInformationLabel->setWordWrap(true);
    d->mInformationLabel->setContentsMargins(6, 0, 6, 0);

    d->createOptionsAction();

    // mToolBar
    d->mToolBar = new KToolBar(d->mContent);
    d->mToolBar->setIconDimensions(KIconLoader::SizeMedium);
    d->mToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    d->mToolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    #define addAction(name) d->mToolBar->addAction(actionCollection->action(name))
    addAction("browse");
    addAction("view");
    d->mToolBar->addSeparator();
    addAction("go_previous");
    addAction("toggle_slideshow");
    addAction("go_next");
    d->mToolBar->addSeparator();
    addAction("rotate_left");
    addAction("rotate_right");
    #undef addAction

    // Thumbnail bar
    d->mThumbnailBar = new ThumbnailBarView(d->mContent);
    ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
    d->mThumbnailBar->setItemDelegate(delegate);
    d->mThumbnailBar->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Right bar
    d->mRightToolBar = new KToolBar(d->mContent);
    d->mRightToolBar->setIconDimensions(KIconLoader::SizeMedium);
    d->mRightToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    d->mRightToolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->mRightToolBar->addAction(d->mActionCollection->action("leave_fullscreen"));
    d->mRightToolBar->addAction(d->mOptionsAction);


    d->updateLayout();

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
    Q_ASSERT(d->mConfigWidget);
    int value = d->mConfigWidget->mSlideShowIntervalSlider->value();
    QString text = formatSlideShowIntervalText(value);
    d->mConfigWidget->mSlideShowIntervalLabel->setText(text);
}

void FullScreenContent::setFullScreenBarHeight(int value)
{
    d->mThumbnailBar->setFixedHeight(value);
    d->mAutoHideContainer->setFixedHeight(value);
    GwenviewConfig::setFullScreenBarHeight(value);
}

void FullScreenContent::slotAboutToShowOptionsMenu()
{
    // This will delete d->mConfigWidget
    d->mOptionsAction->menu()->clear();
    Q_ASSERT(!d->mConfigWidget);

    d->mConfigWidget = new FullScreenConfigWidget;
    FullScreenConfigWidget* widget = d->mConfigWidget;

    // Put widget in the menu
    QWidgetAction* action = new QWidgetAction(d->mOptionsAction->menu());
    action->setDefaultWidget(widget);
    d->mOptionsAction->menu()->addAction(action);

    // Slideshow checkboxes
    widget->mSlideShowLoopCheckBox->setChecked(d->mSlideShow->loopAction()->isChecked());
    connect(widget->mSlideShowLoopCheckBox, SIGNAL(toggled(bool)),
            d->mSlideShow->loopAction(), SLOT(trigger()));

    widget->mSlideShowRandomCheckBox->setChecked(d->mSlideShow->randomAction()->isChecked());
    connect(widget->mSlideShowRandomCheckBox, SIGNAL(toggled(bool)),
            d->mSlideShow->randomAction(), SLOT(trigger()));

    // Interval slider
    widget->mSlideShowIntervalSlider->setValue(int(GwenviewConfig::interval()));
    connect(widget->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
            d->mSlideShow, SLOT(setInterval(int)));
    connect(widget->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
            SLOT(updateSlideShowIntervalLabel()));

    // Interval label
    QString text = formatSlideShowIntervalText(88);
    int width = widget->mSlideShowIntervalLabel->fontMetrics().width(text);
    widget->mSlideShowIntervalLabel->setFixedWidth(width);
    updateSlideShowIntervalLabel();

    // Image information
    connect(widget->mConfigureDisplayedInformationButton, SIGNAL(clicked()),
            SLOT(showImageMetaInfoDialog()));

    // Thumbnails
    widget->mThumbnailGroupBox->setVisible(d->mViewPageVisible);
    if (d->mViewPageVisible) {
        widget->mShowThumbnailsCheckBox->setChecked(GwenviewConfig::showFullScreenThumbnails());
        widget->mHeightSlider->setMinimum(d->mRightToolBar->sizeHint().height());
        widget->mHeightSlider->setValue(d->mThumbnailBar->height());
        connect(widget->mShowThumbnailsCheckBox, SIGNAL(toggled(bool)),
                SLOT(slotShowThumbnailsToggled(bool)));
        connect(widget->mHeightSlider, SIGNAL(valueChanged(int)),
                SLOT(setFullScreenBarHeight(int)));
    }
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
    d->mThumbnailBar->setVisible(value);
    d->updateLayout();
    d->mContent->adjustSize();
    d->mAutoHideContainer->adjustSize();
}

void FullScreenContent::slotViewModeActionToggled(bool value)
{
    d->mViewPageVisible = value;
    d->updateContainerAppearance();
}

} // namespace
