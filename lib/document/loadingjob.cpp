// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#include "loadingjob.h"

// Qt

// KDE
#include <KDebug>
#include <KLocale>
#include <KUrl>

// Local

namespace Gwenview
{

void LoadingJob::doStart()
{
    Document::LoadingState state = document()->loadingState();
    if (state == Document::Loaded || state == Document::LoadingFailed) {
        setError(NoError);
        emitResult();
    } else {
        connect(document().data(), SIGNAL(loaded(KUrl)), SLOT(slotLoaded()));
        connect(document().data(), SIGNAL(loadingFailed(KUrl)), SLOT(slotLoadingFailed()));
    }
}

void LoadingJob::slotLoaded()
{
    setError(NoError);
    emitResult();
}

void LoadingJob::slotLoadingFailed()
{
    setError(UserDefinedError + 1);
    setErrorText(i18n("Could not load document %1", document()->url().pathOrUrl()));
    emitResult();
}

} // namespace
