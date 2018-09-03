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

namespace Gwenview
{

static const int DEFAULT_LAYOUT_MARGIN = 6;

class SideBar;

struct SideBarGroupPrivate;
class SideBarGroup : public QFrame
{
    Q_OBJECT
public:
    SideBarGroup(const QString& title, bool defaultContainerMarginEnabled = true);
    ~SideBarGroup() override;

    void addWidget(QWidget*);
    void addAction(QAction*);
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    SideBarGroupPrivate* const d;
};

struct SideBarPagePrivate;
class SideBarPage : public QWidget
{
    Q_OBJECT
public:
    SideBarPage(const QString& title);
    ~SideBarPage() override;
    void addWidget(QWidget*);
    void addStretch();

    const QString& title() const;

private:
    SideBarPagePrivate* const d;
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
