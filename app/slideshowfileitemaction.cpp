// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2020 Méven Car <meven.car@kdemail.net>

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
#include "slideshowfileitemaction.h"

#include <QMenu>
#include <QMimeDatabase>

#include <KFileItem>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>
#include <QDir>

K_PLUGIN_CLASS_WITH_JSON(SlideShowFileItemAction, "slideshowfileitemaction.json")

SlideShowFileItemAction::SlideShowFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
}

QList<QAction *> SlideShowFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    QStringList itemsForSlideShow;
    QString text;
    const KFileItemList &fileItems = fileItemInfos.items();
    if (fileItemInfos.isDirectory()) {
        if (fileItemInfos.isLocal()) {
            QMimeDatabase db;
            // filter selected dirs containing at least one image
            for (const KFileItem &dirItem : fileItems) {
                QDir dir = QDir(dirItem.localPath());
                const QStringList fileList = dir.entryList(QDir::Filter::Files);
                const bool containsAtLeastAnImage = std::any_of(fileList.cbegin(), fileList.cend(), [&db, &dir](const QString &fileName) {
                    const auto mimeType = db.mimeTypeForFile(dir.absoluteFilePath(fileName), QMimeDatabase::MatchExtension);
                    return mimeType.name().startsWith(QStringLiteral("image"));
                });
                if (containsAtLeastAnImage) {
                    itemsForSlideShow << dirItem.localPath();
                }
            }
        }
        text = i18nc("@action:inmenu Start a slideshow Dolphin context menu", "Start a Slideshow");
    } else if (fileItemInfos.mimeGroup() == QLatin1String("image") && fileItems.length() > 1) {
        for (const KFileItem &fileItem : fileItems) {
            itemsForSlideShow << fileItem.url().toString();
        }
        text = i18nc("@action:inmenu Start a slideshow Dolphin context menu", "Start a Slideshow with selected images");
    }
    if (itemsForSlideShow.isEmpty()) {
        return {};
    }

    auto startSlideShowAction = new QAction(text, parentWidget);
    startSlideShowAction->setIcon(QIcon::fromTheme(QStringLiteral("gwenview")));

    connect(startSlideShowAction, &QAction::triggered, this, [=]() {
        // gwenview -s %u
        auto job = new KIO::CommandLauncherJob(QStringLiteral("gwenview"), QStringList(QStringLiteral("-s")) << itemsForSlideShow);
        job->setDesktopName(QStringLiteral("org.kde.gwenview"));
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));

        job->start();
    });

    return {startSlideShowAction};
}

#include "slideshowfileitemaction.moc"
