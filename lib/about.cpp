// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// Self
#include <about.h>

// Local
#include <version.h>

// KDE
#include <KAboutData>
#include <KLocalizedString>

// Qt

namespace Gwenview
{

KAboutData* createAboutData(const QByteArray& appName, const QByteArray& catalogName, const KLocalizedString& programName)
{
    KAboutData* data = new KAboutData(appName, catalogName, programName, GWENVIEW_VERSION);
    data->setLicense(KAboutData::License_GPL);
    data->setCopyrightStatement(ki18n("Copyright 2000-2013 Gwenview authors"));
    data->addAuthor(
        ki18n("Aurélien Gâteau"),
        ki18n("Main developer"),
        "agateau@kde.org");
    data->addAuthor(
        ki18n("Benjamin Löwe"),
        ki18n("Developer"),
        "benni@mytum.de");
    return data;
}

} // namespace
