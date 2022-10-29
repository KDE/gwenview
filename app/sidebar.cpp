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
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QStyle>
#include <QStyleOptionTab>
#include <QToolButton>
#include <QVBoxLayout>

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
    void paintEvent(QPaintEvent *event) override
    {
        forceIcon();
        QToolButton::paintEvent(event);
    }

    QSize sizeHint() const override
    {
        const_cast<SideBarButton *>(this)->forceIcon();
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
struct SideBarGroupPrivate {
    QFrame *mContainer = nullptr;
    QLabel *mTitleLabel = nullptr;
};

SideBarGroup::SideBarGroup(const QString &title)
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

    auto layout = new QVBoxLayout(this);
    layout->addWidget(d->mTitleLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    clear();
}

SideBarGroup::~SideBarGroup()
{
    delete d;
}

void SideBarGroup::addWidget(QWidget *widget)
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
    auto containerLayout = new QVBoxLayout(d->mContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    layout()->addWidget(d->mContainer);
}

void SideBarGroup::addAction(QAction *action)
{
    int size = KIconLoader::global()->currentSize(KIconLoader::Small);
    QToolButton *button = new SideBarButton();
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
struct SideBarPagePrivate {
    QIcon mIcon;
    QString mTitle;
    QVBoxLayout *mLayout = nullptr;
};

SideBarPage::SideBarPage(const QIcon &icon, const QString &title)
    : QWidget()
    , d(new SideBarPagePrivate)
{
    d->mIcon = icon;
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

const QIcon &SideBarPage::icon() const
{
    return d->mIcon;
}

const QString &SideBarPage::title() const
{
    return d->mTitle;
}

void SideBarPage::addWidget(QWidget *widget)
{
    d->mLayout->addWidget(widget);
}

void SideBarPage::addStretch()
{
    d->mLayout->addStretch();
}

//- SideBarTabBar --------------------------------------------------------------
struct SideBarTabBarPrivate {
    SideBarTabBar::TabButtonStyle tabButtonStyle = SideBarTabBar::TabButtonTextBesideIcon;
};

SideBarTabBar::SideBarTabBar(QWidget *parent)
    : QTabBar(parent)
    , d(new SideBarTabBarPrivate)
{
    setIconSize(QSize(KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium));
}

SideBarTabBar::~SideBarTabBar() = default;

SideBarTabBar::TabButtonStyle SideBarTabBar::tabButtonStyle() const
{
    return d->tabButtonStyle;
}

QSize SideBarTabBar::sizeHint(const TabButtonStyle tabButtonStyle) const
{
    QRect tabBarRect(0, 0, 0, 0);
    for (int i = 0; i < count(); ++i) {
        const QRect tabRect(tabBarRect.topRight(), tabSizeHint(i, tabButtonStyle));
        tabBarRect = tabBarRect.united(tabRect);
    }
    return tabBarRect.size();
}

QSize SideBarTabBar::sizeHint() const
{
    return sizeHint(d->tabButtonStyle);
}

QSize SideBarTabBar::minimumSizeHint() const
{
    return sizeHint(TabButtonIconOnly);
}

QSize SideBarTabBar::tabContentSize(const int index, const TabButtonStyle tabButtonStyle, const QStyleOptionTab &opt) const
{
    if (index < 0 || index > count() - 1) {
        return QSize();
    }

    const int textWidth = opt.fontMetrics.size(Qt::TextShowMnemonic, tabText(index)).width();

    if (tabButtonStyle == TabButtonIconOnly) {
        return QSize(opt.iconSize.width(), qMax(opt.iconSize.height(), opt.fontMetrics.height()));
    }
    if (tabButtonStyle == TabButtonTextOnly) {
        return QSize(textWidth, qMax(opt.iconSize.height(), opt.fontMetrics.height()));
    }
    // 4 is the hardcoded spacing between icons and text used in Qt Widgets
    const int spacing = !opt.icon.isNull() ? 4 : 0;
    return QSize(opt.iconSize.width() + spacing + textWidth, qMax(opt.iconSize.height(), opt.fontMetrics.height()));
}

QSize SideBarTabBar::tabSizeHint(const int index, const TabButtonStyle tabButtonStyle) const
{
    if (index < 0 || index > count() - 1) {
        return QSize();
    }

    QStyleOptionTab opt;
    initStyleOption(&opt, index);
    if (tabButtonStyle == TabButtonIconOnly) {
        opt.text.clear();
    } else if (tabButtonStyle == TabButtonTextOnly) {
        opt.icon = QIcon();
    }
    const QSize contentSize = tabContentSize(index, tabButtonStyle, opt);
    const int buttonMargin = style()->pixelMetric(QStyle::PM_ButtonMargin);
    const int toolBarMargin = style()->pixelMetric(QStyle::PM_ToolBarFrameWidth) + style()->pixelMetric(QStyle::PM_ToolBarItemMargin);
    return QSize(contentSize.width() + buttonMargin * 2 + toolBarMargin * 2, contentSize.height() + buttonMargin * 2 + toolBarMargin * 2);
}

QSize SideBarTabBar::tabSizeHint(const int index) const
{
    return tabSizeHint(index, d->tabButtonStyle);
}

QSize SideBarTabBar::minimumTabSizeHint(int index) const
{
    return tabSizeHint(index, TabButtonIconOnly);
}

void SideBarTabBar::tabLayoutChange()
{
    const int width = this->width();
    if (width < sizeHint(TabButtonTextOnly).width()) {
        d->tabButtonStyle = TabButtonIconOnly;
    } else if (width < sizeHint(TabButtonTextBesideIcon).width()) {
        d->tabButtonStyle = TabButtonTextOnly;
    } else {
        d->tabButtonStyle = TabButtonTextBesideIcon;
    }
}

void SideBarTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QStylePainter painter(this);
    // Don't need to draw PE_FrameTabBarBase because it's in a QTabWidget

    const int selected = currentIndex();

    for (int i = 0; i < this->count(); ++i) {
        if (i == selected) {
            continue;
        }
        drawTab(i, painter);
    }

    // draw selected tab last so it appears on top
    if (selected >= 0) {
        drawTab(selected, painter);
    }
}

void SideBarTabBar::drawTab(int index, QStylePainter &painter) const
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QStyleOptionTabV4 opt;
#else
    QStyleOptionTab opt;
#endif
    QTabBar::initStyleOption(&opt, index);

    // draw background before doing anything else
    painter.drawControl(QStyle::CE_TabBarTabShape, opt);

    const TabButtonStyle tabButtonStyle = this->tabButtonStyle();
    if (tabButtonStyle == TabButtonTextOnly) {
        opt.icon = QIcon();
    } else if (tabButtonStyle == TabButtonIconOnly) {
        opt.text.clear();
    }
    const bool hasIcon = !opt.icon.isNull();
    const bool hasText = !opt.text.isEmpty();

    int flags = Qt::TextShowMnemonic;
    flags |= hasIcon && hasText ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter;
    if (!style()->styleHint(QStyle::SH_UnderlineShortcut, &opt, this)) {
        flags |= Qt::TextHideMnemonic;
    }

    const QSize contentSize = tabContentSize(index, tabButtonStyle, opt);
    const QRect contentRect = QStyle::alignedRect(this->layoutDirection(), Qt::AlignCenter, contentSize, opt.rect);

    if (hasIcon) {
        painter.drawItemPixmap(contentRect, flags, opt.icon.pixmap(opt.iconSize));
    }

    if (hasText) {
        // The available space to draw the text depends on wether we already drew an icon into our contentRect.
        const QSize availableSizeForText = !hasIcon ? contentSize : QSize(contentSize.width() - opt.iconSize.width() - 4, contentSize.height());
        // The '4' above is the hardcoded spacing between icons and text used in Qt Widgets.
        const QRect availableRectForText =
            !hasIcon ? contentRect : QStyle::alignedRect(this->layoutDirection(), Qt::AlignRight, availableSizeForText, contentRect);

        painter.drawItemText(availableRectForText, flags, opt.palette, opt.state & QStyle::State_Enabled, opt.text, QPalette::WindowText);
    }
}

//- SideBar --------------------------------------------------------------------
struct SideBarPrivate {
};

SideBar::SideBar(QWidget *parent)
    : QTabWidget(parent)
    , d(new SideBarPrivate)
{
    setTabBar(new SideBarTabBar(this));
    tabBar()->setDocumentMode(true);
    tabBar()->setUsesScrollButtons(false);
    tabBar()->setFocusPolicy(Qt::NoFocus);
    tabBar()->setExpanding(true);
    setTabPosition(QTabWidget::South);

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

void SideBar::addPage(SideBarPage *page)
{
    // Prevent emitting currentChanged() while populating pages
    SignalBlocker blocker(tabBar());
    addTab(page, page->icon(), page->title());
    const int thisTabIndex = this->count() - 1;
    this->setTabToolTip(thisTabIndex, page->title());
}

QString SideBar::currentPage() const
{
    return currentWidget()->objectName();
}

void SideBar::setCurrentPage(const QString &name)
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
