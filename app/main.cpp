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

// Qt
#include <QPointer>
#include <QScopedPointer>
#include <QUrl>
#include <QTemporaryDir>
#include <QApplication>
#include <QStringList>
#include <QCommandLineParser>

// KDE
#include <KAboutData>
#include <KActionCollection>
#include <KIO/CopyJob>
#include <KLocalizedString>

// Local
#include <lib/about.h>
#include <lib/imageformats/imageformats.h>
#include <lib/gwenviewconfig.h>
#include "mainwindow.h"

#ifdef HAVE_FITS
// This hack is needed to include the fitsplugin moc file in main.cpp
// Otherwise the linker complains about: undefined reference to `qt_static_plugin_FitsPlugin()'
// This symbol is defined in the moc file, but it is not a visible symbol after libgwenview is linked.
// If Q_IMPORT_PLUGIN(FitsPlugin) is moved to the library, gwenview crashes on the first call to FitsPlugin()
// when the vtable is looked up in the plugin registration.
#include <../lib/imageformats/moc_fitsplugin.cpp>
#endif

class StartHelper
{
public:
    StartHelper(const QStringList & args, bool fullscreen, bool slideshow)
    : mFullScreen(false)
    , mSlideShow(false)
    {
        if (!args.isEmpty()) {
            parseArgs(args, fullscreen, slideshow);
        }
    }

    void parseArgs(const QStringList &args, bool fullscreen, bool slideshow)
    {
        if (args.count() > 1) {
            // Create a temp dir containing links to url args
            mMultipleUrlsDir.reset(new QTemporaryDir);
            mUrl = QUrl::fromLocalFile(mMultipleUrlsDir->path());
            QList<QUrl> list;
            QStringList tmpArgs = args;
            tmpArgs.removeDuplicates();
            foreach(const QString & url, tmpArgs) {
                list << QUrl::fromUserInput(url, QDir::currentPath(), QUrl::AssumeLocalFile);
            }

            KIO::CopyJob* job = KIO::link(list, mUrl);
            job->exec();
        } else {
            QString tmpArg = args.first();
            mUrl = QUrl::fromUserInput(tmpArg, QDir::currentPath(), QUrl::AssumeLocalFile);
        }

        if (mUrl.isValid() && (fullscreen || slideshow)) {
            mFullScreen = true;
            if (slideshow) {
                mSlideShow = true;
            }
        }
    }

    void createMainWindow()
    {
        mMainWindow = new Gwenview::MainWindow();
        if (mUrl.isValid()) {
            mMainWindow->setInitialUrl(mUrl);
        } else {
            mMainWindow->showStartMainPage();
        }

        mMainWindow->show();
        if (mFullScreen) {
            mMainWindow->actionCollection()->action("fullscreen")->trigger();
        } else {
            mMainWindow->show();
        }

        if (mSlideShow) {
            mMainWindow->startSlideShow();
        }
    }

private:
    QUrl mUrl;
    bool mFullScreen;
    bool mSlideShow;
    QScopedPointer<QTemporaryDir> mMultipleUrlsDir;
    QPointer<Gwenview::MainWindow> mMainWindow;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("gwenview");
    QScopedPointer<KAboutData> aboutData(
        Gwenview::createAboutData(
            QStringLiteral("gwenview"), /* component name */
            i18n("Gwenview")                    /* display name */
        ));
    aboutData->setShortDescription(i18n("An Image Viewer"));

    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    aboutData->setOrganizationDomain(QByteArray("kde.org"));
    KAboutData::setApplicationData(*aboutData);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("gwenview"), app.windowIcon()));

    QCommandLineParser parser;
    aboutData.data()->setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("f") << QStringLiteral("fullscreen"),
                                        i18n("Start in fullscreen mode")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("s") << QStringLiteral("slideshow"),
                                        i18n("Start in slideshow mode")));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", i18n("A starting file or folders"));
    parser.process(app);
    aboutData.data()->processCommandLine(&parser);

    //KF5 TODO
    //Gwenview::ImageFormats::registerPlugins();

    // startHelper must live for the whole life of the application
    StartHelper startHelper(parser.positionalArguments(),
                            parser.isSet(QStringLiteral("f"))
                                ? true
                                : Gwenview::GwenviewConfig::fullScreenModeActive(),
                            parser.isSet(QStringLiteral("s")));
    if (app.isSessionRestored()) {
        kRestoreMainWindows<Gwenview::MainWindow>();
    } else {
        startHelper.createMainWindow();
    }

    // Workaround for QTBUG-38613
    // Another solution would be to port BalooSemanticInfoBackend::refreshAllTags
    // to be async rather than using exec().
    qApp->sendPostedEvents(0, QEvent::DeferredDelete);

    return app.exec();
}

#ifdef HAVE_FITS
    Q_IMPORT_PLUGIN(FitsPlugin)
#endif
