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

// Qt
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QtMath>
#include <QLineEdit>
#include <QFontDatabase>

// KF
#include <KLocalizedString>

// Local
#include "gwenview_lib_debug.h"
#include <lib/documentview/rasterimageview.h>
#include "croptool.h"
#include "cropwidget.h"
#include "flowlayout.h"

namespace Gwenview
{

// Euclidean algorithm to compute the greatest common divisor of two integers.
// Found at:
// https://en.wikipedia.org/wiki/Euclidean_algorithm
static int gcd(int a, int b)
{
    return b == 0 ? a : gcd(b, a % b);
}

static QSize ratio(const QSize &size)
{
    const int divisor = gcd(size.width(), size.height());
    return size / divisor;
}

struct CropWidgetPrivate : public QWidget
{
    CropWidget* q;
    QList<QWidget*> mAdvancedWidgets;
    QWidget* mPreserveAspectRatioWidget;
    QCheckBox* advancedCheckBox;
    QComboBox* ratioComboBox;
    QSpinBox* widthSpinBox;
    QSpinBox* heightSpinBox;
    QSpinBox* leftSpinBox;
    QSpinBox* topSpinBox;
    QCheckBox* preserveAspectRatioCheckBox;
    QDialogButtonBox* dialogButtonBox;

    Document::Ptr mDocument;
    CropTool* mCropTool;
    bool mUpdatingFromCropTool;
    int mCurrentImageComboBoxIndex;
    int mCropRatioComboBoxCurrentIndex;

    bool ratioIsConstrained() const
    {
        return cropRatio() > 0;
    }

    QSizeF chosenRatio() const
    {
        // A size of 0 represents no ratio, i.e. the combobox is empty
        if (ratioComboBox->currentText().isEmpty()) {
            return QSizeF(0, 0);
        }

        // A preset ratio is selected
        const int index = ratioComboBox->currentIndex();
        if (index != -1 && ratioComboBox->currentText() == ratioComboBox->itemText(index)) {
            return ratioComboBox->currentData().toSizeF();
        }

        // A custom ratio has been entered, extract ratio from the text
        // If invalid, return zero size instead
        const QStringList lst = ratioComboBox->currentText().split(QLatin1Char(':'));
        if (lst.size() != 2) {
            return QSizeF(0, 0);
        }
        bool ok;
        const double width = lst[0].toDouble(&ok);
        if (!ok) {
            return QSizeF(0, 0);
        }
        const double height = lst[1].toDouble(&ok);
        if (!ok) {
            return QSizeF(0, 0);
        }

        // Valid custom value
        return QSizeF(width, height);
    }

    void setChosenRatio(QSizeF size) const
    {
        // Size matches preset ratio, let's set the combobox to that
        const int index = ratioComboBox->findData(size);
        if (index >= 0) {
            ratioComboBox->setCurrentIndex(index);
            return;
        }

        // Deselect whatever was selected if anything
        ratioComboBox->setCurrentIndex(-1);

        // If size is 0 (represents blank combobox, i.e., unrestricted)
        if (size.isEmpty()) {
            ratioComboBox->clearEditText();
            return;
        }

        // Size must be custom ratio, convert to text and add to combobox
        QString ratioString = QStringLiteral("%1:%2").arg(size.width()).arg(size.height());
        ratioComboBox->setCurrentText(ratioString);
    }

    double cropRatio() const
    {
        if (q->advancedSettingsEnabled()) {
            QSizeF size = chosenRatio();
            if (size.isEmpty()) {
                return 0;
            }
            return size.height() / size.width();
        }

        if (q->preserveAspectRatio()) {
            QSizeF size = ratio(mDocument->size());
            return size.height() / size.width();
        }

        return 0;
    }

    void addRatioToComboBox(const QSizeF& size, const QString& label = QString())
    {
        QString text = label.isEmpty()
            ? QStringLiteral("%1:%2").arg(size.width()).arg(size.height())
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
                << QSizeF(16, 9)
                << QSizeF(7, 5)
                << QSizeF(3, 2)
                << QSizeF(4, 3)
                << QSizeF(5, 4);

        addRatioToComboBox(ratio(mDocument->size()), i18n("Current Image"));
        mCurrentImageComboBoxIndex = ratioComboBox->count() - 1; // We need to refer to this ratio later

        addRatioToComboBox(QSizeF(1, 1), i18n("Square"));
        addRatioToComboBox(ratio(QApplication::desktop()->screenGeometry().size()), i18n("This Screen"));
        // The previous string should be changed to
        // addRatioToComboBox(ratio(QGuiApplication::screenAt(QCursor::pos())->geometry().size()), i18n("This Screen"));
        // after switching to Qt > 5.9
        addSectionHeaderToComboBox(i18n("Landscape"));

        for (const QSizeF& size : qAsConst(ratioList)) {
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(sqrt2, 1), i18n("ISO (A4, A3...)"));
        addRatioToComboBox(QSizeF(11, 8.5), i18n("US Letter"));
        addSectionHeaderToComboBox(i18n("Portrait"));
        for (QSizeF size : qAsConst(ratioList)) {
            size.transpose();
            addRatioToComboBox(size);
        }
        addRatioToComboBox(QSizeF(1, sqrt2), i18n("ISO (A4, A3...)"));
        addRatioToComboBox(QSizeF(8.5, 11), i18n("US Letter"));

        ratioComboBox->setMaxVisibleItems(ratioComboBox->count());
        ratioComboBox->clearEditText();

        auto* edit = qobject_cast<QLineEdit*>(ratioComboBox->lineEdit());
        Q_ASSERT(edit);
        // Do not use i18n("%1:%2") because ':' should not be translated, it is
        // used to parse the ratio string.
        edit->setPlaceholderText(QStringLiteral("%1:%2").arg(i18n("Width"), i18n("Height")));

        // Enable clear button
        edit->setClearButtonEnabled(true);
        // Must manually adjust minimum width because the auto size adjustment doesn't take the
        // clear button into account
        const int width = ratioComboBox->minimumSizeHint().width();
        ratioComboBox->setMinimumWidth(width + 24);

        mCropRatioComboBoxCurrentIndex = -1;
        ratioComboBox->setCurrentIndex(mCropRatioComboBoxCurrentIndex);
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

        // When users change the crop rectangle, QSpinBox::setMaximum will be called
        // again, which then adapts the sizeHint due to a different maximum number
        // of digits, leading to horizontal movement in the layout. This can be
        // avoided by setting the minimum width so it fits the largest value possible.
        leftSpinBox->setMinimumWidth(leftSpinBox->sizeHint().width());
        widthSpinBox->setMinimumWidth(widthSpinBox->sizeHint().width());
        topSpinBox->setMinimumWidth(topSpinBox->sizeHint().width());
        heightSpinBox->setMinimumWidth(heightSpinBox->sizeHint().width());
    }

    void initDialogButtonBox()
    {
        QPushButton* cropButton = dialogButtonBox->button(QDialogButtonBox::Ok);
        cropButton->setIcon(QIcon::fromTheme(QStringLiteral("transform-crop-and-resize")));
        cropButton->setText(i18n("Crop"));

        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, q, &CropWidget::cropRequested);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, q, &CropWidget::done);
        QPushButton *resetButton = dialogButtonBox->button(QDialogButtonBox::Reset);
        connect(resetButton, &QPushButton::clicked, q, &CropWidget::reset);
    }

    QWidget* boxWidget(QWidget* parent = nullptr)
    {
        auto* widget = new QWidget(parent);
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        return widget;
    }

    void setupUi(QWidget* cropWidget) {
        cropWidget->setObjectName(QStringLiteral("CropWidget"));

        auto* flowLayout = new FlowLayout(cropWidget, 6, 0);
        flowLayout->setObjectName(QStringLiteral("CropWidgetFlowLayout"));
        flowLayout->setAlignment(Qt::AlignCenter);
        flowLayout->setVerticalSpacing(6);

        // (1) Checkbox
        QWidget* box = boxWidget(cropWidget);
        advancedCheckBox = new QCheckBox(i18nc("@option:check", "Advanced settings"), box);
        advancedCheckBox->setFocusPolicy(Qt::NoFocus);
        box->layout()->addWidget(advancedCheckBox);
        flowLayout->addWidget(box);
        flowLayout->addSpacing(14);

        // (2) Ratio combobox (Advanced settings)
        box = boxWidget(cropWidget);
        mAdvancedWidgets << box;
        auto* label = new QLabel(i18nc("@label:listbox", "Aspect ratio:"), box);
        label->setContentsMargins(4, 4, 4, 4);
        box->layout()->addWidget(label);
        ratioComboBox = new QComboBox(box);
        ratioComboBox->setEditable(true);
        ratioComboBox->setInsertPolicy(QComboBox::NoInsert);
        box->layout()->addWidget(ratioComboBox);
        flowLayout->addWidget(box);
        flowLayout->addSpacing(8);

        // (3) Size spinboxes (Advanced settings)
        box = boxWidget(cropWidget);
        mAdvancedWidgets << box;
        label = new QLabel(i18nc("@label:spinbox", "Size:"), box);
        label->setContentsMargins(4, 4, 4, 4);
        box->layout()->addWidget(label);

        auto* innerLayout = new QHBoxLayout();
        innerLayout->setSpacing(3);

        widthSpinBox = new QSpinBox(box);
        widthSpinBox->setAlignment(Qt::AlignCenter);
        innerLayout->addWidget(widthSpinBox);

        heightSpinBox = new QSpinBox(box);
        heightSpinBox->setAlignment(Qt::AlignCenter);
        innerLayout->addWidget(heightSpinBox);

        box->layout()->addItem(innerLayout);
        flowLayout->addWidget(box);
        flowLayout->addSpacing(8);

        // (4) Position spinboxes (Advanced settings)
        box = boxWidget(cropWidget);
        mAdvancedWidgets << box;
        label = new QLabel(i18nc("@label:spinbox", "Position:"), box);
        label->setContentsMargins(4, 4, 4, 4);
        box->layout()->addWidget(label);

        innerLayout = new QHBoxLayout();
        innerLayout->setSpacing(3);

        leftSpinBox = new QSpinBox(box);
        leftSpinBox->setAlignment(Qt::AlignCenter);
        innerLayout->addWidget(leftSpinBox);

        topSpinBox = new QSpinBox(box);
        topSpinBox->setAlignment(Qt::AlignCenter);
        innerLayout->addWidget(topSpinBox);

        box->layout()->addItem(innerLayout);
        flowLayout->addWidget(box);
        flowLayout->addSpacing(18);

        // (5) Preserve ratio checkbox
        mPreserveAspectRatioWidget = boxWidget(cropWidget);
        preserveAspectRatioCheckBox = new QCheckBox(i18nc("@option:check", "Preserve aspect ratio"), mPreserveAspectRatioWidget);
        mPreserveAspectRatioWidget->layout()->addWidget(preserveAspectRatioCheckBox);
        flowLayout->addWidget(mPreserveAspectRatioWidget);
        flowLayout->addSpacing(18);

        // (6) Dialog buttons
        box = boxWidget(cropWidget);
        dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Reset | QDialogButtonBox::Ok, box);
        box->layout()->addWidget(dialogButtonBox);
        flowLayout->addWidget(box);
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
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));

    connect(d->advancedCheckBox, &QCheckBox::toggled, this, &CropWidget::slotAdvancedCheckBoxToggled);
    for (auto w : d->mAdvancedWidgets) {
        w->setVisible(false);
    }

    connect(d->preserveAspectRatioCheckBox, &QCheckBox::toggled, this, &CropWidget::applyRatioConstraint);

    d->initRatioComboBox();

    connect(d->mCropTool, &CropTool::rectUpdated, this, &CropWidget::setCropRect);

    connect(d->leftSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CropWidget::slotPositionChanged);
    connect(d->topSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CropWidget::slotPositionChanged);
    connect(d->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CropWidget::slotWidthChanged);
    connect(d->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CropWidget::slotHeightChanged);

    d->initDialogButtonBox();

    // We need to listen for both signals because the combobox is multi-function:
    // Text Changed: required so that manual ratio entry is detected (index doesn't change)
    // Index Changed: required so that choosing an item with the same text is detected (e.g. going from US Letter portrait
    // to US Letter landscape)
    connect(d->ratioComboBox, &QComboBox::editTextChanged, this, &CropWidget::slotRatioComboBoxChanged);
    connect(d->ratioComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CropWidget::slotRatioComboBoxChanged);

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

void CropWidget::setPreserveAspectRatio(bool preserve)
{
    d->preserveAspectRatioCheckBox->setChecked(preserve);
}

bool CropWidget::preserveAspectRatio() const
{
    return d->preserveAspectRatioCheckBox->isChecked();
}

void CropWidget::setCropRatio(QSizeF size)
{
    d->setChosenRatio(size);
}

QSizeF CropWidget::cropRatio() const
{
    return d->chosenRatio();
}

void CropWidget::reset()
{
    d->ratioComboBox->clearEditText();
    d->ratioComboBox->setCurrentIndex(-1);

    QSize size = d->mDocument->size();
    d->leftSpinBox->setValue(0);
    d->widthSpinBox->setValue(size.width());
    d->topSpinBox->setValue(0);
    d->heightSpinBox->setValue(size.height());
    emit rectReset();
}

void CropWidget::setCropRatioIndex(int index)
{
    d->ratioComboBox->setCurrentIndex(index);
}

int CropWidget::cropRatioIndex() const
{
    return d->mCropRatioComboBoxCurrentIndex;
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

void CropWidget::slotAdvancedCheckBoxToggled(bool checked)
{
    for (auto w : qAsConst(d->mAdvancedWidgets)) {
        w->setVisible(checked);
    }
    d->mPreserveAspectRatioWidget->setVisible(!checked);
    applyRatioConstraint();
}

void CropWidget::slotRatioComboBoxChanged()
{
    const QString text = d->ratioComboBox->currentText();

    // If text cleared, clear the current item as well
    if (text.isEmpty()) {
        d->ratioComboBox->setCurrentIndex(-1);
    }

    // We want to keep track of the selected ratio, including when the user has entered a custom ratio
    // or cleared the text. We can't simply use currentIndex() because this stays >= 0 when the user manually
    // enters text. We also can't set the current index to -1 when there is no match like above because that
    // interferes when manually entering text.
    // Furthermore, since there can be duplicate text items, we can't rely on findText() as it will stop on
    // the first match it finds. Therefore we must check if there's a match, and if so, get the index directly.
    if (d->ratioComboBox->findText(text) >= 0) {
        d->mCropRatioComboBoxCurrentIndex = d->ratioComboBox->currentIndex();
    } else {
        d->mCropRatioComboBoxCurrentIndex = -1;
    }

    applyRatioConstraint();
}

void CropWidget::updateCropRatio()
{
    // First we need to re-calculate the "Current Image" ratio in case the user rotated the image
    d->ratioComboBox->setItemData(d->mCurrentImageComboBoxIndex, QVariant(ratio(d->mDocument->size())));

    // Always re-apply the constraint, even though we only need to when the user has "Current Image"
    // selected or the "Preserve aspect ratio" checked, since there's no harm
    applyRatioConstraint();
    // If the ratio is unrestricted, calling applyRatioConstraint doesn't update the rect, so we call
    // this manually to make sure the rect is adjusted to fit within the image
    d->mCropTool->setRect(d->mCropTool->rect());
}

} // namespace
