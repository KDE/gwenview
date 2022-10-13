// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
    Gwenview: an image viewer
    SPDX-FileCopyrightText: 2011 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Self
#include "documentviewcontroller.h"

// Local
#include "documentview.h"
#include "gwenview_lib_debug.h"
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/gwenviewconfig.h>
#include <lib/slidecontainer.h>
#include <lib/zoomwidget.h>

// KF
#include <KActionCategory>
#include <KColorUtils>
#include <KLocalizedString>

// Qt
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QHBoxLayout>
#include <QPainter>

namespace Gwenview
{
struct DocumentViewControllerPrivate {
    DocumentViewController *q = nullptr;
    KActionCollection *mActionCollection = nullptr;
    DocumentView *mView = nullptr;
    ZoomWidget *mZoomWidget = nullptr;
    SlideContainer *mToolContainer = nullptr;

    QAction *mZoomToFitAction = nullptr;
    QAction *mZoomToFillAction = nullptr;
    QAction *mActualSizeAction = nullptr;
    QAction *mZoomInAction = nullptr;
    QAction *mZoomOutAction = nullptr;
    QAction *mToggleBirdEyeViewAction = nullptr;
    QAction *mBackgroundColorModeAuto = nullptr;
    QAction *mBackgroundColorModeLight = nullptr;
    QAction *mBackgroundColorModeNeutral = nullptr;
    QAction *mBackgroundColorModeDark = nullptr;
    QList<QAction *> mActions;

    void setupActions()
    {
        auto view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), mActionCollection);

        mZoomToFitAction = view->addAction(QStringLiteral("view_zoom_to_fit"));
        view->collection()->setDefaultShortcut(mZoomToFitAction, Qt::Key_F);
        mZoomToFitAction->setCheckable(true);
        mZoomToFitAction->setChecked(true);
        mZoomToFitAction->setText(i18n("Zoom to Fit"));
        mZoomToFitAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-fit-best")));
        mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));
        mZoomToFitAction->setToolTip(i18nc("@info:tooltip", "Fit image into the viewing area"));
        // clang-format off
        // i18n: "a previous zoom value" is worded in such an unclear way because it can either be the zoom value for the image viewed previously or the
        // zoom value that was used the last time this same image was viewed. Being more clear about this isn't really necessary here so I kept it short
        // but a more elaborate translation would also be fine.
        // The text "in the settings" is supposed to sound like clicking it opens the settings.
        mZoomToFitAction->setWhatsThis(xi18nc("@info:whatsthis, %1 the action's text", "<para>This fits the image into the available viewing area:<list>"
            "<item>Images that are bigger than the viewing area are displayed at a smaller size so they fit.</item>"
            "<item>Images that are smaller than the viewing area are displayed at their normal size. If smaller images should instead use all of "
            "the available viewing area, turn on <emphasis>Enlarge smaller images</emphasis> <link url='%2'>in the settings</link>.</item></list></para>"
            "<para>If \"%1\" is already enabled, pressing it again will switch it off and the image will be displayed at its normal size instead.</para>"
            "<para>\"%1\" is the default zoom mode for images. This can be changed so images are displayed at a previous zoom value instead "
            "<link url='%2'>in the settings</link>.</para>", mZoomToFitAction->iconText(), QStringLiteral("gwenview:/config/imageview")));
            // Keep the previous link address in sync with MainWindow::Private::SettingsOpenerHelper::eventFilter().
        // clang-format on

        mZoomToFillAction = view->addAction(QStringLiteral("view_zoom_to_fill"));
        view->collection()->setDefaultShortcut(mZoomToFillAction, Qt::SHIFT | Qt::Key_F);
        mZoomToFillAction->setCheckable(true);
        mZoomToFillAction->setText(i18n("Zoom to fill window by fitting to width or height"));
        mZoomToFillAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-fit-best")));
        mZoomToFillAction->setIconText(i18nc("@action:button Zoom to fill (fit width or height), shown in status bar, keep it short please", "Fill"));

        mActualSizeAction = view->addAction(KStandardAction::ActualSize);
        mActualSizeAction->setCheckable(true);
        mActualSizeAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom-original")));
        mActualSizeAction->setIconText(QLocale().toString(100).append(QLocale().percent()));

        mZoomInAction = view->addAction(KStandardAction::ZoomIn);
        mZoomOutAction = view->addAction(KStandardAction::ZoomOut);

        mToggleBirdEyeViewAction = view->addAction(QStringLiteral("view_toggle_birdeyeview"));
        mToggleBirdEyeViewAction->setCheckable(true);
        mToggleBirdEyeViewAction->setChecked(GwenviewConfig::birdEyeViewEnabled());
        mToggleBirdEyeViewAction->setText(i18n("Show Bird's Eye View When Zoomed In"));
        mToggleBirdEyeViewAction->setIcon(QIcon::fromTheme(QStringLiteral("zoom")));
        mToggleBirdEyeViewAction->setEnabled(mView != nullptr);

        mBackgroundColorModeAuto = view->addAction(QStringLiteral("view_background_colormode_auto"));
        mBackgroundColorModeAuto->setCheckable(true);
        mBackgroundColorModeAuto->setChecked(GwenviewConfig::backgroundColorMode() == DocumentView::BackgroundColorMode::Auto);
        mBackgroundColorModeAuto->setText(i18nc("@action", "Follow color scheme"));
        mBackgroundColorModeAuto->setEnabled(mView != nullptr);

        mBackgroundColorModeLight = view->addAction(QStringLiteral("view_background_colormode_light"));
        mBackgroundColorModeLight->setCheckable(true);
        mBackgroundColorModeLight->setChecked(GwenviewConfig::backgroundColorMode() == DocumentView::BackgroundColorMode::Light);
        mBackgroundColorModeLight->setText(i18nc("@action", "Light Mode"));
        mBackgroundColorModeLight->setEnabled(mView != nullptr);

        mBackgroundColorModeNeutral = view->addAction(QStringLiteral("view_background_colormode_neutral"));
        mBackgroundColorModeNeutral->setCheckable(true);
        mBackgroundColorModeNeutral->setChecked(GwenviewConfig::backgroundColorMode() == DocumentView::BackgroundColorMode::Neutral);
        mBackgroundColorModeNeutral->setText(i18nc("@action", "Neutral Mode"));
        mBackgroundColorModeNeutral->setEnabled(mView != nullptr);

        mBackgroundColorModeDark = view->addAction(QStringLiteral("view_background_colormode_dark"));
        mBackgroundColorModeDark->setCheckable(true);
        mBackgroundColorModeDark->setChecked(GwenviewConfig::backgroundColorMode() == DocumentView::BackgroundColorMode::Dark);
        mBackgroundColorModeDark->setText(i18nc("@action", "Dark Mode"));
        mBackgroundColorModeDark->setEnabled(mView != nullptr);

        setBackgroundColorModeIcons(mBackgroundColorModeAuto, mBackgroundColorModeLight, mBackgroundColorModeNeutral, mBackgroundColorModeDark);

        auto actionGroup = new QActionGroup(q);
        actionGroup->addAction(mBackgroundColorModeAuto);
        actionGroup->addAction(mBackgroundColorModeLight);
        actionGroup->addAction(mBackgroundColorModeNeutral);
        actionGroup->addAction(mBackgroundColorModeDark);
        actionGroup->setExclusive(true);

        mActions << mZoomToFitAction << mActualSizeAction << mZoomInAction << mZoomOutAction << mZoomToFillAction << mToggleBirdEyeViewAction
                 << mBackgroundColorModeAuto << mBackgroundColorModeLight << mBackgroundColorModeNeutral << mBackgroundColorModeDark;
    }

    void setBackgroundColorModeIcons(QAction *autoAction, QAction *lightAction, QAction *neutralAction, QAction *darkAction) const
    {
        const bool usingLightTheme = qApp->palette().base().color().lightness() > qApp->palette().text().color().lightness();
        const int pixMapWidth(16 * qApp->devicePixelRatio()); // Default icon size in menus is 16 but on toolbars is 22. The icon will only show up in QMenus
                                                              // unless a user adds the action to their toolbar so we go for 16.
        QPixmap lightPixmap(pixMapWidth, pixMapWidth);
        QPixmap neutralPixmap(pixMapWidth, pixMapWidth);
        QPixmap darkPixmap(pixMapWidth, pixMapWidth);
        QPixmap autoPixmap(pixMapWidth, pixMapWidth);
        // Wipe them clean. If we don't do this, the background will have all sorts of weird artifacts.
        lightPixmap.fill(Qt::transparent);
        neutralPixmap.fill(Qt::transparent);
        darkPixmap.fill(Qt::transparent);
        autoPixmap.fill(Qt::transparent);

        const QColor &lightColor = usingLightTheme ? qApp->palette().base().color() : qApp->palette().text().color();
        const QColor &darkColor = usingLightTheme ? qApp->palette().text().color() : qApp->palette().base().color();
        const QColor neutralColor = KColorUtils::mix(lightColor, darkColor, 0.5);

        paintPixmap(lightPixmap, lightColor);
        paintPixmap(neutralPixmap, neutralColor);
        paintPixmap(darkPixmap, darkColor);
        paintAutoPixmap(autoPixmap, lightColor, darkColor);

        autoAction->setIcon(autoPixmap);
        lightAction->setIcon(lightPixmap);
        neutralAction->setIcon(neutralPixmap);
        darkAction->setIcon(darkPixmap);
    }

    void paintPixmap(QPixmap &pixmap, const QColor &color) const
    {
        QPainter painter;
        painter.begin(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // QPainter isn't good at drawing lines that are exactly 1px thick.
        const qreal penWidth = qApp->devicePixelRatio() != 1 ? qApp->devicePixelRatio() : qApp->devicePixelRatio() + 0.001;
        const QColor penColor = KColorUtils::mix(color, qApp->palette().text().color(), 0.3);
        const QPen pen(penColor, penWidth);
        const qreal margin = pen.widthF() / 2.0;
        const QMarginsF penMargins(margin, margin, margin, margin);
        const QRectF rect = pixmap.rect();

        painter.setBrush(color);
        painter.setPen(pen);
        painter.drawEllipse(rect.marginsRemoved(penMargins));

        painter.end();
    }

    void paintAutoPixmap(QPixmap &pixmap, const QColor &lightColor, const QColor &darkColor) const
    {
        QPainter painter;
        painter.begin(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // QPainter isn't good at drawing lines that are exactly 1px thick.
        const qreal penWidth = qApp->devicePixelRatio() != 1 ? qApp->devicePixelRatio() : qApp->devicePixelRatio() + 0.001;
        const QColor lightPenColor = KColorUtils::mix(lightColor, darkColor, 0.3);
        const QPen lightPen(lightPenColor, penWidth);
        const QColor darkPenColor = KColorUtils::mix(darkColor, lightColor, 0.3);
        const QPen darkPen(darkPenColor, penWidth);

        const qreal margin = lightPen.widthF() / 2.0;
        const QMarginsF penMargins(margin, margin, margin, margin);
        QRectF rect = pixmap.rect();
        rect = rect.marginsRemoved(penMargins);
        int lightStartAngle = 45 * 16;
        int lightSpanAngle = 180 * 16;
        int darkStartAngle = -135 * 16;
        int darkSpanAngle = 180 * 16;

        painter.setBrush(lightColor);
        painter.setPen(lightPen);
        painter.drawChord(rect, lightStartAngle, lightSpanAngle);
        painter.setBrush(darkColor);
        painter.setPen(darkPen);
        painter.drawChord(rect, darkStartAngle, darkSpanAngle);

        painter.end();
    }

    void connectZoomWidget()
    {
        if (!mZoomWidget || !mView) {
            return;
        }

        // from mZoomWidget to mView
        QObject::connect(mZoomWidget, &ZoomWidget::zoomChanged, mView, &DocumentView::setZoom);

        // from mView to mZoomWidget
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
        for (QAction *action : qAsConst(mActions)) {
            action->setEnabled(enabled);
        }
    }
};

DocumentViewController::DocumentViewController(KActionCollection *actionCollection, QObject *parent)
    : QObject(parent)
    , d(new DocumentViewControllerPrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mView = nullptr;
    d->mZoomWidget = nullptr;
    d->mToolContainer = nullptr;

    d->setupActions();
}

DocumentViewController::~DocumentViewController()
{
    delete d;
}

void DocumentViewController::setView(DocumentView *view)
{
    // Forget old view
    if (d->mView) {
        disconnect(d->mView, nullptr, this, nullptr);
        for (QAction *action : qAsConst(d->mActions)) {
            disconnect(action, nullptr, d->mView, nullptr);
        }
        disconnect(d->mBackgroundColorModeAuto, &QAction::triggered, this, nullptr);
        disconnect(d->mBackgroundColorModeLight, &QAction::triggered, this, nullptr);
        disconnect(d->mBackgroundColorModeNeutral, &QAction::triggered, this, nullptr);
        disconnect(d->mBackgroundColorModeDark, &QAction::triggered, this, nullptr);

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
    connect(d->mActualSizeAction, &QAction::triggered, d->mView, &DocumentView::zoomActualSize);
    connect(d->mZoomInAction, SIGNAL(triggered()), d->mView, SLOT(zoomIn()));
    connect(d->mZoomOutAction, SIGNAL(triggered()), d->mView, SLOT(zoomOut()));

    connect(d->mToggleBirdEyeViewAction, &QAction::triggered, d->mView, &DocumentView::toggleBirdEyeView);

    connect(d->mBackgroundColorModeAuto, &QAction::triggered, this, [this]() {
        d->mView->setBackgroundColorMode(DocumentView::BackgroundColorMode::Auto);
        qApp->paletteChanged(qApp->palette());
    });
    connect(d->mBackgroundColorModeLight, &QAction::triggered, this, [this]() {
        d->mView->setBackgroundColorMode(DocumentView::BackgroundColorMode::Light);
        qApp->paletteChanged(qApp->palette());
    });
    connect(d->mBackgroundColorModeNeutral, &QAction::triggered, this, [this]() {
        d->mView->setBackgroundColorMode(DocumentView::BackgroundColorMode::Neutral);
        qApp->paletteChanged(qApp->palette());
    });
    connect(d->mBackgroundColorModeDark, &QAction::triggered, this, [this]() {
        d->mView->setBackgroundColorMode(DocumentView::BackgroundColorMode::Dark);
        qApp->paletteChanged(qApp->palette());
    });

    d->updateActions();
    updateZoomToFitActionFromView();
    updateZoomToFillActionFromView();
    updateTool();

    // Sync zoom widget
    d->connectZoomWidget();
    d->updateZoomWidgetVisibility();
}

DocumentView *DocumentViewController::view() const
{
    return d->mView;
}

void DocumentViewController::setZoomWidget(ZoomWidget *widget)
{
    d->mZoomWidget = widget;

    d->mZoomWidget->setActions(d->mZoomToFitAction, d->mActualSizeAction, d->mZoomInAction, d->mZoomOutAction, d->mZoomToFillAction);

    d->mZoomWidget->setMaximumZoom(qreal(DocumentView::MaximumZoom));

    d->connectZoomWidget();
    d->updateZoomWidgetVisibility();
}

ZoomWidget *DocumentViewController::zoomWidget() const
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
    AbstractRasterImageViewTool *tool = d->mView->currentTool();
    if (tool && tool->widget()) {
        // Use a QueuedConnection to ensure the size of the view has been
        // updated by the time the slot is called.
        connect(d->mToolContainer, &SlideContainer::slidedIn, tool, &AbstractRasterImageViewTool::onWidgetSlidedIn, Qt::QueuedConnection);
        d->mToolContainer->setContent(tool->widget());
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

void DocumentViewController::setToolContainer(SlideContainer *container)
{
    d->mToolContainer = container;
}

} // namespace
