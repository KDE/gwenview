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
#include "mainwindow.h"

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
    QUrl mUrl;
    bool mFullScreen;
    bool mSlideShow;
    std::unique_ptr<QTemporaryDir> mMultipleUrlsDir;
    std::unique_ptr<Gwenview::MainWindow> mMainWindow;
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
                            parser.isSet(QStringLiteral("f")),
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
