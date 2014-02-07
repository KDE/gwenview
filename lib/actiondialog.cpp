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
#include <actiondialog.moc>

// Local
#include <graphicshudbutton.h>
#include <graphicshudlabel.h>

// KDE
#include <kdebug.h>

// Qt
#include <QAction>
#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QTimer>

namespace Gwenview
{

struct ActionDialogPrivate
{
    QGraphicsLinearLayout* mLayout;
    GraphicsHudLabel* mLabel;
    QList<GraphicsHudButton*> mButtonList;

    void updateButtonWidths()
    {
        qreal minWidth = 0;
        Q_FOREACH(GraphicsHudButton* button, mButtonList) {
            minWidth = qMax(minWidth, button->preferredWidth());
        }
        Q_FOREACH(GraphicsHudButton* button, mButtonList) {
            button->setMinimumWidth(minWidth);
        }
    }
};

ActionDialog::ActionDialog(QGraphicsWidget* parent)
: GraphicsHudWidget(parent)
, d(new ActionDialogPrivate)
{
    QGraphicsWidget* content = new QGraphicsWidget();
    d->mLayout = new QGraphicsLinearLayout(Qt::Vertical, content);
    d->mLabel = new GraphicsHudLabel();
    d->mLayout->addItem(d->mLabel);
    d->mLayout->setItemSpacing(0, 24);
    init(content, GraphicsHudWidget::OptionNone);

    setContentsMargins(6, 6, 6, 6);
    setAutoDeleteOnFadeout(true);
    QTimer::singleShot(30000, this, SLOT(fadeOut()));
}

ActionDialog::~ActionDialog()
{
    delete d;
}

GraphicsHudButton* ActionDialog::addAction(QAction* action, const QString& overrideText)
{
    QString text = overrideText.isEmpty() ? action->text() : overrideText;
    GraphicsHudButton* button = addButton(text);
    connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    return button;
}

GraphicsHudButton* ActionDialog::addButton(const QString& text)
{
    GraphicsHudButton* button = new GraphicsHudButton();
    connect(button, SIGNAL(clicked()), SLOT(fadeOut()));
    button->setText(text);
    d->mLayout->addItem(button);
    d->mLayout->setAlignment(button, Qt::AlignCenter);
    d->mButtonList += button;

    return button;
}

void ActionDialog::setText(const QString& msg)
{
    d->mLabel->setText(msg);
}

void ActionDialog::showEvent(QShowEvent* event)
{
    GraphicsHudWidget::showEvent(event);
    d->updateButtonWidths();
}


} // namespace
