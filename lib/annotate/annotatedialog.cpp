/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#include "annotatedialog.h"

// Qt
#include <QDialogButtonBox>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWindow>

// KF
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowConfig>

// KImageAnnotator
#include <kImageAnnotator/KImageAnnotator.h>

namespace Gwenview
{
namespace
{
KConfigGroup annotateDialogConfigGroup()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    return KConfigGroup(config, QStringLiteral("AnnotateDialog"));
}
} // namespace

AnnotateDialog::AnnotateDialog(QWidget *parent)
    : QDialog(parent)
    , mAnnotator(new kImageAnnotator::KImageAnnotator())
{
    mAnnotator->setParent(this);
    mAnnotator->setSaveToolSelection(true);
    auto layout = new QVBoxLayout;
    layout->addWidget(mAnnotator);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    setLayout(layout);
    restoreWindowSize();
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AnnotateDialog::~AnnotateDialog()
{
    saveWindowSize();
    delete mAnnotator;
}

void AnnotateDialog::setImage(const QImage &image)
{
    mAnnotator->loadImage(QPixmap::fromImage(image));
}

QImage AnnotateDialog::getImage() const
{
    return mAnnotator->image();
}

void AnnotateDialog::restoreWindowSize()
{
    // Default size for first launch, before any saved size exists.
    resize(parentWidget()->size() * 0.8);
    // KWindowConfig works on QWindow, which is created lazily for dialogs.
    create();
    KWindowConfig::restoreWindowSize(windowHandle(), annotateDialogConfigGroup());
    // Workaround for QTBUG-40584: restored QWindow size is not applied to the widget.
    resize(windowHandle()->size());
}

void AnnotateDialog::saveWindowSize()
{
    KConfigGroup group = annotateDialogConfigGroup();
    KWindowConfig::saveWindowSize(windowHandle(), group);
    group.sync();
}

} // namespace
