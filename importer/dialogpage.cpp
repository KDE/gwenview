// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "dialogpage.h"

// Qt
#include <QAction>
#include <QEventLoop>
#include <QList>
#include <QPushButton>
#include <QVBoxLayout>

// KF
#include <KGuiItem>
#include <KMessageBox>

// Local
#include "importdialog.h"
#include <ui_dialogpage.h>

namespace Gwenview
{
struct DialogPagePrivate : public Ui_DialogPage {
    QVBoxLayout *mLayout = nullptr;
    QList<QPushButton *> mButtons;
    QEventLoop *mEventLoop = nullptr;
    DialogPage *q = nullptr;
    QStringList failedFileList;
    QStringList failedDirList;
    QAction *fileDetails = nullptr;
    QAction *dirDetails = nullptr;

    void setupFailedListActions()
    {
        fileDetails = new QAction(i18n("Show failed files..."));
        mErrorMessageWidget->addAction(fileDetails);
        QObject::connect(fileDetails, &QAction::triggered, q, &DialogPage::slotShowFailedFileDetails);
        fileDetails->setVisible(false);

        dirDetails = new QAction(i18n("Show failed subfolders..."));
        mErrorMessageWidget->addAction(dirDetails);
        QObject::connect(dirDetails, &QAction::triggered, q, &DialogPage::slotShowFailedDirDetails);
        dirDetails->setVisible(false);
    }

    void showErrors(const QStringList &files, const QStringList &dirs)
    {
        mErrorMessageWidget->setVisible(true);
        failedFileList.clear();
        failedDirList.clear();
        QStringList message;
        if (files.count() > 0) {
            failedFileList = files;
            message << i18np("Failed to import %1 document.", "Failed to import %1 documents.", files.count());
            fileDetails->setVisible(true);
        } else
            fileDetails->setVisible(false);

        if (dirs.count() > 0) {
            failedDirList = dirs;
            message << i18np("Failed to create %1 destination subfolder.", "Failed to create %1 destination subfolders.", dirs.count());
            dirDetails->setVisible(true);
        } else
            dirDetails->setVisible(false);

        mErrorMessageWidget->setText(message.join(QStringLiteral("<br/>")));
        mErrorMessageWidget->animatedShow();
    }

    void showFailedFileDetails()
    {
        QString message = i18n("Failed to import documents:");
        KMessageBox::errorList(q, message, failedFileList);
    }

    void showFailedDirDetails()
    {
        QString message = i18n("Failed to create subfolders:");
        KMessageBox::errorList(q, message, failedDirList);
    }
};

DialogPage::DialogPage(QWidget *parent)
    : QWidget(parent)
    , d(new DialogPagePrivate)
{
    d->q = this;
    d->setupUi(this);
    d->mLayout = new QVBoxLayout(d->mButtonContainer);
    d->setupFailedListActions();
    d->mErrorMessageWidget->hide();
}

DialogPage::~DialogPage()
{
    delete d;
}

void DialogPage::removeButtons()
{
    qDeleteAll(d->mButtons);
    d->mButtons.clear();
}

void DialogPage::setText(const QString &text)
{
    d->mLabel->setText(text);
}

int DialogPage::addButton(const KGuiItem &item)
{
    int id = d->mButtons.size();
    auto *button = new QPushButton;
    KGuiItem::assign(button, item);
    button->setFixedHeight(button->sizeHint().height() * 2);

    connect(button, &QAbstractButton::clicked, this, [this, id]() {
        d->mEventLoop->exit(id);
    });
    d->mLayout->addWidget(button);
    d->mButtons << button;
    return id;
}

void DialogPage::slotShowErrors(const QStringList &files, const QStringList &dirs)
{
    d->showErrors(files, dirs);
}

void DialogPage::slotShowFailedFileDetails()
{
    d->showFailedFileDetails();
}

void DialogPage::slotShowFailedDirDetails()
{
    d->showFailedDirDetails();
}

int DialogPage::exec()
{
    QEventLoop loop;
    d->mEventLoop = &loop;
    return loop.exec();
}

} // namespace
