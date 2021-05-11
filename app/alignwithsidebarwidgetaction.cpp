/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "alignwithsidebarwidgetaction.h"

#include <lib/gwenviewconfig.h>

#include <KLocalizedString>

#include <QApplication>
#include <QEvent>
#include <QStyle>

#include <cmath>

AlignWithSideBarWidgetAction::AlignWithSideBarWidgetAction(QObject *parent)
    : QWidgetAction(parent)
{
    setText(i18nc("@action:inmenu a spacer that aligns toolbar buttons with the sidebar",
                  "Sidebar Alignment Spacer"));
}

void AlignWithSideBarWidgetAction::setSideBar(QWidget *sideBar)
{
    mSideBar = sideBar;
    const QList<QWidget *> widgets = createdWidgets();
    for (auto widget : widgets) {
        static_cast<AligningSpacer *>(widget)->setSideBar(sideBar);
    }
}

QWidget *AlignWithSideBarWidgetAction::createWidget(QWidget *parent)
{
    auto aligningSpacer = new AligningSpacer(parent);
    aligningSpacer->setSideBar(mSideBar);
    return aligningSpacer;
}

AligningSpacer::AligningSpacer(QWidget *parent)
    : QWidget{parent}
{
    if ((mToolbar = qobject_cast<QToolBar *>(parent))) {
        connect (mToolbar, &QToolBar::orientationChanged,
                 this, &AligningSpacer::update);
    }
}

void AligningSpacer::setSideBar(QWidget *sideBar)
{
    if (mSideBar) {
        mSideBar->removeEventFilter(this);
    }
    mSideBar = sideBar;
    if (mSideBar) {
        mSideBar->installEventFilter(this);
    }
    update();
}

bool AligningSpacer::eventFilter(QObject * /* watched */, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::Resize:
        update();
        return false;
    default:
        return false;
    }
}

QSize AligningSpacer::sizeHint() const
{
    if (!mSideBar
        || (mToolbar && mToolbar->orientation() == Qt::Vertical)
    ) {
        return QSize(0, 0);
    }

    int sideBarWidth = mSideBar->geometry().width();
    if (sideBarWidth <= 0) {
        if (!Gwenview::GwenviewConfig::sideBarSplitterSizes().isEmpty()) {
            sideBarWidth = Gwenview::GwenviewConfig::sideBarSplitterSizes().constFirst();
        } else {
            // We get to this code when gwenview.rc was deleted or when this is the first run.
            // There doesn't seem to be an easy way to get the width the sideBar is going
            // to have at this point in time so we set it to some value that leads to
            // a nice default appearance on the first run.
            sideBarWidth = 242;
        }
    }

    int newWidth;
    if (QApplication::layoutDirection() != Qt::RightToLeft) {
        newWidth = sideBarWidth - mapTo(window(), QPoint(0,0)).x();
    } else {
        newWidth = sideBarWidth - window()->width() + mapTo(window(), QPoint(width(), 0)).x();
    }

    const float separatorWidth = static_cast<float>(
            style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, nullptr, this));
    if (!mWasSeparatorRemoved) {
         // Make it so a potentially following separator looks aligned with the sidebar.
        newWidth -= std::ceil(separatorWidth * 0.3);
    } else {
        // Make it so removing the separator doesn't change the toolbutton positions.
        newWidth += std::floor(separatorWidth * 0.7);
    }

    // Make sure nothing weird can happen.
    if (newWidth < 0 || newWidth > sideBarWidth) {
        newWidth = 0;
    }
    return QSize(newWidth, 0);
}

void AligningSpacer::moveEvent(QMoveEvent * /* moved */)
{
    update();
}

void AligningSpacer::setFollowingSeparatorVisible(bool visible)
{
    if (!mToolbar) {
        return;
    }
    if (mToolbar->orientation() == Qt::Vertical) {
        visible = true;
    }

    const QList<QAction *> toolbarActions = mToolbar->actions();
    bool actionForThisSpacerFound = false; // If this is true, the next action in the iteration
                                           // is what we are interested in.
    for (auto action : toolbarActions) {
        if (actionForThisSpacerFound) {
            if (visible && mWasSeparatorRemoved) {
                mToolbar->insertSeparator(action);
                mWasSeparatorRemoved = false;
            } else if (!visible && action->isSeparator()) {
                mToolbar->removeAction(action);
                mWasSeparatorRemoved = true;
            }
            return;
        }
        if (mToolbar->widgetForAction(action) == this) {
            actionForThisSpacerFound = true;
        }
    }
}

void AligningSpacer::update()
{
    if (sizeHint().width() < 8) {
        // Because the spacer is so small the separator should be visible to serve its purpose.
        if (mWasSeparatorRemoved) {
            setFollowingSeparatorVisible(true);
        }
    } else if (mSideBar) {
        setFollowingSeparatorVisible(mSideBar->isVisible());
    }
    setFixedWidth(sizeHint().width());
}
