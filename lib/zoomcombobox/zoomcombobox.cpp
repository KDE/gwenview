// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "zoomcombobox.h"
#include "zoomcombobox_p.h"
#include <KLocalizedString>
#include <QApplication>
#include <QInputMethod>
#include <QLineEdit>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

using namespace Gwenview;

ZoomValidator::ZoomValidator(int minimum, int maximum, ZoomComboBox *q, ZoomComboBoxPrivate *d, QWidget* parent)
    : QValidator(parent)
    , m_minimum(minimum)
    , m_maximum(maximum)
    , m_zoomComboBox(q)
    , m_zoomComboBoxPrivate(d)
{
}

ZoomValidator::~ZoomValidator() noexcept
{
}

int ZoomValidator::minimum() const
{
    return m_minimum;
}

void ZoomValidator::setMinimum(const int minimum)
{
    if (m_minimum == minimum) {
        return;
    }
    m_minimum = minimum;
    Q_EMIT minimumChanged(minimum);
    Q_EMIT changed();
}

int ZoomValidator::maximum() const
{
    return m_maximum;
}

void ZoomValidator::setMaximum(const int maximum)
{
    if (m_maximum == maximum) {
        return;
    }
    m_maximum = maximum;
    Q_EMIT maximumChanged(maximum);
    Q_EMIT changed();
}

QString ZoomValidator::percentSymbol() const
{
    return m_percentSymbol;
}

QValidator::State ZoomValidator::validate(QString& input, int& pos) const
{
    if (input.isEmpty()) {
        return QValidator::Invalid;
    }
    if (m_zoomComboBoxPrivate->currentTextIndex() > -1) {
        return QValidator::Acceptable;
    }
    input = input.trimmed();
    const QString groupSeparator = locale().groupSeparator();
    const QString percent = locale().percent();
    input.remove(groupSeparator);
    input.remove(percent);
    bool ok;
    int value = locale().toInt(input, &ok);
    if (!ok || value < m_minimum || value > m_maximum) {
        return QValidator::Invalid;
    }
    if (!input.endsWith(percent)) {
        return QValidator::Intermediate;
    }
    input = locale().toString(value);
    pos = qMin(pos, input.length());
    input.append(percent);
    return QValidator::Acceptable;//m_zoomComboBox->validate(input, pos);
}

void ZoomValidator::fixup(QString& input) const
{
    input = input.trimmed();
    if(!input.endsWith(m_percentSymbol)) {
        input.append(m_percentSymbol);
    }
//     m_zoomComboBox->fixup(input);
}

bool ZoomValidator::event(QEvent* event)
{
    if (event->type() == QEvent::LocaleChange) {
        const QString percentSymbol = locale().percent();
        if (m_percentSymbol != percentSymbol) {
            m_percentSymbol = percentSymbol;
            Q_EMIT percentSymbolChanged(m_percentSymbol);
        }
    }
    return QValidator::event(event);
}

ZoomComboBoxPrivate::ZoomComboBoxPrivate(ZoomComboBox *q)
    : q_ptr(q)
    , lineEdit(new QLineEdit(q))
    , validator(new ZoomValidator(0, 0, q, this))
{
}

void Gwenview::ZoomComboBoxPrivate::updateLineEdit()
{
    Q_Q(ZoomComboBox);
    // only run this for custom input
    if (currentTextIndex() > -1) {
        return;
    }
    const QSignalBlocker blocker(lineEdit);
    int cursorPosition = lineEdit->cursorPosition();
    const int selectionLength = lineEdit->selectionLength();
    QString text = q->textFromValue(value);
    QValidator::State state = validator->validate(text, cursorPosition);
    if (state == QValidator::Acceptable) {
        validator->fixup(text);
        q->setEditText(text);
        if (selectionLength > 0) {
            lineEdit->setSelection(cursorPosition, selectionLength);
        }
        q->update();
    }
}

int Gwenview::ZoomComboBoxPrivate::currentTextIndex() const
{
    Q_Q(const ZoomComboBox);
    return q->findText(q->currentText(), Qt::MatchFixedString);
}


ZoomComboBox::ZoomComboBox(QWidget* parent)
    : QComboBox(parent)
    , d_ptr(new ZoomComboBoxPrivate(this))
{
    Q_D(ZoomComboBox);

    d->lineEdit->setObjectName(QLatin1String("zoomLineEdit"));
    d->lineEdit->setInputMethodHints(Qt::ImhDigitsOnly);
    setLineEdit(d->lineEdit);

    d->validator->setObjectName(QLatin1String("zoomValidator"));
    setValidator(d->validator);

    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    int percentLength = d->validator->percentSymbol().length();
    setMinimumContentsLength(locale().toString(9999).length() + percentLength);

//     connect(d->lineEdit, &QLineEdit::selectionChanged, this, [this, d, percentLength](){
//         
//         if (d->currentTextIndex() == -1 && d->lineEdit->selectionEnd() > lineEdit()->text().length() - percentLength) {
//             
//         }
//     });
//     connect(d->lineEdit, &QLineEdit::textChanged, this, [this, d](const QString &text){
//         d->updateLineEdit();
//     });
//     connect(d->lineEdit, &QLineEdit::textEdited, this, [this, d](const QString &text){
//         setCurrentText(text);
//     });
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
    Q_EMIT valueChanged(value);
    d->updateLineEdit();
}

int ZoomComboBox::minimum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->minimum();
}

void ZoomComboBox::setMinimum(int minimum)
{
    Q_D(ZoomComboBox);
    if (this->minimum() == minimum) {
        return;
    }
    d->validator->setMinimum(minimum);
    // reset value if value is not between minimum and maximum
    setValue(d->value);
    Q_EMIT minimumChanged(minimum);
}

int ZoomComboBox::maximum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->maximum();
}

void ZoomComboBox::setMaximum(int maximum)
{
    Q_D(ZoomComboBox);
    if (this->maximum() == maximum) {
        return;
    }
    d->validator->setMaximum(maximum);
    // reset value if value is not between minimum and maximum
    setValue(d->value);
    Q_EMIT maximumChanged(maximum);
}

void ZoomComboBox::setCurrentText(const QString& text)
{
    Q_D(ZoomComboBox);
    if (text.isEmpty()) {
        return;
    }
    const int i = d->currentTextIndex();
    if (i > -1) {
        setCurrentIndex(i);
    } else if (lineEdit()->hasAcceptableInput()) {
        setValue(valueFromText(text));
    }
}

int ZoomComboBox::valueFromText(const QString& text) const
{
    Q_D(const ZoomComboBox);
    QString copy = text;
    int pos = lineEdit()->cursorPosition();
    qDebug() << "before validate()" << copy;
    qDebug() << "index in validate()" << d->currentTextIndex();
    QValidator::State state = d->validator->validate(copy, pos);
    qDebug() << "after validate()" << copy << state;
    return locale().toInt(copy);
}

QString ZoomComboBox::textFromValue(const int value) const
{
    Q_D(const ZoomComboBox);
    QString text = locale().toString(value);
    d->validator->fixup(text);
    return text;
}

QValidator::State Gwenview::ZoomComboBox::validate(QString& input, int& pos) const
{
    Q_D(const ZoomComboBox);
    return d->validator->validate(input, pos);
}

void Gwenview::ZoomComboBox::fixup(QString& input) const
{
    Q_D(const ZoomComboBox);
    d->validator->fixup(input);
}
