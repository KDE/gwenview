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
#ifndef IMPORTER_H
#define IMPORTER_H

// Qt
#include <QObject>
#include <QUrl>

// KF

// Local

class KJob;

namespace Gwenview
{

struct ImporterPrivate;
class Importer : public QObject
{
    Q_OBJECT
public:
    explicit Importer(QWidget* authWindow);
    ~Importer() override;

    /**
     * Defines the auto-rename format applied to imported documents
     * Set to QString() to reset
     */
    void setAutoRenameFormat(const QString&);

    void start(const QList<QUrl>& list, const QUrl& destUrl);

    QList<QUrl> importedUrlList() const;

    /**
     * Documents which have been skipped during import
     */
    QList<QUrl> skippedUrlList() const;

    /**
     * Documents which have failed during import
     */
    QList<QUrl> failedUrlList() const;

    /**
     * Subfolders which failed to create during import
     */
    QList<QUrl> failedSubFolderList() const;

    /**
     * How many documents have been renamed during import
     */
    int renamedCount() const;

Q_SIGNALS:
    void importFinished();

    void progressChanged(int);

    void maximumChanged(int);

    /**
     * An error has occurred and caused the whole process to stop without
     * importing anything
     */
    void error(const QString& message);

private Q_SLOTS:
    void slotCopyDone(KJob*);
    void slotPercent(KJob*, unsigned long);
    void emitProgressChanged();

private:
    friend struct ImporterPrivate;
    ImporterPrivate* const d;
    void advance();
    void finalizeImport();
};

} // namespace

#endif /* IMPORTER_H */
