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

// KDE

// Qt
#include <QAction>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace Gwenview
{

struct ActionDialogPrivate
{
    QVBoxLayout* mLayout;
    QLabel* mLabel;
};

ActionDialog::ActionDialog(QWidget* parent)
: KDialog(parent)
, d(new ActionDialogPrivate)
{
    setButtons(KDialog::None);
    QWidget* content = new QWidget();
    d->mLayout = new QVBoxLayout(content);
    d->mLabel = new QLabel();
    d->mLabel->setWordWrap(true);
    d->mLayout->addWidget(d->mLabel);
    setMainWidget(content);
}

ActionDialog::~ActionDialog()
{
    delete d;
}

QPushButton* ActionDialog::addAction(QAction* action, const QString& overrideText)
{
    QPushButton* button = addButton(overrideText.isEmpty() ? action->text() : overrideText);
    button->setIcon(action->icon());
    connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    return button;
}

QPushButton* ActionDialog::addButton(const QString& text)
{
    QPushButton* button = new QPushButton(text);
    button->setMinimumHeight(button->sizeHint().height() * 2);
    d->mLayout->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(accept()));
    return button;
}

void ActionDialog::setText(const QString& msg) {
    d->mLabel->setText(msg);
}

} // namespace
