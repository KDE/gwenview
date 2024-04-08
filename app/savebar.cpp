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
#include <QToolTip>
#include <QUrl>

// KF
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageWidget>

// Local
#include "gwenview_app_debug.h"
#include "lib/document/documentfactory.h"
#include "lib/gwenviewconfig.h"
#include "lib/memoryutils.h"

namespace Gwenview
{
struct SaveBarPrivate {
    SaveBar *q = nullptr;
    KActionCollection *mActionCollection = nullptr;
    KMessageWidget *mSaveMessage = nullptr;
    KMessageWidget *mTooManyChangesMessage = nullptr;
    QAction *mSaveAllAction = nullptr;
    QUrl mCurrentUrl;

    void createTooManyChangesMessage()
    {
        mTooManyChangesMessage = new KMessageWidget;
        mTooManyChangesMessage->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
        mTooManyChangesMessage->setText(i18n("You have modified many images. To avoid memory problems, you should save your changes."));
        mTooManyChangesMessage->setCloseButtonVisible(false);
        mTooManyChangesMessage->setPosition(KMessageWidget::Position::Header);
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

        mTooManyChangesMessage->setVisible(memoryUsage > maxMemoryUsage);
    }

    void updateTopRowWidget(const QList<QUrl> &lst)
    {
        mSaveMessage->clearActions();

        QStringList links;
        QString message;

        if (lst.contains(mCurrentUrl)) {
            message = i18n("Current image modified");

            mSaveMessage->addAction(mActionCollection->action(QStringLiteral("edit_undo")));
            mSaveMessage->addAction(mActionCollection->action(QStringLiteral("edit_redo")));

            mSaveMessage->addAction(mActionCollection->action(QStringLiteral("file_save")));
            mSaveMessage->addAction(mActionCollection->action(QStringLiteral("file_save_as")));

            if (lst.size() > 1) {
                mSaveMessage->addAction(mSaveAllAction);
            }

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
            message = i18np("One image modified", "%1 images modified", lst.size());
            if (lst.size() > 1) {
                links << QStringLiteral("<a href='first'>%1</a>").arg(i18n("Go to first modified image"));
            } else {
                links << QStringLiteral("<a href='first'>%1</a>").arg(i18n("Go to it"));
            }
        }

        mSaveMessage->setText(message + QStringLiteral(" ") + links.join(QStringLiteral(" | ")));
    }
};

SaveBar::SaveBar(QWidget *parent, KActionCollection *actionCollection)
    : SlideContainer(parent)
    , d(new SaveBarPrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mSaveMessage = new KMessageWidget();
    d->mSaveMessage->setPosition(KMessageWidget::Position::Header);
    d->mSaveMessage->setCloseButtonVisible(false);
    d->mSaveMessage->setObjectName(QStringLiteral("saveBarWidget"));

    // Used for navigating between files when more than one is modified
    connect(d->mSaveMessage, &KMessageWidget::linkActivated, this, &SaveBar::triggerAction);

    d->mSaveAllAction = new QAction();
    d->mSaveAllAction->setText(i18n("Save All"));
    d->mSaveAllAction->setToolTip(i18nc("@info:tooltip", "Save all modified images"));
    d->mSaveAllAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save-all")));
    connect(d->mSaveAllAction, &QAction::triggered, this, &SaveBar::requestSaveAll);

    d->createTooManyChangesMessage();

    // The "save bar" and the "too many changes" headers could appear on top of each other, so let's do it
    auto headerLayout = new QVBoxLayout;
    headerLayout->setSpacing(0);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(d->mSaveMessage);
    headerLayout->addWidget(d->mTooManyChangesMessage);

    // Can't set the layout only, we use a widget to slide in and out
    auto dummyContent = new QWidget;
    dummyContent->setContentsMargins(0, 0, 0, 0);
    dummyContent->setLayout(headerLayout);

    setContent(dummyContent);

    connect(DocumentFactory::instance(), &DocumentFactory::modifiedDocumentListChanged, this, &SaveBar::updateContent);
}

SaveBar::~SaveBar()
{
    delete d;
}

void SaveBar::setFullScreenMode(bool isFullScreen)
{
    d->mTooManyChangesMessage->clearActions();
    if (isFullScreen) {
        d->mTooManyChangesMessage->addAction(d->mSaveAllAction);
    }
    updateContent();
}

void SaveBar::updateContent()
{
    QList<QUrl> lst = DocumentFactory::instance()->modifiedDocumentList();

    if (window()->isFullScreen()) {
        d->mSaveMessage->hide();
    } else {
        d->mSaveMessage->show();
        d->updateTopRowWidget(lst);
    }

    d->updateTooManyChangesFrame(lst);

    if (lst.isEmpty() || (window()->isFullScreen() && !d->mTooManyChangesMessage->isVisibleTo(d->mSaveMessage))) {
        slideOut();
    } else {
        slideIn();
    }
}

void SaveBar::triggerAction(const QString &action)
{
    QList<QUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
    if (action == QLatin1String("first")) {
        Q_EMIT goToUrl(lst[0]);
    } else if (action == QLatin1String("previous")) {
        int pos = lst.indexOf(d->mCurrentUrl);
        --pos;
        Q_ASSERT(pos >= 0);
        Q_EMIT goToUrl(lst[pos]);
    } else if (action == QLatin1String("next")) {
        int pos = lst.indexOf(d->mCurrentUrl);
        ++pos;
        Q_ASSERT(pos < lst.size());
        Q_EMIT goToUrl(lst[pos]);
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

#include "moc_savebar.cpp"
