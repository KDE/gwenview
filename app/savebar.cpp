// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "savebar.h"

// Qt
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QToolTip>
#include <QUrl>

// KF
#include <KActionCollection>
#include <KColorScheme>
#include <KIconLoader>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include "lib/document/documentfactory.h"
#include "lib/gwenviewconfig.h"
#include "lib/memoryutils.h"
#include "lib/paintutils.h"

namespace Gwenview
{
QToolButton *createToolButton()
{
    auto *button = new QToolButton;
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->hide();
    return button;
}

struct SaveBarPrivate {
    SaveBar *q;
    KActionCollection *mActionCollection;
    QWidget *mSaveBarWidget;
    QWidget *mTopRowWidget;
    QToolButton *mUndoButton;
    QToolButton *mRedoButton;
    QToolButton *mSaveCurrentUrlButton;
    QToolButton *mSaveAsButton;
    QToolButton *mSaveAllButton;
    QToolButton *mSaveAllFullScreenButton;
    QLabel *mMessageLabel;
    QLabel *mActionsLabel;
    QFrame *mTooManyChangesFrame;
    QUrl mCurrentUrl;

    void createTooManyChangesFrame()
    {
        mTooManyChangesFrame = new QFrame;

        // Icon
        auto *iconLabel = new QLabel;
        QPixmap pix = QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(KIconLoader::SizeSmall);
        iconLabel->setPixmap(pix);

        // Text label
        auto *textLabel = new QLabel;
        textLabel->setText(i18n("You have modified many images. To avoid memory problems, you should save your changes."));

        mSaveAllFullScreenButton = createToolButton();

        // Layout
        auto *layout = new QHBoxLayout(mTooManyChangesFrame);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(iconLabel);
        layout->addWidget(textLabel);
        layout->addWidget(mSaveAllFullScreenButton);
        mTooManyChangesFrame->hide();

        // CSS
        KColorScheme scheme(mSaveBarWidget->palette().currentColorGroup(), KColorScheme::Window);
        QColor warningBackgroundColor = scheme.background(KColorScheme::NegativeBackground).color();
        QColor warningBorderColor = PaintUtils::adjustedHsv(warningBackgroundColor, 0, 150, 0);
        QColor warningColor = scheme.foreground(KColorScheme::NegativeText).color();

        QString css =
            ".QFrame {"
            "	background-color: %1;"
            "	border: 1px solid %2;"
            "	border-radius: 4px;"
            "	padding: 3px;"
            "}"
            ".QFrame QLabel {"
            "	color: %3;"
            "}";
        css = css.arg(warningBackgroundColor.name(), warningBorderColor.name(), warningColor.name());
        mTooManyChangesFrame->setStyleSheet(css);
    }

    void applyNormalStyleSheet()
    {
        QColor bgColor = QToolTip::palette().base().color();
        QColor borderColor = PaintUtils::adjustedHsv(bgColor, 0, 150, 0);
        QColor fgColor = QToolTip::palette().text().color();

        QString css =
            "#saveBarWidget {"
            "	background-color: %1;"
            "	border-top: 1px solid %2;"
            "	border-bottom: 1px solid %2;"
            "	color: %3;"
            "}";

        css = css.arg(bgColor.name(), borderColor.name(), fgColor.name());
        mSaveBarWidget->setStyleSheet(css);
    }

    void applyFullScreenStyleSheet()
    {
        QString css =
            "#saveBarWidget {"
            "	background-color: #333;"
            "}";
        mSaveBarWidget->setStyleSheet(css);
    }

    void updateTooManyChangesFrame(const QList<QUrl> &list)
    {
        qreal maxPercentageOfMemoryUsage = GwenviewConfig::percentageOfMemoryUsageWarning();
        qulonglong maxMemoryUsage = MemoryUtils::getTotalMemory() * maxPercentageOfMemoryUsage;
        qulonglong memoryUsage = 0;
        for (const QUrl &url : list) {
            Document::Ptr doc = DocumentFactory::instance()->load(url);
            memoryUsage += doc->memoryUsage();
        }

        mTooManyChangesFrame->setVisible(memoryUsage > maxMemoryUsage);
    }

    void updateTopRowWidget(const QList<QUrl> &lst)
    {
        QStringList links;
        QString message;

        if (lst.contains(mCurrentUrl)) {
            message = i18n("Current image modified");

            mUndoButton->show();
            mRedoButton->show();

            if (lst.size() > 1) {
                QString previous = i18n("Previous modified image");
                QString next = i18n("Next modified image");
                if (mCurrentUrl == lst[0]) {
                    links << previous;
                } else {
                    links << QStringLiteral("<a href='previous'>%1</a>").arg(previous);
                }
                if (mCurrentUrl == lst[lst.size() - 1]) {
                    links << next;
                } else {
                    links << QStringLiteral("<a href='next'>%1</a>").arg(next);
                }
            }
        } else {
            mUndoButton->hide();
            mRedoButton->hide();

            message = i18np("One image modified", "%1 images modified", lst.size());
            if (lst.size() > 1) {
                links << QStringLiteral("<a href='first'>%1</a>").arg(i18n("Go to first modified image"));
            } else {
                links << QStringLiteral("<a href='first'>%1</a>").arg(i18n("Go to it"));
            }
        }

        mSaveCurrentUrlButton->setVisible(lst.contains(mCurrentUrl));
        mSaveAsButton->setVisible(lst.contains(mCurrentUrl));
        mSaveAllButton->setVisible(lst.size() >= 1);

        mMessageLabel->setText(message);
        mMessageLabel->setMaximumWidth(mMessageLabel->minimumSizeHint().width());
        mActionsLabel->setText(links.join(" | "));
    }

    void updateWidgetSizes()
    {
        auto *layout = static_cast<QVBoxLayout *>(mSaveBarWidget->layout());
        int topRowHeight = q->window()->isFullScreen() ? 0 : mTopRowWidget->height();
        int bottomRowHeight = mTooManyChangesFrame->isVisibleTo(mSaveBarWidget) ? mTooManyChangesFrame->sizeHint().height() : 0;

        int height = 2 * layout->margin() + topRowHeight + bottomRowHeight;
        if (topRowHeight > 0 && bottomRowHeight > 0) {
            height += layout->spacing();
        }
        mSaveBarWidget->setFixedHeight(height);
    }
};

SaveBar::SaveBar(QWidget *parent, KActionCollection *actionCollection)
    : SlideContainer(parent)
    , d(new SaveBarPrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mSaveBarWidget = new QWidget();
    d->mSaveBarWidget->setObjectName(QLatin1String("saveBarWidget"));
    d->applyNormalStyleSheet();

    d->mMessageLabel = new QLabel;
    d->mMessageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    d->mUndoButton = createToolButton();
    d->mRedoButton = createToolButton();
    d->mSaveCurrentUrlButton = createToolButton();
    d->mSaveAsButton = createToolButton();
    d->mSaveAllButton = createToolButton();

    d->mActionsLabel = new QLabel;
    d->mActionsLabel->setAlignment(Qt::AlignCenter);
    d->mActionsLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    d->createTooManyChangesFrame();

    // Setup top row
    d->mTopRowWidget = new QWidget;
    auto *rowLayout = new QHBoxLayout(d->mTopRowWidget);
    rowLayout->addWidget(d->mMessageLabel);
    rowLayout->setStretchFactor(d->mMessageLabel, 1);
    rowLayout->addWidget(d->mUndoButton);
    rowLayout->addWidget(d->mRedoButton);
    rowLayout->addWidget(d->mActionsLabel);
    rowLayout->addWidget(d->mSaveCurrentUrlButton);
    rowLayout->addWidget(d->mSaveAsButton);
    rowLayout->addWidget(d->mSaveAllButton);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    // Setup bottom row
    auto *bottomRowLayout = new QHBoxLayout;
    bottomRowLayout->addStretch();
    bottomRowLayout->addWidget(d->mTooManyChangesFrame);
    bottomRowLayout->addStretch();

    // Gather everything together
    auto *layout = new QVBoxLayout(d->mSaveBarWidget);
    layout->addWidget(d->mTopRowWidget);
    layout->addLayout(bottomRowLayout);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(3);

    setContent(d->mSaveBarWidget);

    connect(DocumentFactory::instance(), &DocumentFactory::modifiedDocumentListChanged, this, &SaveBar::updateContent);

    connect(d->mActionsLabel, &QLabel::linkActivated, this, &SaveBar::triggerAction);
}

SaveBar::~SaveBar()
{
    delete d;
}

void SaveBar::initActionDependentWidgets()
{
    d->mUndoButton->setDefaultAction(d->mActionCollection->action("edit_undo"));
    d->mRedoButton->setDefaultAction(d->mActionCollection->action("edit_redo"));
    d->mSaveCurrentUrlButton->setDefaultAction(d->mActionCollection->action("file_save"));
    d->mSaveAsButton->setDefaultAction(d->mActionCollection->action("file_save_as"));

    // FIXME: Not using an action for now
    d->mSaveAllButton->setText(i18n("Save All"));
    d->mSaveAllButton->setToolTip(i18nc("@info:tooltip", "Save all modified images"));
    d->mSaveAllButton->setIcon(QIcon::fromTheme("document-save-all"));
    connect(d->mSaveAllButton, &QToolButton::clicked, this, &SaveBar::requestSaveAll);

    d->mSaveAllFullScreenButton->setText(i18n("Save All"));
    connect(d->mSaveAllFullScreenButton, &QToolButton::clicked, this, &SaveBar::requestSaveAll);

    int height = d->mUndoButton->sizeHint().height();
    d->mTopRowWidget->setFixedHeight(height);
    d->updateWidgetSizes();
}

void SaveBar::setFullScreenMode(bool isFullScreen)
{
    d->mSaveAllFullScreenButton->setVisible(isFullScreen);
    if (isFullScreen) {
        d->applyFullScreenStyleSheet();
    } else {
        d->applyNormalStyleSheet();
    }
    updateContent();
}

void SaveBar::updateContent()
{
    QList<QUrl> lst = DocumentFactory::instance()->modifiedDocumentList();

    if (window()->isFullScreen()) {
        d->mTopRowWidget->hide();
    } else {
        d->mTopRowWidget->show();
        d->updateTopRowWidget(lst);
    }

    d->updateTooManyChangesFrame(lst);

    d->updateWidgetSizes();
    if (lst.isEmpty() || (window()->isFullScreen() && !d->mTooManyChangesFrame->isVisibleTo(d->mSaveBarWidget))) {
        slideOut();
    } else {
        slideIn();
    }
}

void SaveBar::triggerAction(const QString &action)
{
    QList<QUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
    if (action == "first") {
        emit goToUrl(lst[0]);
    } else if (action == "previous") {
        int pos = lst.indexOf(d->mCurrentUrl);
        --pos;
        Q_ASSERT(pos >= 0);
        emit goToUrl(lst[pos]);
    } else if (action == "next") {
        int pos = lst.indexOf(d->mCurrentUrl);
        ++pos;
        Q_ASSERT(pos < lst.size());
        emit goToUrl(lst[pos]);
    } else {
        qCWarning(GWENVIEW_APP_LOG) << "Unknown action: " << action;
    }
}

void SaveBar::setCurrentUrl(const QUrl &url)
{
    d->mCurrentUrl = url;
    updateContent();
}

} // namespace
