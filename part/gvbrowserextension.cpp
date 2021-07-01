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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "gvbrowserextension.h"

// Qt

// KF
#include <KIconLoader>
#include <KParts/ReadOnlyPart>

// Local
#include "../lib/document/documentfactory.h"
#include "../lib/print/printhelper.h"

namespace Gwenview
{
struct GVBrowserExtensionPrivate {
    KParts::ReadOnlyPart *mPart;
};

GVBrowserExtension::GVBrowserExtension(KParts::ReadOnlyPart *part)
    : KParts::BrowserExtension(part)
    , d(new GVBrowserExtensionPrivate)
{
    d->mPart = part;
    emit enableAction("print", true);
    const QString iconPath = KIconLoader::global()->iconPath(QStringLiteral("image-x-generic"), KIconLoader::SizeSmall);
    emit setIconUrl(QUrl::fromLocalFile(iconPath));
}

GVBrowserExtension::~GVBrowserExtension()
{
    delete d;
}

void GVBrowserExtension::print()
{
    Document::Ptr doc = DocumentFactory::instance()->load(d->mPart->url());
    PrintHelper printHelper(d->mPart->widget());
    printHelper.print(doc);
}

} // namespace
