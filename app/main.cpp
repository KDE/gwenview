/*
Gwenview: an image viewer
Copyright 2007-2012 Aurélien Gâteau <agateau@kde.org>

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
#include <config-gwenview.h>
// STL
#include <memory>

// Qt

// KDE
#include <KAboutData>
#include <KActionCollection>
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KIO/CopyJob>
#include <KLocale>
#include <KMessageBox>
#include <KTempDir>
#include <KUrl>

// Local
#include <lib/imageformats/imageformats.h>
#include <lib/version.h>
#include "mainwindow.h"

class StartHelper
{
public:
    StartHelper()
    : mFullScreen(false)
    , mSlideShow(false)
    {
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
        if (args->count() > 0) {
            parseArgs(args);
        }
        args->clear();
    }

    void parseArgs(KCmdLineArgs* args)
    {
        if (args->count() > 1) {
            // Createa a temp dir containing links to url args
            mMultipleUrlsDir.reset(new KTempDir);
            mUrl = KUrl::fromPath(mMultipleUrlsDir->name());
            KUrl::List list;
            for (int pos = 0; pos < args->count(); ++pos) {
                list << args->url(pos);
            }
            KIO::CopyJob* job = KIO::link(list, mUrl);
            job->exec();
        } else {
            mUrl = args->url(0);
        }

        if (mUrl.isValid() && (args->isSet("f") || args->isSet("s"))) {
            mFullScreen = true;
            if (args->isSet("s")) {
                mSlideShow = true;
            }
        }
    }

    void createMainWindow()
    {
        Gwenview::MainWindow* window = new Gwenview::MainWindow();
        if (mUrl.isValid()) {
            window->setInitialUrl(mUrl);
        } else {
            window->showStartMainPage();
        }

        window->show();
        if (mFullScreen) {
            window->actionCollection()->action("fullscreen")->trigger();
        } else {
            window->show();
        }

        if (mSlideShow) {
            window->startSlideShow();
        }
    }

private:
    KUrl mUrl;
    bool mFullScreen;
    bool mSlideShow;
    std::auto_ptr<KTempDir> mMultipleUrlsDir;
    std::auto_ptr<Gwenview::MainWindow> mMainWindow;
};

int main(int argc, char *argv[])
{
    KAboutData aboutData(
        "gwenview",        /* appname */
        0,                 /* catalogName */
        ki18n("Gwenview"), /* programName */
        GWENVIEW_VERSION); /* version */
    aboutData.setShortDescription(ki18n("An Image Viewer"));
    aboutData.setLicense(KAboutData::License_GPL);
    aboutData.setCopyrightStatement(ki18n("Copyright 2000-2013 Aurélien Gâteau"));
    aboutData.addAuthor(
        ki18n("Aurélien Gâteau"),
        ki18n("Main developer"),
        "agateau@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("f", ki18n("Start in fullscreen mode"));
    options.add("s", ki18n("Start in slideshow mode"));
    options.add("+[file or folder]", ki18n("A starting file or folder"));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;
    Gwenview::ImageFormats::registerPlugins();

    // startHelper must live for the whole life of the application
    StartHelper startHelper;
    if (app.isSessionRestored()) {
        kRestoreMainWindows<Gwenview::MainWindow>();
    } else {
        startHelper.createMainWindow();
    }
    return app.exec();
}
