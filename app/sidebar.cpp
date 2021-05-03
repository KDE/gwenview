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
#include "sidebar.h"

// Qt
#include <QAction>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QFontDatabase>

// KF
#include <KIconLoader>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/signalblocker.h>

namespace Gwenview
{

/**
 * A button which always leave room for an icon, even if there is none, so that
 * all button texts are correctly aligned.
 */
class SideBarButton : public QToolButton
{
protected:
    void paintEvent(QPaintEvent* event) override
    {
        forceIcon();
        QToolButton::paintEvent(event);
    }

    QSize sizeHint() const override
    {
        const_cast<SideBarButton*>(this)->forceIcon();
        return QToolButton::sizeHint();
    }

private:
    void forceIcon()
    {
        if (!icon().isNull()) {
            return;
        }

        // Assign an empty icon to the button if there is no icon associated
        // with the action so that all button texts are correctly aligned.
        QSize wantedSize = iconSize();
        if (mEmptyIcon.isNull() || mEmptyIcon.actualSize(wantedSize) != wantedSize) {
            QPixmap pix(wantedSize);
            pix.fill(Qt::transparent);
            mEmptyIcon.addPixmap(pix);
        }
        setIcon(mEmptyIcon);
    }

    QIcon mEmptyIcon;
};

//- SideBarGroup ---------------------------------------------------------------
struct SideBarGroupPrivate
{
    QFrame* mContainer;
    QLabel* mTitleLabel;
};

SideBarGroup::SideBarGroup(const QString& title)
: QFrame()
, d(new SideBarGroupPrivate)
{
    d->mContainer = nullptr;
    d->mTitleLabel = new QLabel(this);
    d->mTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font(d->mTitleLabel->font());
    font.setPointSizeF(font.pointSizeF() + 1);
    d->mTitleLabel->setFont(font);
    d->mTitleLabel->setText(title);
    d->mTitleLabel->setVisible(!d->mTitleLabel->text().isEmpty());

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(d->mTitleLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    clear();
}

SideBarGroup::~SideBarGroup()
{
    delete d;
}


void SideBarGroup::addWidget(QWidget* widget)
{
    widget->setParent(d->mContainer);
    d->mContainer->layout()->addWidget(widget);
}

void SideBarGroup::clear()
{
    if (d->mContainer) {
        d->mContainer->deleteLater();
    }

    d->mContainer = new QFrame(this);
    QVBoxLayout* containerLayout = new QVBoxLayout(d->mContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    layout()->addWidget(d->mContainer);
}

void SideBarGroup::addAction(QAction* action)
{
    int size = KIconLoader::global()->currentSize(KIconLoader::Small);
    QToolButton* button = new SideBarButton();
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    button->setAutoRaise(true);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIconSize(QSize(size, size));
    if (action->menu()) {
        button->setPopupMode(QToolButton::InstantPopup);
    }
    addWidget(button);
}

//- SideBarPage ----------------------------------------------------------------
struct SideBarPagePrivate
{
    QString mTitle;
    QVBoxLayout* mLayout;
};

SideBarPage::SideBarPage(const QString& title)
: QWidget()
, d(new SideBarPagePrivate)
{
    d->mTitle = title;

    d->mLayout = new QVBoxLayout(this);
    QMargins margins = d->mLayout->contentsMargins();
    margins.setRight(qMax(0, margins.right() - style()->pixelMetric(QStyle::PM_SplitterWidth)));
    d->mLayout->setContentsMargins(margins);
}

SideBarPage::~SideBarPage()
{
    delete d;
}

const QString& SideBarPage::title() const
{
    return d->mTitle;
}

void SideBarPage::addWidget(QWidget* widget)
{
    d->mLayout->addWidget(widget);
}

void SideBarPage::addStretch()
{
    d->mLayout->addStretch();
}

//- SideBar --------------------------------------------------------------------
struct SideBarPrivate
{
};

SideBar::SideBar(QWidget* parent)
: QTabWidget(parent)
, d(new SideBarPrivate)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    tabBar()->setDocumentMode(true);
    tabBar()->setUsesScrollButtons(false);
    tabBar()->setFocusPolicy(Qt::NoFocus);
    setTabPosition(QTabWidget::South);
    setElideMode(Qt::ElideRight);

    connect(tabBar(), &QTabBar::currentChanged, [=]() {
        GwenviewConfig::setSideBarPage(currentPage());
    });
}

SideBar::~SideBar()
{
    delete d;
}

QSize SideBar::sizeHint() const
{
    return QSize(200, 200);
}

void SideBar::addPage(SideBarPage* page)
{
    // Prevent emitting currentChanged() while populating pages
    SignalBlocker blocker(tabBar());
    addTab(page, page->title());
}

QString SideBar::currentPage() const
{
    return currentWidget()->objectName();
}

void SideBar::setCurrentPage(const QString& name)
{
    for (int index = 0; index < count(); ++index) {
        if (widget(index)->objectName() == name) {
            setCurrentIndex(index);
        }
    }
}

void SideBar::loadConfig()
{
    setCurrentPage(GwenviewConfig::sideBarPage());
}

} // namespace
