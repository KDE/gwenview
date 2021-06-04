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
    Q_EMIT changed();
}

QString ZoomValidator::percentSymbol() const
{
    return m_percentSymbol;
}

QValidator::State ZoomValidator::validate(QString& input, int& pos) const
{
    if (m_zoomComboBoxPrivate->getTextIndex(input) > -1) {
        return QValidator::Acceptable;
    }
    if(!input.endsWith(m_percentSymbol)) {
        input.append(m_percentSymbol);
    }
//     QString copy = input;
//     bool ok;
//     int value = m_zoomComboBox->valueFromText(input, &ok);
//     qDebug() << QStringLiteral("input: %1, ok: %2, value: %3, min: %4, max: %5").arg(input).arg(ok).arg(value).arg(m_minimum).arg(m_maximum);
//     if (!ok || value < m_minimum || value > m_maximum) {
//         return QValidator::Intermediate;
//     }
// 
//     input = locale().toString(value);
//     input.append(m_percentSymbol);
//     input = m_zoomComboBox->textFromValue(value);
    return m_zoomComboBox->validate(input, pos);
}

void ZoomValidator::fixup(QString& input) const
{
    input = input.trimmed();
//     if (m_zoomComboBoxPrivate->getTextIndex(input) > -1) {
//         return;
//     }
//     int value = qBound(m_minimum, locale().toInt(input), m_maximum);
//     if (value != m_zoomComboBoxPrivate->value) {
//         input = locale().toString(value);
//     }
    if(!input.endsWith(m_percentSymbol)) {
        input.append(m_percentSymbol);
    }
}

bool ZoomValidator::event(QEvent* event)
{
    if (event->type() == QEvent::LocaleChange) {
        const QString groupSeparator = locale().groupSeparator();
        const QString percentSymbol = locale().percent();
        if (m_groupSeparator != groupSeparator
            || m_percentSymbol != percentSymbol) {
            m_groupSeparator = groupSeparator;
            m_percentSymbol = percentSymbol;
            Q_EMIT changed();
        }
    }
    return QValidator::event(event);
}

ZoomComboBoxPrivate::ZoomComboBoxPrivate(ZoomComboBox *q)
    : q_ptr(q)
    , lineEdit(new QLineEdit(q))
    , validator(new ZoomValidator(0, 0, q, this))
    , textIndex(getTextIndex(q->currentText()))
{
}

void ZoomComboBoxPrivate::updateTextFromValue(int value)
{
    Q_Q(ZoomComboBox);
}

void ZoomComboBoxPrivate::updateValueFromText(const QString &text)
{
    Q_Q(ZoomComboBox);
    q->setValue(q->valueFromText(text));
}

void Gwenview::ZoomComboBoxPrivate::updateLineEdit()
{
    Q_Q(ZoomComboBox);
    // only run this for custom input
    if (getTextIndex(lineEdit->text()) > -1) {
        return;
    }
    const QSignalBlocker blocker(lineEdit);
    int cursorPosition = lineEdit->cursorPosition();
    const int selectionLength = lineEdit->selectionLength();
    
//     QValidator::State state = validator->validate(text, cursorPosition);
//     if (state == QValidator::Acceptable) {
//         validator->fixup(text);
//         q->setEditText(text);
//         if (selectionLength > 0) {
//             lineEdit->setSelection(cursorPosition, selectionLength);
//         }
//         q->update();
//     }
}

int Gwenview::ZoomComboBoxPrivate::getTextIndex(const QString &text) const
{
    Q_Q(const ZoomComboBox);
    return q->findText(text, Qt::MatchFixedString);
}


ZoomComboBox::ZoomComboBox(QWidget* parent)
    : QComboBox(parent)
    , d_ptr(new ZoomComboBoxPrivate(this))
{
    Q_D(ZoomComboBox);

    d->lineEdit->setObjectName(QLatin1String("zoomLineEdit"));
    setLineEdit(d->lineEdit);

    d->validator->setObjectName(QLatin1String("zoomValidator"));
    setValidator(d->validator);

    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    int percentLength = d->validator->percentSymbol().length();
    setMinimumContentsLength(locale().toString(9999).length() + percentLength);

    connect(d->validator, &QValidator::changed, this, [this, d](){
        if (d->textIndex == -1) {
            setValue(d->value);
        }
    });
    connect(d->lineEdit, &QLineEdit::textEdited, this, [this, d](const QString &text){
        d->textIndex = d->getTextIndex(text);
        if (d->textIndex > -1) {
            setCurrentIndex(d->textIndex);
        } else {
            setValue(valueFromText(text));
        }
    });
    // NOTE: textChanged is emitted after textEdited
    connect(d->lineEdit, &QLineEdit::textChanged, this, [this, d](const QString &text){
        d->textIndex = d->getTextIndex(text);
    });
//     connect(d->lineEdit, &QLineEdit::selectionChanged, this, [this, d, percentLength](){
//         
//         if (d->currentTextIndex() == -1 && d->lineEdit->selectionEnd() > lineEdit()->text().length() - percentLength) {
//             
//         }
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
    setEditText(textFromValue(d->value));
    Q_EMIT valueChanged(value);
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

int ZoomComboBox::valueFromText(const QString& text, bool *ok) const
{
    Q_D(const ZoomComboBox);
    QString copy = text;
    copy.remove(locale().groupSeparator());
    copy.remove(locale().percent());
    return locale().toInt(copy, ok);
}

QString ZoomComboBox::textFromValue(const int value) const
{
    Q_D(const ZoomComboBox);
    QString text = locale().toString(value);
    d->validator->fixup(text);
    text.remove(locale().groupSeparator());
    return text;
}

QValidator::State Gwenview::ZoomComboBox::validate(QString& input, int& pos) const
{
    Q_D(const ZoomComboBox);
    if (d->getTextIndex(input) > -1) {
        pos = qMin(pos, input.length());
        return QValidator::Acceptable;
    }
    QValidator::State state;
    QString copy = input.trimmed();
    copy.remove(locale().groupSeparator());
    copy.remove(locale().percent());
    bool ok = false;
    int value = locale().toInt(copy, &ok);
    if (!ok || copy.isEmpty() || value < minimum() || value > maximum()) {
        state = QValidator::Intermediate;
    } else {
        state = QValidator::Acceptable;
    }
    input = copy.append(locale().percent());
    pos = qMin(pos, copy.length());

    return state;
}

void Gwenview::ZoomComboBox::fixup(QString& input) const
{
    Q_D(const ZoomComboBox);
    d->validator->fixup(input);
}
