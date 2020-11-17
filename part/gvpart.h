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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef GVPART_H
#define GVPART_H

// Qt

// KF
#include <KParts/ReadOnlyPart>

// Local
#include <lib/document/document.h>

class KAboutData;

namespace Gwenview
{

class DocumentView;

class GVPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    GVPart(QWidget* parentWidget, QObject* parent, const QVariantList&);

    static KAboutData* createAboutData();

protected:
    bool openUrl(const QUrl &url) override;

private Q_SLOTS:
    void showContextMenu();
    void showProperties();
    void saveAs();
    void showJobError(KJob*);

private:
    DocumentView* mDocumentView;
};

} // namespace

#endif /* GVPART_H */
