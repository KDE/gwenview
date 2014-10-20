// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#include "resizeimagedialog.h"

// Qt
#include <QDialogButtonBox>
#include <QPushButton>

// KDE
#include <KLocalizedString>
#include <KGuiItem>

// Local
#include <ui_resizeimagewidget.h>

namespace Gwenview
{

struct ResizeImageDialogPrivate : public Ui_ResizeImageWidget
{
    bool mUpdateFromRatio;
    QSize mOriginalSize;
};

ResizeImageDialog::ResizeImageDialog(QWidget* parent)
: QDialog(parent)
, d(new ResizeImageDialogPrivate)
{
    d->mUpdateFromRatio = false;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QWidget* content = new QWidget(this);
    d->setupUi(content);
    mainLayout->addWidget(content);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ResizeImageDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ResizeImageDialog::reject);
    mainLayout->addWidget(buttonBox);

    content->layout()->setMargin(0);
    KGuiItem::assign(okButton, KGuiItem(i18n("Resize"), "transform-scale"));
    setWindowTitle(content->windowTitle());
    d->mWidthSpinBox->setFocus();

    connect(d->mWidthSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotWidthChanged);
    connect(d->mHeightSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotHeightChanged);
    connect(d->mKeepAspectCheckBox, &QCheckBox::toggled, this, &ResizeImageDialog::slotKeepAspectChanged);
}

ResizeImageDialog::~ResizeImageDialog()
{
    delete d;
}

void ResizeImageDialog::setOriginalSize(const QSize& size)
{
    d->mOriginalSize = size;
    d->mOriginalWidthLabel->setText(QString::number(size.width()));
    d->mOriginalHeightLabel->setText(QString::number(size.height()));
    d->mWidthSpinBox->setValue(size.width());
    d->mHeightSpinBox->setValue(size.height());
}

QSize ResizeImageDialog::size() const
{
    return QSize(
               d->mWidthSpinBox->value(),
               d->mHeightSpinBox->value()
           );
}

void ResizeImageDialog::slotWidthChanged(int width)
{
    if (!d->mKeepAspectCheckBox->isChecked()) {
        return;
    }
    if (d->mUpdateFromRatio) {
        return;
    }
    d->mUpdateFromRatio = true;
    d->mHeightSpinBox->setValue(d->mOriginalSize.height() * width / d->mOriginalSize.width());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotHeightChanged(int height)
{
    if (!d->mKeepAspectCheckBox->isChecked()) {
        return;
    }
    if (d->mUpdateFromRatio) {
        return;
    }
    d->mUpdateFromRatio = true;
    d->mWidthSpinBox->setValue(d->mOriginalSize.width() * height / d->mOriginalSize.height());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotKeepAspectChanged(bool value)
{
    if (value) {
        slotWidthChanged(d->mWidthSpinBox->value());
    }
}

} // namespace
