/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef ALIGNWITHSIDEBARWIDGETACTION_H
#define ALIGNWITHSIDEBARWIDGETACTION_H

#include <QPointer>
#include <QToolBar>
#include <QWidgetAction>

/**
 * A spacer used to align actions in the toolbar with the sidebar.
 */
class AlignWithSideBarWidgetAction : public QWidgetAction
{
    Q_OBJECT

public:
    AlignWithSideBarWidgetAction(QObject *parent = nullptr);

    /**
     * @param sideBar the QWidget this spacer will base its width on to facilitate alignment.
     *                Can be nullptr.
     */
    void setSideBar(QWidget *sideBar);

protected:
    /** @see QWidgetAction::createWidget() */
    QWidget *createWidget(QWidget *parent) override;

private:
    /** The sideBar to align with. */
    QPointer<QWidget> mSideBar;
};

/**
 * The widget of AlignWithSideBarWidgetAction.
 * This class is not meant to be used from outside AlignWithSideBarWidgetAction.
 */
class AligningSpacer : public QWidget
{
    Q_OBJECT

public:
    AligningSpacer(QWidget *parent);

    /** @see AlignWithSideBarWidgetAction::setSideBar() */
    void setSideBar(QWidget *sideBar);

    /**
     * Used to trigger updateWidth() whenever mSideBar's width changes.
     */
    bool eventFilter(QObject * /* watched */, QEvent *event) override;

    /** @see QWidget::sizeHint() */
    QSize sizeHint() const override;

protected:
    /**
     * Used to trigger updateWidth() when the containing toolbar is locked/unlocked.
     */
    void moveEvent(QMoveEvent * /* moved */) override;

private:
    /**
     * Having a separator directly after an empty space looks bad unless the separator
     * is aligned with a sidebar. This method is used to remove or re-add the separator
     * so everything looks nice.
     * @param visible Wether a potential separator after this widget should be visible.
     *                This will always be changed to true if the parent toolbar is vertical.
     */
    void setFollowingSeparatorVisible(bool visible);

    /**
     * Updates the width of the spacer depending on the sizeHint() and sets the visibility
     * of a potentially following separator.
     */
    void update();

private:
    /** The SideBar to align with. */
    QPointer<QWidget> mSideBar;
    /** The parentWidget() of this widget or nullptr when the parent isn't a QToolBar. */
    QPointer<QToolBar> mToolbar;
    /** This spacer removes following separators if they would look bad/pointless. */
    bool mWasSeparatorRemoved = false;
};

#endif // ALIGNWITHSIDEBARWIDGETACTION_H
