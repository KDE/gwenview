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
#include "progresspage.h"

// Local
#include "importer.h"
#include <ui_progresspage.h>

namespace Gwenview
{
struct ProgressPagePrivate : public Ui_ProgressPage {
    ProgressPage *q;
    Importer *mImporter;
};

ProgressPage::ProgressPage(Importer *importer)
    : d(new ProgressPagePrivate)
{
    d->q = this;
    d->mImporter = importer;
    d->setupUi(this);

    connect(d->mImporter, &Importer::progressChanged, d->mProgressBar, &QProgressBar::setValue);
    connect(d->mImporter, &Importer::maximumChanged, d->mProgressBar, &QProgressBar::setMaximum);
}

ProgressPage::~ProgressPage()
{
    delete d;
}

} // namespace
