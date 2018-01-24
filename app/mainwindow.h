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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QAction>

// KDE
#include <KXmlGuiWindow>

class QModelIndex;

class QUrl;

namespace Gwenview
{

class ViewMainPage;
class ContextManager;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    /**
     * Defines the url to display when the window is shown for the first time.
     */
    void setInitialUrl(const QUrl&);

    void startSlideShow();

    ViewMainPage* viewMainPage() const;

    ContextManager* contextManager() const;

    void setDistractionFreeMode(bool);

public Q_SLOTS:
    void showStartMainPage();

    /**
     * Go to url, without changing current mode
     */
    void goToUrl(const QUrl&);

Q_SIGNALS:
    void viewModeChanged();

public Q_SLOTS:
    virtual void setCaption(const QString&) Q_DECL_OVERRIDE;

    virtual void setCaption(const QString&, bool modified) Q_DECL_OVERRIDE;

protected:
    virtual bool queryClose() Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
    virtual void saveProperties(KConfigGroup&) Q_DECL_OVERRIDE;
    virtual void readProperties(const KConfigGroup&) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void setActiveViewModeAction(QAction* action);
    void openDirUrl(const QUrl&);
    void slotThumbnailViewIndexActivated(const QModelIndex&);

    void slotStartMainPageUrlSelected(const QUrl&);

    void goUp();
    void toggleSideBar(bool visible);
    void updateToggleSideBarAction();
    void slotModifiedDocumentListChanged();
    void slotUpdateCaption(const QString& caption);
    void slotPartCompleted();
    void slotDirModelNewItems();
    void slotDirListerCompleted();

    void slotSelectionChanged();
    void slotCurrentDirUrlChanged(const QUrl &url);

    void goToPrevious();
    void goToNext();
    void goToFirst();
    void goToLast();
    void updatePreviousNextActions();

    void leaveFullScreen();
    void toggleFullScreen(bool);
    void toggleSlideShow();
    void updateSlideShowAction();

    void saveCurrent();
    void saveCurrentAs();
    void openFile();
    void openUrl(const QUrl& url);
    void reload();

    void showDocumentInFullScreen(const QUrl&);

    void showConfigDialog();
    void loadConfig();
    void print();

    void preloadNextUrl();

    void toggleMenuBar();
    void toggleStatusBar();

    void showFirstDocumentReached();
    void showLastDocumentReached();

private:
    struct Private;
    MainWindow::Private* const d;

    void openSelectedDocuments();
    void saveConfig();

    void folderViewUrlChanged(const QUrl &url);
};

} // namespace

#endif /* MAINWINDOW_H */
