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
#include "about.h"

// Local
#include <version.h>

// KDE
#include <KAboutData>
#include <KLocalizedString>

// Qt
#include <QString>

namespace Gwenview
{

KAboutData* createAboutData(const QString& appName, const QString& programName)
{
    KAboutData* data = new KAboutData(appName, programName, GWENVIEW_VERSION);
    data->setLicense(KAboutLicense::GPL);
    data->setCopyrightStatement(i18n("Copyright 2000-2014 Gwenview authors"));
    data->addAuthor(
        i18n("Aurélien Gâteau"),
        i18n("Main developer"),
        "agateau@kde.org");
    data->addAuthor(
        i18n("Benjamin Löwe"),
        i18n("Developer"),
        "benni@mytum.de");
    return data;
}

} // namespace
