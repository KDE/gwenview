// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "zoomcombobox.h"
#include "zoomcombobox_p.h"

#include <QAbstractItemView>
#include <QAction>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QWheelEvent>

#include <cmath>

using namespace Gwenview;

ZoomValidator::ZoomValidator(qreal minimum, qreal maximum, ZoomComboBox *q, ZoomComboBoxPrivate *d, QWidget *parent)
    : QValidator(parent)
    , m_minimum(minimum)
    , m_maximum(maximum)
    , m_zoomComboBox(q)
    , m_zoomComboBoxPrivate(d)
{
}

ZoomValidator::~ZoomValidator() noexcept = default;

qreal ZoomValidator::minimum() const
{
    return m_minimum;
}

void ZoomValidator::setMinimum(const qreal minimum)
{
    if (m_minimum == minimum) {
        return;
    }
    m_minimum = minimum;
    Q_EMIT changed();
}

qreal ZoomValidator::maximum() const
{
    return m_maximum;
}

void ZoomValidator::setMaximum(const qreal maximum)
{
    if (m_maximum == maximum) {
        return;
    }
    m_maximum = maximum;
    Q_EMIT changed();
}

QValidator::State ZoomValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)
    if (m_zoomComboBox->findText(input, Qt::MatchFixedString) > -1) {
        return QValidator::Acceptable;
    }

    QString copy = input.trimmed();
    copy.remove(locale().groupSeparator());
    copy.remove(locale().percent());
    const bool startsWithNumber = copy.constBegin()->isNumber();

    if (startsWithNumber || copy.isEmpty()) {
        return QValidator::Intermediate;
    }

    QValidator::State state;
    bool ok = false;
    int value = locale().toInt(copy, &ok);
    if (!ok || value < std::ceil(m_minimum * 100) || value > std::floor(m_maximum * 100)) {
        state = QValidator::Intermediate;
    } else {
        state = QValidator::Acceptable;
    }
    return state;
}

ZoomComboBoxPrivate::ZoomComboBoxPrivate(ZoomComboBox *q)
    : q_ptr(q)
    , validator(new ZoomValidator(0, 0, q, this))
{
}

ZoomComboBox::ZoomComboBox(QWidget *parent)
    : QComboBox(parent)
    , d_ptr(new ZoomComboBoxPrivate(this))
{
    Q_D(ZoomComboBox);

    d->validator->setObjectName(QLatin1String("zoomValidator"));
    setValidator(d->validator);

    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    // QLocale::percent() will return a QString in Qt 6.
    // Qt encourages using QString(locale().percent()) in QLocale documentation.
    int percentLength = QString(locale().percent()).length();
    setMinimumContentsLength(locale().toString(9999).length() + percentLength);

    connect(lineEdit(), &QLineEdit::textEdited, this, [this, d](const QString &text) {
        const bool startsWithNumber = text.constBegin()->isNumber();
        int matches = 0;
        for (int i = 0; i < count(); ++i) {
            if (itemText(i).startsWith(text, Qt::CaseInsensitive)) {
                matches += 1;
            }
        }
        Qt::MatchFlags matchFlags = startsWithNumber || matches > 1 ? Qt::MatchFixedString : Qt::MatchStartsWith;
        const int textIndex = findText(text, matchFlags);
        if (textIndex < 0) {
            d->value = valueFromText(text);
        }
        if (textIndex >= 0 || d->value >= minimum()) {
            // update only when doing so would not change the typed text due
            // to being below the mininum as that means we can't type a new percentage
            // one key at a time.
            if (textIndex < 0)
                d->lastCustomZoomValue = d->value;
            updateDisplayedText();
            activateAndChangeZoomTo(textIndex);
            lineEdit()->setCursorPosition(lineEdit()->cursorPosition() - 1);
        }
    });
    connect(this, qOverload<int>(&ZoomComboBox::highlighted), this, &ZoomComboBox::changeZoomTo);
    view()->installEventFilter(this);
    connect(this, qOverload<int>(&ZoomComboBox::activated), this, &ZoomComboBox::activateAndChangeZoomTo);
}

ZoomComboBox::~ZoomComboBox() noexcept = default;

void ZoomComboBox::setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction)
{
    Q_D(ZoomComboBox);
    d->setActions(zoomToFitAction, zoomToFillAction, actualSizeAction);

    connect(zoomToFitAction, &QAction::toggled, this, &ZoomComboBox::updateDisplayedText);
    connect(zoomToFillAction, &QAction::toggled, this, &ZoomComboBox::updateDisplayedText);
}

void ZoomComboBoxPrivate::setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction)
{
    Q_Q(ZoomComboBox);
    q->clear();
    q->addItem(zoomToFitAction->iconText(), QVariant::fromValue(zoomToFitAction)); // index = 0
    q->addItem(zoomToFillAction->iconText(), QVariant::fromValue(zoomToFillAction)); // index = 1
    q->addItem(actualSizeAction->iconText(), QVariant::fromValue(actualSizeAction)); // index will change later

    mZoomToFitAction = zoomToFitAction;
    mZoomToFillAction = zoomToFillAction;
    mActualSizeAction = actualSizeAction;
}

qreal ZoomComboBox::value() const
{
    Q_D(const ZoomComboBox);
    return d->value;
}

void ZoomComboBox::setValue(qreal value)
{
    Q_D(ZoomComboBox);
    d->value = value;

    updateDisplayedText();
}

qreal ZoomComboBox::minimum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->minimum();
}

void ZoomComboBox::setMinimum(qreal minimum)
{
    Q_D(ZoomComboBox);
    if (this->minimum() == minimum) {
        return;
    }
    d->validator->setMinimum(minimum);
    setValue(qMax(minimum, d->value));
    // Generate zoom presets below 100%
    // FIXME: combobox value gets reset to last index value when this code runs
    const int zoomToFillActionIndex = findData(QVariant::fromValue(d->mZoomToFillAction));
    const int actualSizeActionIndex = findData(QVariant::fromValue(d->mActualSizeAction));
    for (int i = actualSizeActionIndex - 1; i > zoomToFillActionIndex; --i) {
        removeItem(i);
    }
    qreal value = minimum * 2; // The minimum zoom value itself is already available through "fit".
    for (int i = zoomToFillActionIndex + 1; value < 1.0; ++i) {
        insertItem(i, textFromValue(value), QVariant::fromValue(value));
        value *= 2;
    }
}

qreal ZoomComboBox::maximum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->maximum();
}

void ZoomComboBox::setMaximum(qreal maximum)
{
    Q_D(ZoomComboBox);
    if (this->maximum() == maximum) {
        return;
    }
    d->validator->setMaximum(maximum);
    setValue(qMin(d->value, maximum));
    // Generate zoom presets above 100%
    // NOTE: This probably has the same problem as setMinimum(),
    // but the problem is never enountered since max zoom doesn't actually change
    const int actualSizeActionIndex = findData(QVariant::fromValue(d->mActualSizeAction));
    const int count = this->count();
    for (int i = actualSizeActionIndex + 1; i < count; ++i) {
        removeItem(i);
    }
    qreal value = 2.0;
    while (value < maximum) {
        addItem(textFromValue(value), QVariant::fromValue(value));
        value *= 2;
    }
    if (value >= maximum) {
        addItem(textFromValue(maximum), QVariant::fromValue(maximum));
    }
}

qreal ZoomComboBox::valueFromText(const QString &text, bool *ok) const
{
    Q_D(const ZoomComboBox);
    QString copy = text;
    copy.remove(locale().groupSeparator());
    copy.remove(locale().percent());
    return qreal(locale().toInt(copy, ok)) / 100.0;
}

QString ZoomComboBox::textFromValue(const qreal value) const
{
    Q_D(const ZoomComboBox);
    QString text = locale().toString(qRound(value * 100));
    d->validator->fixup(text);
    text.remove(locale().groupSeparator());
    return text.append(locale().percent());
}

void ZoomComboBox::updateDisplayedText()
{
    Q_D(ZoomComboBox);
    if (d->mZoomToFitAction->isChecked()) {
        lineEdit()->setText(d->mZoomToFitAction->iconText());
    } else if (d->mZoomToFillAction->isChecked()) {
        lineEdit()->setText(d->mZoomToFillAction->iconText());
    } else if (d->mActualSizeAction->isChecked()) {
        lineEdit()->setText(d->mActualSizeAction->iconText());
    } else {
        lineEdit()->setText(textFromValue(d->value));
    }
}

void Gwenview::ZoomComboBox::showPopup()
{
    updateCurrentIndex();

    // We don't want to emit a QComboBox::highlighted event just because the popup is opened.
    const QSignalBlocker blocker(this);
    QComboBox::showPopup();
}

bool ZoomComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == view()) {
        switch (event->type()) {
        case QEvent::Hide: {
            Q_D(ZoomComboBox);
            changeZoomTo(d->lastSelectedIndex);
            break;
        }
        case QEvent::ShortcutOverride: {
            if (view()->isVisibleTo(this)) {
                auto keyEvent = static_cast<QKeyEvent *>(event);
                if (keyEvent->key() == Qt::Key_Escape) {
                    event->accept();
                }
            }
            break;
        }
        default:
            break;
        }
    }

    return QComboBox::eventFilter(watched, event);
}

void ZoomComboBox::focusOutEvent(QFocusEvent *)
{
    Q_D(ZoomComboBox);
    // Should the user have started typing a custom value
    // that was out of our range, then we have a temporary
    // state that is illegal. This is needed to allow a user
    // to type a zoom with multiple keystrokes, but when the
    // user leaves focus we should reset to the last known 'good'
    // zoom value.
    if (d->lastSelectedIndex == -1)
        setValue(d->lastCustomZoomValue);
}

void ZoomComboBox::wheelEvent(QWheelEvent *event)
{
    updateCurrentIndex();
    if (currentIndex() != -1) {
        // Everything should work as expected.
        return QComboBox::wheelEvent(event);
    }

    // There is no exact match for the current zoom value in the
    // ComboBox. We need to find the closest matches, so scrolling
    // works as expected.
    Q_D(ZoomComboBox);
    int closestZoomValueSmallerThanCurrent = 1; // = "fill" action
    int closestZoomValueBiggerThanCurrent = count() - 1;

    for (int i = 2; itemData(i) != QVariant::Invalid; i++) {
        bool isValidConversion;
        const qreal zoomValue = valueFromText(itemText(i), &isValidConversion);
        if (!isValidConversion) {
            continue;
        }

        if (zoomValue < d->value) {
            closestZoomValueSmallerThanCurrent = i;
        } else {
            closestZoomValueBiggerThanCurrent = i;
            break;
        }
    }
    if (event->angleDelta().y() > 0) {
        setCurrentIndex(closestZoomValueBiggerThanCurrent);
    } else {
        setCurrentIndex(closestZoomValueSmallerThanCurrent);
    }
    return QComboBox::wheelEvent(event);
}

void ZoomComboBox::updateCurrentIndex()
{
    Q_D(ZoomComboBox);

    if (d->mZoomToFitAction->isChecked()) {
        setCurrentIndex(0);
        d->lastSelectedIndex = 0;
    } else if (d->mZoomToFillAction->isChecked()) {
        setCurrentIndex(1);
        d->lastSelectedIndex = 1;
    } else if (d->mActualSizeAction->isChecked()) {
        const int actualSizeActionIndex = findData(QVariant::fromValue(d->mActualSizeAction));
        setCurrentIndex(actualSizeActionIndex);
        d->lastSelectedIndex = actualSizeActionIndex;
    } else {
        // Now is a good time to save the zoom value that was selected before the user changes it through the popup.
        d->lastCustomZoomValue = d->value;

        // Highlight the correct index if the current zoom value exists as an option in the popup.
        // If it doesn't exist, it is set to -1.
        d->lastSelectedIndex = findText(textFromValue(d->value));
        setCurrentIndex(d->lastSelectedIndex);
    }
}

void ZoomComboBox::changeZoomTo(int index)
{
    if (index < 0) {
        Q_D(ZoomComboBox);
        Q_EMIT zoomChanged(d->lastCustomZoomValue);
        return;
    }

    QVariant itemData = this->itemData(index);
    auto action = itemData.value<QAction *>();
    if (action) {
        if (!action->isCheckable() || !action->isChecked()) {
            action->trigger();
        }
    } else if (itemData.canConvert(QMetaType::QReal)) {
        Q_EMIT zoomChanged(itemData.toReal());
    }
}

void ZoomComboBox::activateAndChangeZoomTo(int index)
{
    Q_D(ZoomComboBox);
    d->lastSelectedIndex = index;

    // The user has explicitly selected this zoom value so we
    // remember it the same way as if they had typed it themselves.
    QVariant itemData = this->itemData(index);
    if (!itemData.value<QAction *>() && itemData.canConvert(QMetaType::QReal)) {
        d->lastCustomZoomValue = itemData.toReal();
    }

    changeZoomTo(index);
}
