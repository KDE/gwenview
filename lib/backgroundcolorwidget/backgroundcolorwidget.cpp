/* SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "backgroundcolorwidget.h"

#include <lib/gwenviewconfig.h>
#include <lib/statusbartoolbutton.h>

#include <KColorUtils>
#include <KLocalizedString>
#include <KSqueezedTextLabel>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolButton>

using namespace Gwenview;

class Gwenview::BackgroundColorWidgetPrivate
{
    Q_DECLARE_PUBLIC(BackgroundColorWidget)
public:
    BackgroundColorWidgetPrivate(BackgroundColorWidget *q);
    BackgroundColorWidget* const q_ptr;

    void initChildWidgets();
    void initPixmaps();
    void paintPixmap(QPixmap &pixmap, const QColor &color);
    void paintAutoPixmap(QPixmap &pixmap, const QColor &lightColor, const QColor &darkColor);
    void setIcons(const QIcon &autoIcon, const QIcon &lightIcon, const QIcon &neutralIcon, const QIcon &darkIcon);

    BackgroundColorWidget::ColorMode colorMode;
    bool mirrored;
    bool usingLightTheme;

    QHBoxLayout *hBoxLayout = nullptr;
    KSqueezedTextLabel *label = nullptr;
    QButtonGroup *buttonGroup = nullptr;
    StatusBarToolButton *autoButton = nullptr;
    StatusBarToolButton *lightButton = nullptr;
    StatusBarToolButton *neutralButton = nullptr;
    StatusBarToolButton *darkButton = nullptr;

    QAction *autoMode = nullptr;
    QAction *lightMode = nullptr;
    QAction *neutralMode = nullptr;
    QAction *darkMode = nullptr;
};

BackgroundColorWidgetPrivate::BackgroundColorWidgetPrivate(BackgroundColorWidget *q)
    : q_ptr(q)
    , colorMode(GwenviewConfig::backgroundColorMode())
    , mirrored(qApp->isRightToLeft())
    , usingLightTheme(q->usingLightTheme())
    , hBoxLayout(new QHBoxLayout(q))
    , label(new KSqueezedTextLabel(q))
    , buttonGroup(new QButtonGroup(q))
    , autoButton(new StatusBarToolButton(q))
    , lightButton(new StatusBarToolButton(q))
    , neutralButton(new StatusBarToolButton(q))
    , darkButton(new StatusBarToolButton(q))
{
    initChildWidgets();
};

void BackgroundColorWidgetPrivate::initChildWidgets()
{
    Q_Q(BackgroundColorWidget);
    hBoxLayout->setContentsMargins(0, 0, 0, 0);
    hBoxLayout->setSpacing(0);

    label->setText(i18nc("@label:chooser Describes what a row of buttons control", "Background color:"));
    // 4 is the same as the hardcoded spacing between icons and labels defined in
    // QToolButton::sizeHint() when toolButtonStyle == ToolButtonTextBesideIcon.
    label->setContentsMargins(4, 0, 4, 0);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setTextElideMode(Qt::ElideRight);
    hBoxLayout->addWidget(label);

    buttonGroup->setExclusive(true);

    for (auto button : {autoButton, lightButton, neutralButton, darkButton}) {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setCheckable(true);
        buttonGroup->addButton(button);
        hBoxLayout->addWidget(button);
    }

    autoButton->setGroupPosition(mirrored ? StatusBarToolButton::GroupRight : StatusBarToolButton::GroupLeft);
    autoButton->setChecked(colorMode == BackgroundColorWidget::Auto);

    lightButton->setGroupPosition(StatusBarToolButton::GroupCenter);
    lightButton->setChecked(colorMode == BackgroundColorWidget::Light);

    neutralButton->setGroupPosition(StatusBarToolButton::GroupCenter);
    neutralButton->setChecked(colorMode == BackgroundColorWidget::Neutral);

    darkButton->setGroupPosition(mirrored ? StatusBarToolButton::GroupLeft : StatusBarToolButton::GroupRight);
    darkButton->setChecked(colorMode == BackgroundColorWidget::Dark);

    initPixmaps();
}

void BackgroundColorWidgetPrivate::initPixmaps()
{
    QPixmap lightPixmap(lightButton->iconSize() * qApp->devicePixelRatio());
    QPixmap neutralPixmap(neutralButton->iconSize() * qApp->devicePixelRatio());
    QPixmap darkPixmap(darkButton->iconSize() * qApp->devicePixelRatio());
    QPixmap autoPixmap(/*autoButton*/darkButton->iconSize() * qApp->devicePixelRatio());
    // Wipe them clean. If we don't do this, the background will have all sorts of weird artifacts.
    lightPixmap.fill(Qt::transparent);
    neutralPixmap.fill(Qt::transparent);
    darkPixmap.fill(Qt::transparent);
    autoPixmap.fill(Qt::transparent);

    const QColor &lightColor = usingLightTheme ? qApp->palette().base().color() : qApp->palette().text().color();
    const QColor &darkColor = usingLightTheme ? qApp->palette().text().color() : qApp->palette().base().color();
    QColor neutralColor = KColorUtils::mix(lightColor, darkColor, 0.5);

    paintPixmap(lightPixmap, lightColor);
    paintPixmap(neutralPixmap, neutralColor);
    paintPixmap(darkPixmap, darkColor);
    paintAutoPixmap(autoPixmap, lightColor, darkColor);

    setIcons(QIcon(autoPixmap), QIcon(lightPixmap), QIcon(neutralPixmap), QIcon(darkPixmap));
}

void BackgroundColorWidgetPrivate::paintPixmap(QPixmap &pixmap, const QColor &color)
{
    QPainter painter;
    painter.begin(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // QPainter isn't good at drawing lines that are exactly 1px thick.
    qreal penWidth = qApp->devicePixelRatio() != 1 ? qApp->devicePixelRatio() : qApp->devicePixelRatio() + 0.001;
    QColor penColor = KColorUtils::mix(color, qApp->palette().text().color(), 0.3);
    QPen pen(penColor, penWidth);
    qreal margin = pen.widthF()/2.0;
    QMarginsF penMargins(margin, margin, margin, margin);
    QRectF rect = pixmap.rect();

    painter.setBrush(color);
    painter.setPen(pen);
    painter.drawEllipse(rect.marginsRemoved(penMargins));

    painter.end();
}

void BackgroundColorWidgetPrivate::paintAutoPixmap(QPixmap &pixmap, const QColor &lightColor, const QColor &darkColor)
{
    QPainter painter;
    painter.begin(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // QPainter isn't good at drawing lines that are exactly 1px thick.
    qreal penWidth = qApp->devicePixelRatio() != 1 ? qApp->devicePixelRatio() : qApp->devicePixelRatio() + 0.001;
    QColor lightPenColor = KColorUtils::mix(lightColor, darkColor, 0.3);
    QPen lightPen(lightPenColor, penWidth);
    QColor darkPenColor = KColorUtils::mix(darkColor, lightColor, 0.3);
    QPen darkPen(darkPenColor, penWidth);

    qreal margin = lightPen.widthF()/2.0;
    QMarginsF penMargins(margin, margin, margin, margin);
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

// This function is written this way because existing Gwenview code tends to
// define actions in a central location and then pass them down to children.
// This code makes sure the buttons still have icons if the actions have not
// been set yet.
void BackgroundColorWidgetPrivate::setIcons(const QIcon &autoIcon, const QIcon &lightIcon, const QIcon &neutralIcon, const QIcon &darkIcon) {
    if (autoMode) {
        autoMode->setIcon(autoIcon);
    } else {
        autoButton->setIcon(autoIcon);
    }
    if (lightMode) {
        lightMode->setIcon(lightIcon);
    } else {
        lightButton->setIcon(lightIcon);
    }
    if (neutralMode) {
        neutralMode->setIcon(neutralIcon);
    } else {
        neutralButton->setIcon(neutralIcon);
    }
    if (darkMode) {
        darkMode->setIcon(darkIcon);
    } else {
        darkButton->setIcon(darkIcon);
    }
}

BackgroundColorWidget::BackgroundColorWidget(QWidget* parent)
    : QWidget(parent)
    , d_ptr(new BackgroundColorWidgetPrivate(this))
{
    Q_D(BackgroundColorWidget);
    connect(qApp, &QApplication::paletteChanged, this, [d]() {
        d->usingLightTheme = usingLightTheme();
        d->initPixmaps();
    });
    connect(qApp, &QApplication::layoutDirectionChanged, this, [d]() {
        d->mirrored = qApp->isRightToLeft();
        d->autoButton->setGroupPosition(d->mirrored ? StatusBarToolButton::GroupRight : StatusBarToolButton::GroupLeft);
        d->darkButton->setGroupPosition(d->mirrored ? StatusBarToolButton::GroupLeft : StatusBarToolButton::GroupRight);
    });
}

BackgroundColorWidget::~BackgroundColorWidget() noexcept
{
}

BackgroundColorWidget::ColorMode BackgroundColorWidget::colorMode() const
{
    Q_D(const BackgroundColorWidget);
    return d->colorMode;
}

void BackgroundColorWidget::setColorMode(ColorMode colorMode)
{
    Q_D(BackgroundColorWidget);
    if (d->colorMode == colorMode) {
        return;
    }

    d->colorMode = colorMode;
    d->autoButton->setChecked(colorMode == BackgroundColorWidget::Auto);
    d->lightButton->setChecked(colorMode == BackgroundColorWidget::Light);
    d->neutralButton->setChecked(colorMode == BackgroundColorWidget::Neutral);
    d->darkButton->setChecked(colorMode == BackgroundColorWidget::Dark);
    Q_EMIT colorModeChanged(colorMode);
}

void BackgroundColorWidget::setActions(QAction* autoMode, QAction* lightMode, QAction* neutralMode, QAction* darkMode)
{
    Q_D(BackgroundColorWidget);
    autoMode->setIcon(d->autoButton->icon());
    lightMode->setIcon(d->lightButton->icon());
    neutralMode->setIcon(d->neutralButton->icon());
    darkMode->setIcon(d->darkButton->icon());
    d->autoMode = autoMode;
    d->lightMode = lightMode;
    d->neutralMode = neutralMode;
    d->darkMode = darkMode;

    d->autoButton->setDefaultAction(autoMode);
    d->lightButton->setDefaultAction(lightMode);
    d->neutralButton->setDefaultAction(neutralMode);
    d->darkButton->setDefaultAction(darkMode);

    QActionGroup *actionGroup = new QActionGroup(this);
    actionGroup->addAction(autoMode);
    actionGroup->addAction(lightMode);
    actionGroup->addAction(neutralMode);
    actionGroup->addAction(darkMode);
    actionGroup->setExclusive(true);
}

bool BackgroundColorWidget::usingLightTheme()
{
    return qApp->palette().base().color().lightness() > qApp->palette().text().color().lightness();
}
