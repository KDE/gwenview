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
#include <QPainter>
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
#include <lib/shadowfilter.h>
#include <lib/slideshow.h>

namespace Gwenview
{

/**
 * A widget which behaves more or less like a QToolBar, but which uses real
 * widgets for the toolbar items. We need a real widget to be able to position
 * the option menu.
 */
class FullScreenToolBar : public QWidget
{
public:
    FullScreenToolBar(QWidget* parent = 0)
    : QWidget(parent)
    , mLayout(new QHBoxLayout(this))
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mLayout->setSpacing(0);
        mLayout->setMargin(0);
    }

    void addAction(QAction* action)
    {
        QToolButton* button = new QToolButton;
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setAutoRaise(true);
        const int extent = KIconLoader::SizeMedium;
        button->setIconSize(QSize(extent, extent));
        mLayout->addWidget(button);
    }

    void addSeparator()
    {
        mLayout->addSpacing(KDialog::spacingHint());
    }

    void addStretch()
    {
        mLayout->addStretch();
    }

    void setDirection(QBoxLayout::Direction direction)
    {
        mLayout->setDirection(direction);
    }

private:
    QBoxLayout* mLayout;
};

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
    FullScreenToolBar* mToolBar;
    FullScreenToolBar* mRightToolBar;
    ThumbnailBarView* mThumbnailBar;
    QLabel* mInformationLabel;
    Document::Ptr mCurrentDocument;
    QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
    QPointer<FullScreenConfigWidget> mConfigWidget;
    KAction* mOptionsAction;

    bool mFullScreenMode;
    bool mViewPageVisible;

    void createOptionsAction()
    {
        // We do not use a KActionMenu because:
        //
        // - It causes the button to show a small down arrow on its right,
        // which makes it wider
        //
        // - We can't control where the menu shows: in no-thumbnail-mode, the
        // menu should not be aligned to the right edge of the screen because
        // if the mode is changed to thumbnail-mode, we want the option button
        // to remain visible
        mOptionsAction = new KAction(q);
        mOptionsAction->setPriority(QAction::LowPriority);
        mOptionsAction->setIcon(KIcon("configure"));
        mOptionsAction->setToolTip(i18nc("@info:tooltip", "Configure full screen mode"));
        QObject::connect(mOptionsAction, SIGNAL(triggered()), q, SLOT(showOptionsMenu()));
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
            mRightToolBar->setDirection(QBoxLayout::TopToBottom);

            QHBoxLayout* layout = new QHBoxLayout(mContent);
            layout->setMargin(0);
            layout->setSpacing(0);
            QVBoxLayout* vLayout;

            // First column
            vLayout = new QVBoxLayout;
            vLayout->addWidget(mToolBar);
            vLayout->addWidget(mInformationLabel);
            layout->addLayout(vLayout);
            // Second column
            layout->addSpacing(2);
            layout->addWidget(mThumbnailBar);
            layout->addSpacing(2);
            // Third column
            vLayout = new QVBoxLayout;
            vLayout->addWidget(mRightToolBar);
            layout->addLayout(vLayout);

            mThumbnailBar->setFixedHeight(GwenviewConfig::fullScreenBarHeight());
            mAutoHideContainer->setFixedHeight(GwenviewConfig::fullScreenBarHeight());
        } else {
            mRightToolBar->setDirection(QBoxLayout::RightToLeft);

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
    EventWatcher::install(d->mContent, QEvent::PaletteChange, this, SLOT(slotPaletteChanged()));
    layout->addWidget(d->mContent);

    d->createOptionsAction();

    // mToolBar
    d->mToolBar = new FullScreenToolBar(d->mContent);

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
    ShadowFilter* filter = new ShadowFilter(d->mToolBar);
    filter->setShadow(ShadowFilter::RightEdge, QColor(0, 0, 0, 64));
    filter->setShadow(ShadowFilter::BottomEdge, QColor(255, 255, 255, 8));

    // mInformationLabel
    d->mInformationLabel = new QLabel;
    d->mInformationLabel->setWordWrap(true);
    d->mInformationLabel->setContentsMargins(6, 0, 6, 0);
    d->mInformationLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    d->mInformationLabel->setAutoFillBackground(true);

    filter = new ShadowFilter(d->mInformationLabel);
    filter->setShadow(ShadowFilter::LeftEdge, QColor(0, 0, 0, 64));
    filter->setShadow(ShadowFilter::TopEdge, QColor(0, 0, 0, 64));
    filter->setShadow(ShadowFilter::RightEdge, QColor(0, 0, 0, 128));
    filter->setShadow(ShadowFilter::BottomEdge, QColor(255, 255, 255, 8));

    // Thumbnail bar
    d->mThumbnailBar = new ThumbnailBarView(d->mContent);
    d->mThumbnailBar->setThumbnailScaleMode(ThumbnailView::ScaleToSquare);
    ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
    d->mThumbnailBar->setItemDelegate(delegate);
    d->mThumbnailBar->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Right bar
    d->mRightToolBar = new FullScreenToolBar(d->mContent);
    d->mRightToolBar->addAction(d->mActionCollection->action("leave_fullscreen"));
    d->mRightToolBar->addAction(d->mOptionsAction);
    d->mRightToolBar->addStretch();
    d->mRightToolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    filter = new ShadowFilter(d->mRightToolBar);
    filter->setShadow(ShadowFilter::LeftEdge, QColor(0, 0, 0, 64));
    filter->setShadow(ShadowFilter::BottomEdge, QColor(255, 255, 255, 8));

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

void FullScreenContent::slotPaletteChanged()
{
    QPalette pal = d->mContent->palette();
    pal.setColor(QPalette::Window, pal.color(QPalette::Window).dark(110));
    d->mInformationLabel->setPalette(pal);
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

void FullScreenContent::showOptionsMenu()
{
    Q_ASSERT(!d->mConfigWidget);

    d->mConfigWidget = new FullScreenConfigWidget;
    FullScreenConfigWidget* widget = d->mConfigWidget;

    // Put widget in a menu
    QMenu menu;
    QWidgetAction* action = new QWidgetAction(&menu);
    action->setDefaultWidget(widget);
    menu.addAction(action);

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

    // Show menu below its button
    QPoint pos;
    QWidget* button = d->mOptionsAction->associatedWidgets().first();
    Q_ASSERT(button);
    kWarning() << button << button->geometry();
    if (QApplication::isRightToLeft()) {
        pos = button->mapToGlobal(button->rect().bottomLeft());
    } else {
        pos = button->mapToGlobal(button->rect().bottomRight());
        pos.rx() -= menu.sizeHint().width();
    }
    kWarning() << pos;
    menu.exec(pos);
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
