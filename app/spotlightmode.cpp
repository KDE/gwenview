#include "spotlightmode.h"

// Qt
#include <QHBoxLayout>
#include <QPushButton>

// Local
#include "gwenview_app_debug.h"
#include <lib/gwenviewconfig.h>

namespace Gwenview
{

struct SpotlightModePrivate {
    SpotlightMode *q = nullptr;
    QToolButton *mButtonQuit = nullptr;
    KActionCollection *mActionCollection = nullptr;
};

SpotlightMode::SpotlightMode(QWidget *parent, KActionCollection *actionCollection)
    : QHBoxLayout(parent)
    , d(new SpotlightModePrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mButtonQuit = new QToolButton();
    d->mButtonQuit->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    d->mButtonQuit->setAutoRaise(true);
    d->mButtonQuit->setVisible(false);
    addWidget(d->mButtonQuit, 0, Qt::AlignTop | Qt::AlignRight);

    connect(d->mButtonQuit, &QPushButton::released, this, &SpotlightMode::emitButtonQuitClicked);
}

SpotlightMode::~SpotlightMode()
{
    delete d;
}

void SpotlightMode::setVisibleSpotlightModeQuitButton(bool visible)
{
    d->mButtonQuit->setVisible(visible);
}

void SpotlightMode::emitButtonQuitClicked()
{
    GwenviewConfig::setSpotlightMode(false);
    d->mActionCollection->action(QStringLiteral("view_toggle_spotlightmode"))->trigger();
}

} // namespace
