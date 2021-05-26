// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "zoomcombobox/zoomcombobox.h"
#include <KLocalizedString>
#include <QApplication>
#include <QInputMethod>
#include <QLineEdit>
#include <QDebug>

using namespace Gwenview;

class Gwenview::ZoomComboBoxPrivate
{
    Q_DECLARE_PUBLIC(ZoomComboBox)
public:
    ZoomComboBoxPrivate(ZoomComboBox *q);
    ZoomComboBox *const q_ptr;

    QString zoomValueText(int value) const;
    void undecorateText(QString &text);
    void updateZoomText();
    int positionBeforeSuffix() const;

    int value = -1;
    QIntValidator *validator = nullptr;

    // Using QString because QLocale::percent() will return a QString in Qt 6.
    QString percent;
};

ZoomComboBoxPrivate::ZoomComboBoxPrivate(ZoomComboBox *q)
    : q_ptr(q)
    , validator(new QIntValidator(-1, -1, q))
    , percent(q->locale().percent())
{
}

int ZoomComboBoxPrivate::positionBeforeSuffix() const
{
    Q_Q(const ZoomComboBox);
    return q->lineEdit()->displayText().length() - percent.length();
}

QString ZoomComboBoxPrivate::zoomValueText(int value) const
{
    Q_Q(const ZoomComboBox);
    return q->textFromValue(value).append(percent);
}

void Gwenview::ZoomComboBoxPrivate::undecorateText(QString& text)
{
    if (text.endsWith(percent)) {
        text.chop(percent.length());
    }
}

void Gwenview::ZoomComboBoxPrivate::updateZoomText()
{
    Q_Q(ZoomComboBox);
    // only run this for custom input
    if (q->currentIndex() > -1) {
        return;
    }
    auto lineEdit = q->lineEdit();
    const QSignalBlocker blocker(lineEdit);
    q->setEditText(q->textFromValue(value).append(percent));
    // if selectionLength is 0, sets the cursorPosition instead
    lineEdit->setSelection(qMin(lineEdit->cursorPosition(), positionBeforeSuffix()),
                           lineEdit->selectionLength());
}

ZoomComboBox::ZoomComboBox(QWidget* parent)
    : QComboBox(parent)
    , d_ptr(new ZoomComboBoxPrivate(this))
{
    Q_D(ZoomComboBox);
    QLineEdit *lineEdit = new QLineEdit(this);
    lineEdit->setInputMethodHints(Qt::ImhDigitsOnly);
    setLineEdit(lineEdit);
    setInsertPolicy(QComboBox::NoInsert);
    setMinimumContentsLength(locale().toString(9999).length() + d->percent.length());
    setEditable(true);
    // Don't use setValidator() because doing so makes it impossible to enter
    // non-integer combobox items. It's useful to be able to have non-integer
    // combobox items that can be selected via text input while only accepting
    // integers as custom input.
}

ZoomComboBox::~ZoomComboBox() noexcept
{
}

int ZoomComboBox::value() const
{
    Q_D(const ZoomComboBox);
    return d->value;
}

void ZoomComboBox::setValue(int value)
{
    Q_D(ZoomComboBox);
    value = qBound(minimum(), value, maximum());
    if (d->value == value) {
        return;
    }

    d->value = value;
    Q_EMIT valueChanged(d->value);
    d->updateZoomText();
}

int ZoomComboBox::minimum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->bottom();
}

void ZoomComboBox::setMinimum(int minimum)
{
    Q_D(ZoomComboBox);
    if (this->minimum() == minimum) {
        return;
    }
    d->validator->setBottom(minimum);
    // reset value if value is not between minimum and maximum
    setValue(d->value);
    Q_EMIT minimumChanged(minimum);
}

int ZoomComboBox::maximum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->top();
}

void ZoomComboBox::setMaximum(int maximum)
{
    Q_D(ZoomComboBox);
    if (this->maximum() == maximum) {
        return;
    }
    d->validator->setTop(maximum);
    // reset value if value is not between minimum and maximum
    setValue(d->value);
    Q_EMIT maximumChanged(maximum);
}

void ZoomComboBox::setCurrentText(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }
    const int i = findText(text);
    if (i > -1) {
        setCurrentIndex(i);
    } else {
        setValue(valueFromText(text));
    }
}

int ZoomComboBox::valueFromText(const QString& text) const
{
    QString copy = text.trimmed();
    int pos = lineEdit()->cursorPosition();
    const int oldpos = pos;
    QValidator::State state = validate(copy, pos);
    if (state != QValidator::Acceptable) {
        fixup(copy);
    }
    qDebug() << copy << state;
    return locale().toInt(copy);
}

QString ZoomComboBox::textFromValue(const int value) const
{
    Q_D(const ZoomComboBox);
    return locale().toString(value);
}

QValidator::State ZoomComboBox::validate(QString &input, int &pos) const
{
    Q_D(const ZoomComboBox);
    return d->validator->validate(input, pos);
}

void ZoomComboBox::fixup(QString &input) const
{
    Q_D(const ZoomComboBox);
    d->validator->fixup(input);
    input.remove(locale().groupSeparator());
}

void ZoomComboBox::changeEvent(QEvent* event)
{
    Q_D(ZoomComboBox);
    if (event->type() == QEvent::LocaleChange) {
        const QString& percent = locale().percent();
        if (d->percent != percent) {
            d->percent = percent;
            d->updateZoomText();
        }
    }
    return QComboBox::changeEvent(event);
}
