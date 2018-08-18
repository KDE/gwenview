// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "documentviewcontroller.h"

// Local
#include "abstractdocumentviewadapter.h"
#include "documentview.h"
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/slidecontainer.h>
#include <lib/zoomwidget.h>

// KDE
#include <KActionCategory>
#include <KLocalizedString>

// Qt
#include <QAction>
#include <QHBoxLayout>
#include <QDebug>

namespace Gwenview
{

/**
 * A simple container which:
 * - Horizontally center the tool widget
 * - Provide a darker background
 */
class ToolContainerContent : public QWidget
{
public:
    explicit ToolContainerContent(QWidget* parent = nullptr)
    : QWidget(parent)
    , mLayout(new QHBoxLayout(this))
    {
        mLayout->setMargin(0);
        setAutoFillBackground(true);
        setBackgroundRole(QPalette::Mid);
    }

    void setToolWidget(QWidget* widget)
    {
        mLayout->addWidget(widget, 0, Qt::AlignCenter);
        setFixedHeight(widget->sizeHint().height());
    }

private:
    QHBoxLayout* mLayout;
};

struct DocumentViewControllerPrivate
{
    DocumentViewController* q;
    KActionCollection* mActionCollection;
    DocumentView* mView;
    ZoomWidget* mZoomWidget;
    SlideContainer* mToolContainer;
    ToolContainerContent* mToolContainerContent;

    QAction * mZoomToFitAction;
    QAction * mZoomToFillAction;
    QAction * mActualSizeAction;
    QAction * mZoomInAction;
    QAction * mZoomOutAction;
    QList<QAction *> mActions;

    void setupActions()
    {
        KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), mActionCollection);

        mZoomToFitAction = view->addAction(QStringLiteral("view_zoom_to_fit"));
        view->collection()->setDefaultShortcut(mZoomToFitAction, Qt::Key_F);
        mZoomToFitAction->setCheckable(true);
        mZoomToFitAction->setChecked(true);
        mZoomToFitAction->setText(i18n("Zoom to fit"));
        mZoomToFitAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-fit-best")));
        mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));

        mZoomToFillAction = view->addAction(QStringLiteral("view_zoom_to_fill"));
        view->collection()->setDefaultShortcut(mZoomToFillAction, Qt::SHIFT + Qt::Key_F);
        mZoomToFillAction->setCheckable(true);
        mZoomToFillAction->setText(i18n("Zoom to fill window by fitting to width or height"));
        mZoomToFillAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-fit-best")));
        mZoomToFillAction->setIconText(i18nc("@action:button Zoom to fill (fit width or height), shown in status bar, keep it short please", "Fill"));

        mActualSizeAction = view->addAction(KStandardAction::ActualSize);
        mActualSizeAction->setCheckable(true);
        mActualSizeAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-original")));
        mActualSizeAction->setIconText(i18nc("@action:button Zoom to original size, shown in status bar, keep it short please", "100%"));

        mZoomInAction = view->addAction(KStandardAction::ZoomIn);
        mZoomOutAction = view->addAction(KStandardAction::ZoomOut);

        mActions << mZoomToFitAction << mActualSizeAction << mZoomInAction << mZoomOutAction << mZoomToFillAction;
    }

    void connectZoomWidget()
    {
        if (!mZoomWidget || !mView) {
            return;
        }

        QObject::connect(mZoomWidget, &ZoomWidget::zoomChanged, mView, &DocumentView::setZoom);

        QObject::connect(mView, &DocumentView::minimumZoomChanged, mZoomWidget, &ZoomWidget::setMinimumZoom);
        QObject::connect(mView, &DocumentView::zoomChanged, mZoomWidget, &ZoomWidget::setZoom);

        mZoomWidget->setMinimumZoom(mView->minimumZoom());
        mZoomWidget->setZoom(mView->zoom());
    }

    void updateZoomWidgetVisibility()
    {
        if (!mZoomWidget) {
            return;
        }
        mZoomWidget->setVisible(mView && mView->canZoom());
    }

    void updateActions()
    {
        const bool enabled = mView && mView->isVisible() && mView->canZoom();
        Q_FOREACH(QAction * action, mActions) {
            action->setEnabled(enabled);
        }
    }
};

DocumentViewController::DocumentViewController(KActionCollection* actionCollection, QObject* parent)
: QObject(parent)
, d(new DocumentViewControllerPrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mView = nullptr;
    d->mZoomWidget = nullptr;
    d->mToolContainer = nullptr;
    d->mToolContainerContent = new ToolContainerContent;

    d->setupActions();
}

DocumentViewController::~DocumentViewController()
{
    delete d;
}

void DocumentViewController::setView(DocumentView* view)
{
    // Forget old view
    if (d->mView) {
        disconnect(d->mView, nullptr, this, nullptr);
        Q_FOREACH(QAction * action, d->mActions) {
            disconnect(action, nullptr, d->mView, nullptr);
        }
        disconnect(d->mZoomWidget, nullptr, d->mView, nullptr);
    }

    // Connect new view
    d->mView = view;
    if (!d->mView) {
        return;
    }
    connect(d->mView, &DocumentView::adapterChanged, this, &DocumentViewController::slotAdapterChanged);
    connect(d->mView, &DocumentView::zoomToFitChanged, this, &DocumentViewController::updateZoomToFitActionFromView);
    connect(d->mView, &DocumentView::zoomToFillChanged, this, &DocumentViewController::updateZoomToFillActionFromView);
    connect(d->mView, &DocumentView::currentToolChanged, this, &DocumentViewController::updateTool);

    connect(d->mZoomToFitAction, &QAction::triggered, d->mView, &DocumentView::toggleZoomToFit);
    connect(d->mZoomToFillAction, &QAction::triggered, d->mView, &DocumentView::toggleZoomToFill);
    connect(d->mActualSizeAction, SIGNAL(triggered()),
            d->mView, SLOT(zoomActualSize()));
    connect(d->mZoomInAction, SIGNAL(triggered()),
            d->mView, SLOT(zoomIn()));
    connect(d->mZoomOutAction, SIGNAL(triggered()),
            d->mView, SLOT(zoomOut()));

    d->updateActions();
    updateZoomToFitActionFromView();
    updateZoomToFillActionFromView();
    updateTool();

    // Sync zoom widget
    d->connectZoomWidget();
    d->updateZoomWidgetVisibility();
}

DocumentView* DocumentViewController::view() const
{
    return d->mView;
}

void DocumentViewController::setZoomWidget(ZoomWidget* widget)
{
    d->mZoomWidget = widget;

    d->mZoomWidget->setActions(
        d->mZoomToFitAction,
        d->mActualSizeAction,
        d->mZoomInAction,
        d->mZoomOutAction,
        d->mZoomToFillAction
    );

    d->mZoomWidget->setMaximumZoom(qreal(DocumentView::MaximumZoom));

    d->connectZoomWidget();
    d->updateZoomWidgetVisibility();
}

ZoomWidget* DocumentViewController::zoomWidget() const
{
    return d->mZoomWidget;
}

void DocumentViewController::slotAdapterChanged()
{
    d->updateActions();
    d->updateZoomWidgetVisibility();
}

void DocumentViewController::updateZoomToFitActionFromView()
{
    d->mZoomToFitAction->setChecked(d->mView->zoomToFit());
}

void DocumentViewController::updateZoomToFillActionFromView()
{
    d->mZoomToFillAction->setChecked(d->mView->zoomToFill());
}

void DocumentViewController::updateTool()
{
    if (!d->mToolContainer) {
        return;
    }
    AbstractRasterImageViewTool* tool = d->mView->currentTool();
    if (tool && tool->widget()) {
        // Use a QueuedConnection to ensure the size of the view has been
        // updated by the time the slot is called.
        connect(d->mToolContainer, &SlideContainer::slidedIn, tool, &AbstractRasterImageViewTool::onWidgetSlidedIn, Qt::QueuedConnection);
        d->mToolContainerContent->setToolWidget(tool->widget());
        d->mToolContainer->slideIn();
    } else {
        d->mToolContainer->slideOut();
    }
}

void DocumentViewController::reset()
{
    setView(nullptr);
    d->updateActions();
}

void DocumentViewController::setToolContainer(SlideContainer* container)
{
    d->mToolContainer = container;
    container->setContent(d->mToolContainerContent);
}

} // namespace
