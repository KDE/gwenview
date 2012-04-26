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
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QWindowsStyle>

// KDE
#include <KActionCollection>
#include <KLocale>
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

class FullScreenConfigDialog : public QFrame, public Ui_FullScreenConfigDialog
{
public:
    FullScreenConfigDialog(QWidget* parent)
        : QFrame(parent)
        {
        setWindowFlags(Qt::Popup);
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
    QAction* mOptionsAction;
    QWidget* mOptionsButton;

    bool mFullScreenMode;
    bool mViewPageVisible;

    void createOptionsAction()
    {
        mOptionsAction = new QAction(q);
        mOptionsAction->setIcon(KIcon("configure"));
        mOptionsAction->setToolTip(i18nc("@info:tooltip", "Configure Full Screen Mode"));
        QObject::connect(mOptionsAction, SIGNAL(triggered()),
                         q, SLOT(showFullScreenConfigDialog()));
    }

    void setupThemeListWidget(QListWidget* listWidget)
    {
        QStringList themeList = FullScreenTheme::themeNameList();
        listWidget->addItems(themeList);
        int row = themeList.indexOf(FullScreenTheme::currentThemeName());
        listWidget->setCurrentRow(row);

        QObject::connect(listWidget, SIGNAL(currentTextChanged(QString)),
                         q, SLOT(setCurrentFullScreenTheme(QString)));
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
            d->mOptionsButton = d->mOptionsAction->associatedWidgets().first();
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

/**
 * Helper function for showFullScreenConfigDialog
 */
static void setupCheckBox(QCheckBox* checkBox, QAction* action)
{
    checkBox->setChecked(action->isChecked());
    QObject::connect(checkBox, SIGNAL(toggled(bool)),
                     action, SLOT(trigger()));
}

void FullScreenContent::showFullScreenConfigDialog()
{
    FullScreenConfigDialog* dialog = new FullScreenConfigDialog(d->mOptionsButton);
    d->mFullScreenConfigDialog = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setObjectName(QLatin1String("configDialog"));

    // Checkboxes
    setupCheckBox(dialog->mSlideShowLoopCheckBox, d->mSlideShow->loopAction());
    setupCheckBox(dialog->mSlideShowRandomCheckBox, d->mSlideShow->randomAction());

    // Interval slider
    dialog->mSlideShowIntervalSlider->setValue(int(GwenviewConfig::interval()));
    connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
            d->mSlideShow, SLOT(setInterval(int)));

    // Interval label
    connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
            SLOT(updateSlideShowIntervalLabel()));
    QString text = formatSlideShowIntervalText(88);
    int width = dialog->mSlideShowIntervalLabel->fontMetrics().width(text);
    dialog->mSlideShowIntervalLabel->setFixedWidth(width);
    updateSlideShowIntervalLabel();

    // Config button
    connect(dialog->mConfigureDisplayedInformationButton, SIGNAL(clicked()),
            SLOT(showImageMetaInfoDialog()));

    d->setupThemeListWidget(dialog->mThemeListWidget);

    // Thumbnails check box
    dialog->mShowThumbnailsCheckBox->setChecked(GwenviewConfig::showFullScreenThumbnails());
    connect(dialog->mShowThumbnailsCheckBox, SIGNAL(toggled(bool)),
            SLOT(slotShowThumbnailsToggled(bool)));

    // Height slider
    dialog->mHeightSlider->setValue(height());
    connect(dialog->mHeightSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setFullScreenBarHeight(int)));

    // Show dialog below the button
    QRect buttonRect = d->mOptionsButton->rect();
    QPoint pos;
    if (dialog->isRightToLeft()) {
        pos = buttonRect.bottomRight();
        pos.rx() -= dialog->width() - 1;
    } else {
        pos = buttonRect.bottomLeft();
    }
    pos = d->mOptionsButton->mapToGlobal(pos);
    dialog->move(pos);
    dialog->show();
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
