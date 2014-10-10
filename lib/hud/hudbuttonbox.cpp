// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2014 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// Self
#include <hud/hudbuttonbox.h>

// Local
#include <hud/hudbutton.h>
#include <hud/hudcountdown.h>
#include <hud/hudlabel.h>
#include <graphicswidgetfloater.h>

// KDE

// Qt
#include <QAction>
#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>

namespace Gwenview
{

struct HudButtonBoxPrivate
{
    QGraphicsLinearLayout* mLayout;
    HudLabel* mLabel;
    QList<HudButton*> mButtonList;
    HudCountDown* mCountDown;

    void updateButtonWidths()
    {
        qreal minWidth = 0;
        Q_FOREACH(HudButton* button, mButtonList) {
            minWidth = qMax(minWidth, button->preferredWidth());
        }
        Q_FOREACH(HudButton* button, mButtonList) {
            button->setMinimumWidth(minWidth);
        }
    }
};

HudButtonBox::HudButtonBox(QGraphicsWidget* parent)
: HudWidget(parent)
, d(new HudButtonBoxPrivate)
{
    d->mCountDown = 0;
    QGraphicsWidget* content = new QGraphicsWidget();
    d->mLayout = new QGraphicsLinearLayout(Qt::Vertical, content);
    d->mLabel = new HudLabel();
    d->mLayout->addItem(d->mLabel);
    d->mLayout->setItemSpacing(0, 24);
    init(content, HudWidget::OptionNone);

    setContentsMargins(6, 6, 6, 6);
    setAutoDeleteOnFadeout(true);
}

HudButtonBox::~HudButtonBox()
{
    delete d;
}

void HudButtonBox::addCountDown(qreal ms)
{
    Q_ASSERT(!d->mCountDown);
    d->mCountDown = new HudCountDown(this);
    connect(d->mCountDown, &HudCountDown::timeout, this, &HudButtonBox::fadeOut);

    GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(this);
    floater->setChildWidget(d->mCountDown);
    floater->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    floater->setHorizontalMargin(6);
    floater->setVerticalMargin(6);

    d->mCountDown->start(ms);
}

HudButton* HudButtonBox::addAction(QAction* action, const QString& overrideText)
{
    QString text = overrideText.isEmpty() ? action->text() : overrideText;
    HudButton* button = addButton(text);
    connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    return button;
}

HudButton* HudButtonBox::addButton(const QString& text)
{
    HudButton* button = new HudButton();
    connect(button, &HudButton::clicked, this, &HudButtonBox::fadeOut);
    button->setText(text);
    d->mLayout->addItem(button);
    d->mLayout->setAlignment(button, Qt::AlignCenter);
    d->mButtonList += button;

    return button;
}

void HudButtonBox::setText(const QString& msg)
{
    d->mLabel->setText(msg);
}

void HudButtonBox::showEvent(QShowEvent* event)
{
    HudWidget::showEvent(event);
    d->updateButtonWidths();
}


} // namespace
