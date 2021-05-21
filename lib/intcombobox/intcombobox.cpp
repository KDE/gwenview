// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "intcombobox/intcombobox.h"
#include <QLineEdit>

using namespace Gwenview;

class Gwenview::IntComboBoxPrivate
{
    Q_DECLARE_PUBLIC(IntComboBox)
public:
    IntComboBoxPrivate(IntComboBox *q);
    IntComboBox *const q_ptr;

    int boundedValue(int v) const { return qBound(minimum, v, maximum); }

    int value = 0;
    int minimum = 0;
    int maximum = 99;
    QString suffix;

    QIntValidator *validator = nullptr;
};

IntComboBoxPrivate::IntComboBoxPrivate(IntComboBox *q)
    : q_ptr(q)
    , validator(new QIntValidator(minimum, maximum, q))
{
}

IntComboBox::IntComboBox(QWidget* parent)
    : QComboBox(parent)
    , d_ptr(new IntComboBoxPrivate(this))
{
    Q_D(IntComboBox);
    QLineEdit *lineEdit = new QLineEdit(this);
    lineEdit->setInputMethodHints(Qt::ImhDigitsOnly);
//     lineEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setLineEdit(lineEdit);
    setInsertPolicy(QComboBox::NoInsert);
    setMinimumContentsLength(locale().toString(1600).length());
    setValidator(d->validator);
    setEditable(true);
}

IntComboBox::~IntComboBox() noexcept
{
}

QString IntComboBox::cleanText() const
{
    Q_D(const IntComboBox);
    QString text = lineEdit()->displayText();
    if (text.endsWith(d->suffix)) {
        text.chop(d->suffix.length());
    }
    return text;
}

int IntComboBox::value() const
{
    Q_D(const IntComboBox);
    return d->value;
}

void IntComboBox::setValue(int value)
{
    Q_D(IntComboBox);
    value = d->boundedValue(value);
    if (d->value == value) {
        return;
    }

    d->value = value;
    Q_EMIT valueChanged(d->value);
    setCurrentText(textFromValue(value).append(d->suffix));
}

void IntComboBox::setValue(const QString& text)
{
    Q_D(IntComboBox);
    if (currentText() == text || text.isEmpty()) {
        return;
    }
    setValue(valueFromText(text));
}

int IntComboBox::minimum() const
{
    Q_D(const IntComboBox);
    return d->minimum;
}

void IntComboBox::setMinimum(int minimum)
{
    Q_D(IntComboBox);
    setRange(minimum, d->maximum);
}

int IntComboBox::maximum() const
{
    Q_D(const IntComboBox);
    return d->maximum;
}

void IntComboBox::setMaximum(int maximum)
{
    Q_D(IntComboBox);
    setRange(d->minimum, maximum);
}

// NOTE: using IntComboBox::setRange() to do IntComboBox::setMinimum() and
// IntComboBox::setMaximum() instead of vice versa prevents QValidator::changed()
// from being emitted twice when IntComboBox::setRange() is called.
void IntComboBox::setRange(int minimum, int maximum)
{
    Q_D(IntComboBox);
    if (d->minimum != minimum) {
        d->minimum = minimum;
        Q_EMIT minimumChanged(minimum);
    }

    if (d->maximum != maximum) {
        d->maximum = maximum;
        Q_EMIT maximumChanged(maximum);
    }
    d->validator->setRange(minimum, maximum);
    // reset value if value is not between minimum and maximum
    setValue(d->value);
}


QString IntComboBox::suffix() const
{
    Q_D(const IntComboBox);
    return d->suffix;
}

void IntComboBox::setSuffix(const QString& suffix)
{
    Q_D(IntComboBox);
    if (d->suffix == suffix) {
        return;
    }

    d->suffix = suffix;
    Q_EMIT suffixChanged(d->suffix);
}

int IntComboBox::valueFromText(const QString& text) const
{
    Q_D(const IntComboBox);
    QString copy = text.trimmed();
    int pos = 0;
    QValidator::State state = validate(copy, pos);
    if (state != QValidator::Acceptable) {
        d->validator->fixup(copy);
    }
    return locale().toInt(copy);
}

QString IntComboBox::textFromValue(const int value) const
{
    Q_D(const IntComboBox);
    return locale().toString(value);
}

QValidator::State IntComboBox::validate(QString &input, int &pos) const
{
    Q_D(const IntComboBox);
    return d->validator->validate(input, pos);
}

void IntComboBox::fixup(QString &input) const
{
    Q_D(const IntComboBox);
    d->validator->fixup(input);
}

void IntComboBox::setValidator(QIntValidator *validator)
{
    Q_D(IntComboBox);
    if (d->validator != validator) {
        d->validator = validator;
    }
    QComboBox::setValidator(validator);
}
