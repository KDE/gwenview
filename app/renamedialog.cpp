// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2018 Aurélien Gâteau <agateau@kde.org>
Copyright 2018 Peter Mühlenpfordt <devel@ukn8.de>

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
#include "renamedialog.h"

// Qt
#include <QPushButton>
#include <QMimeDatabase>

// KDE
#include <KLocalizedString>
#include <KGuiItem>

// Local
#include <ui_renamedialog.h>

namespace Gwenview
{

struct RenameDialogPrivate : public Ui_RenameDialog
{
};

RenameDialog::RenameDialog(QWidget* parent)
: QDialog(parent)
, d(new RenameDialogPrivate)
{
    d->setupUi(this);

    QPushButton* okButton = d->mButtonBox->button(QDialogButtonBox::Ok);
    KGuiItem::assign(okButton, KGuiItem(i18nc("@action:button", "Rename"), "edit-rename"));

    connect(d->mFilename, &QLineEdit::textChanged, this, &RenameDialog::updateButtons);
}

RenameDialog::~RenameDialog()
{
    delete d;
}

void RenameDialog::setFilename(const QString& filename)
{
    d->mFilename->setText(filename);
    d->mFilenameLabel->setText(xi18n("Rename <filename>%1</filename> to:", filename));

    const QMimeDatabase db;
    const QString extension = db.suffixForFileName(filename);
    int selectionLength = filename.length();
    if (extension.length() > 0) {
        // The filename contains an extension. Assure that only the filename
        // gets selected.
        selectionLength -= extension.length() + 1;
    }
    d->mFilename->setSelection(0, selectionLength);
}

QString RenameDialog::filename() const
{
    return d->mFilename->text();
}

void RenameDialog::updateButtons() {
    const bool enableButton = !d->mFilename->text().isEmpty();
    d->mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(enableButton);
}

} // namespace
