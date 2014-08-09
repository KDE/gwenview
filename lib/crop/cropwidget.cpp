// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "cropwidget.moc"

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QtCore/qmath.h>

// KDE
#include <KDebug>
#include <KDialog>
#include <KGlobalSettings>
#include <KLineEdit>
#include <KLocale>

// Local
#include <lib/documentview/rasterimageview.h>
#include "croptool.h"
#include "signalblocker.h"
#include "ui_cropwidget.h"
#include "cropwidget.h"

namespace Gwenview
{

// Euclidean algorithm to compute the greatest common divisor of two integers.
// Found at:
// http://en.wikipedia.org/wiki/Euclidean_algorithm
static int gcd(int a, int b)
{
    return b == 0 ? a : gcd(b, a % b);
}

static QSize screenRatio()
{
    const QRect rect = QApplication::desktop()->screenGeometry();
    const int width = rect.width();
    const int height = rect.height();
    const int divisor = gcd(width, height);
    return QSize(width / divisor, height / divisor);
}

struct CropWidgetPrivate : public Ui_CropWidget
{
    CropWidget* q;

    Document::Ptr mDocument;
    CropTool* mCropTool;
    bool mUpdatingFromCropTool;

    bool ratioIsConstrained() const
    {
        return cropRatio() > 0;
    }

    double cropRatio() const
    {
        int index = ratioComboBox->currentIndex();
        if (index != -1 && ratioComboBox->currentText() == ratioComboBox->itemText(index)) {
            // Get ratio from predefined value
            // Note: We check currentText is itemText(currentIndex) because
            // currentIndex is not reset to -1 when text is edited by hand.
            QSizeF size = ratioComboBox->itemData(index).toSizeF();
            return size.height() / size.width();
        }

        // Not a predefined value, extract ratio from the combobox text
        const QStringList lst = ratioComboBox->currentText().split(':');
        if (lst.size() != 2) {
            return 0;
        }

        bool ok;
        const double width = lst[0].toDouble(&ok);
        if (!ok) {
            return 0;
        }
        const double height = lst[1].toDouble(&ok);
        if (!ok) {
            return 0;
        }

        return height / width;
    }

    void addRatioToComboBox(const QSizeF& size, const QString& label = QString())
    {
        QString text = label.isEmpty()
            ? QString("%1:%2").arg(size.width()).arg(size.height())
            : label;
        ratioComboBox->addItem(text, QVariant(size));
    }

    void addSectionHeaderToComboBox(const QString& title)
    {
        // Insert a line
        ratioComboBox->insertSeparator(ratioComboBox->count());

        // Insert our section header
        // This header is made of a separator with a text. We reset
        // Qt::AccessibleDescriptionRole to the header text otherwise QComboBox
        // delegate will draw a separator line instead of our text.
        int index = ratioComboBox->count();
        ratioComboBox->insertSeparator(index);
        ratioComboBox->setItemText(index, title);
        ratioComboBox->setItemData(index, title, Qt::AccessibleDescriptionRole);
        ratioComboBox->setItemData(index, Qt::AlignHCenter, Qt::TextAlignmentRole);
    }

    void initRatioComboBox()
    {
        QList<QSizeF> ratioList;
        const qreal sqrt2 = qSqrt(2.);
        ratioList
                << QSizeF(7, 5)
                << QSizeF(3, 2)
                << QSizeF(4, 3)
                << QSizeF(5, 4);

        addRatioToComboBox(QSizeF(1, 1), i18n("Square"));
        addRatioToComboBox(screenRatio(), i18n("This Screen"));
        addSectionHeaderToComboBox(i18n("Landscape"));

        Q_FOREACH(const QSizeF& size, ratioList) {
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(sqrt2, 1), i18n("ISO Size (A4, A3...)"));
        addRatioToComboBox(QSizeF(11, 8.5), i18n("US Letter"));
        addSectionHeaderToComboBox(i18n("Portrait"));
        Q_FOREACH(QSizeF size, ratioList) {
            size.transpose();
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(1, sqrt2), i18n("ISO Size (A4, A3...)"));
        addRatioToComboBox(QSizeF(8.5, 11), i18n("US Letter"));

        ratioComboBox->setMaxVisibleItems(ratioComboBox->count());
        ratioComboBox->setEditText(QString());

        KLineEdit* edit = qobject_cast<KLineEdit*>(ratioComboBox->lineEdit());
        Q_ASSERT(edit);
        // Do not use i18n("%1:%2") because ':' should not be translated, it is
        // used to parse the ratio string.
        edit->setClickMessage(QString("%1:%2").arg(i18n("Width")).arg(i18n("Height")));
    }

    QRect cropRect() const
    {
        QRect rect(
            leftSpinBox->value(),
            topSpinBox->value(),
            widthSpinBox->value(),
            heightSpinBox->value()
        );
        return rect;
    }

    void initSpinBoxes()
    {
        QSize size = mDocument->size();
        leftSpinBox->setMaximum(size.width());
        widthSpinBox->setMaximum(size.width());
        topSpinBox->setMaximum(size.height());
        heightSpinBox->setMaximum(size.height());
    }

    void initDialogButtonBox()
    {
        QPushButton* cropButton = dialogButtonBox->button(QDialogButtonBox::Ok);
        cropButton->setIcon(KIcon("transform-crop-and-resize"));
        cropButton->setText(i18n("Crop"));

        QObject::connect(dialogButtonBox, SIGNAL(accepted()),
                         q, SIGNAL(cropRequested()));
        QObject::connect(dialogButtonBox, SIGNAL(rejected()),
                         q, SIGNAL(done()));
    }
};

CropWidget::CropWidget(QWidget* parent, RasterImageView* imageView, CropTool* cropTool)
: QWidget(parent)
, d(new CropWidgetPrivate)
{
    setWindowFlags(Qt::Tool);
    d->q = this;
    d->mDocument = imageView->document();
    d->mUpdatingFromCropTool = false;
    d->mCropTool = cropTool;
    d->setupUi(this);
    setFont(KGlobalSettings::smallestReadableFont());
    layout()->setMargin(KDialog::marginHint());
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(d->advancedCheckBox, SIGNAL(toggled(bool)),
            d->advancedWidget, SLOT(setVisible(bool)));
    d->advancedWidget->setVisible(false);
    d->advancedWidget->layout()->setMargin(0);

    d->initRatioComboBox();

    connect(d->mCropTool, SIGNAL(rectUpdated(QRect)),
            SLOT(setCropRect(QRect)));

    connect(d->leftSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPositionChanged()));
    connect(d->topSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPositionChanged()));
    connect(d->widthSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotWidthChanged()));
    connect(d->heightSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotHeightChanged()));

    d->initDialogButtonBox();

    connect(d->ratioComboBox, SIGNAL(editTextChanged(QString)),
            SLOT(slotRatioComboBoxEditTextChanged()));

    // Don't do this before signals are connected, otherwise the tool won't get
    // initialized
    d->initSpinBoxes();

    setCropRect(d->mCropTool->rect());
}

CropWidget::~CropWidget()
{
    delete d;
}

void CropWidget::setAdvancedSettingsEnabled(bool enable)
{
    d->advancedCheckBox->setChecked(enable);
}

bool CropWidget::advancedSettingsEnabled() const
{
    return d->advancedCheckBox->isChecked();
}

void CropWidget::setCropRect(const QRect& rect)
{
    d->mUpdatingFromCropTool = true;
    d->leftSpinBox->setValue(rect.left());
    d->topSpinBox->setValue(rect.top());
    d->widthSpinBox->setValue(rect.width());
    d->heightSpinBox->setValue(rect.height());
    d->mUpdatingFromCropTool = false;
}

void CropWidget::slotPositionChanged()
{
    const QSize size = d->mDocument->size();
    d->widthSpinBox->setMaximum(size.width() - d->leftSpinBox->value());
    d->heightSpinBox->setMaximum(size.height() - d->topSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotWidthChanged()
{
    d->leftSpinBox->setMaximum(d->mDocument->width() - d->widthSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int height = int(d->widthSpinBox->value() * d->cropRatio());
        d->heightSpinBox->setValue(height);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotHeightChanged()
{
    d->topSpinBox->setMaximum(d->mDocument->height() - d->heightSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int width = int(d->heightSpinBox->value() / d->cropRatio());
        d->widthSpinBox->setValue(width);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::applyRatioConstraint()
{
    double ratio = d->cropRatio();
    d->mCropTool->setCropRatio(ratio);

    if (!d->ratioIsConstrained()) {
        return;
    }
    QRect rect = d->cropRect();
    rect.setHeight(int(rect.width() * ratio));
    d->mCropTool->setRect(rect);
}

void CropWidget::slotRatioComboBoxEditTextChanged()
{
    applyRatioConstraint();
}

} // namespace
