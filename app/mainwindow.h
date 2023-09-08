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

// KF
#include <KXmlGuiWindow>

class QModelIndex;

class QUrl;

class QMouseEvent;

namespace Gwenview
{
class ViewMainPage;
class ContextManager;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow() override;
    /**
     * Defines the url to display when the window is shown for the first time.
     */
    void setInitialUrl(const QUrl &);

    void startSlideShow();

    ViewMainPage *viewMainPage() const;

    ContextManager *contextManager() const;

    void setDistractionFreeMode(bool);

public Q_SLOTS:
    void showStartMainPage();

    /**
     * Go to url, without changing current mode
     */
    void goToUrl(const QUrl &);

Q_SIGNALS:
    void viewModeChanged();

public Q_SLOTS:
    void setCaption(const QString &) override;

    void setCaption(const QString &, bool modified) override;

protected:
    bool queryClose() override;
    QSize sizeHint() const override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void saveProperties(KConfigGroup &) override;
    void readProperties(const KConfigGroup &) override;
    bool eventFilter(QObject *, QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private Q_SLOTS:
    void setActiveViewModeAction(QAction *action);
    void openDirUrl(const QUrl &);
    void slotThumbnailViewIndexActivated(const QModelIndex &);

    void slotStartMainPageUrlSelected(const QUrl &);

    void goUp();
    void toggleSideBar(bool visible);
    void toggleOperationsSideBar(bool visible);
    void slotModifiedDocumentListChanged();
    void slotUpdateCaption(const QString &caption);
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
    void openUrl(const QUrl &url);
    void reload();

    void showDocumentInFullScreen(const QUrl &);

    /**
     * Shows Gwenview's settings dialog.
     * @param page defines which page should be shown initially.
     */
    void showConfigDialog(int page = 0);
    void loadConfig();
    void print();
    void printPreview();

    void preloadNextUrl();

    void toggleMenuBar();
    void toggleStatusBar(bool visible);

    void showFirstDocumentReached();
    void showLastDocumentReached();

    void replaceLocation();

    void onFocusChanged(QWidget *old, QWidget *now);

private:
    struct Private;
    MainWindow::Private *const d;

    void openSelectedDocuments();
    void saveConfig();
    void configureShortcuts();

    void mouseButtonNavigate(QMouseEvent *);

    void folderViewUrlChanged(const QUrl &url);
    void syncSortOrder(const QUrl &url);
};

} // namespace

#endif /* MAINWINDOW_H */
