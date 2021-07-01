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
#ifndef SIDEBAR_H
#define SIDEBAR_H

// Qt
#include <QFrame>
#include <QTabWidget>
#include <QTabBar>
#include <QStylePainter>

namespace Gwenview
{

class SideBar;

struct SideBarGroupPrivate;
class SideBarGroup : public QFrame
{
    Q_OBJECT
public:
    SideBarGroup(const QString& title = "");
    ~SideBarGroup() override;

    void addWidget(QWidget*);
    void addAction(QAction*);
    void clear();

private:
    SideBarGroupPrivate* const d;
};

struct SideBarPagePrivate;
class SideBarPage : public QWidget
{
    Q_OBJECT
public:
    SideBarPage(const QIcon& icon, const QString& title);
    ~SideBarPage() override;
    void addWidget(QWidget*);
    void addStretch();

    const QIcon& icon() const;
    const QString& title() const;

private:
    SideBarPagePrivate* const d;
};

struct SideBarTabBarPrivate;
class SideBarTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit SideBarTabBar(QWidget* parent);
    ~SideBarTabBar() override;

    enum TabButtonStyle {
        TabButtonIconOnly,
        TabButtonTextOnly,
        TabButtonTextBesideIcon,
    };
    Q_ENUM(TabButtonStyle)

    TabButtonStyle tabButtonStyle() const;

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

protected:
    QSize tabSizeHint(const int index) const override;
    QSize minimumTabSizeHint(int index) const override;
    // Switches the TabButtonStyle based on the width
    void tabLayoutChange() override;
    void paintEvent(QPaintEvent *event) override;
private:
    // Like sizeHint, but just for content size
    QSize tabContentSize(const int index, const TabButtonStyle tabButtonStyle, const QStyleOptionTab& opt) const;
    // Gets the tab size hint for the given TabButtonStyle
    QSize tabSizeHint(const int index, const TabButtonStyle tabButtonStyle) const;
    // Gets the size hint for the given TabButtonStyle
    QSize sizeHint(const TabButtonStyle tabButtonStyle) const;
    void drawTab(int index, QStylePainter &painter) const;
    const std::unique_ptr<SideBarTabBarPrivate> d;
};

struct SideBarPrivate;
class SideBar : public QTabWidget
{
    Q_OBJECT
public:
    explicit SideBar(QWidget* parent);
    ~SideBar() override;

    void addPage(SideBarPage*);

    QString currentPage() const;
    void setCurrentPage(const QString& name);

    void loadConfig();

    QSize sizeHint() const override;

private:
    SideBarPrivate* const d;
};

} // namespace

#endif /* SIDEBAR_H */
