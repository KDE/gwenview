// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "zoomcombobox.h"
#include "zoomcombobox_p.h"

#include <QAbstractItemView>
#include <QAction>
#include <QEvent>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QWheelEvent>

#include <cmath>

bool fuzzyEqual(qreal a, qreal b)
{
    return (qFuzzyIsNull(a) && qFuzzyIsNull(b)) || qFuzzyCompare(a, b);
}

bool fuzzyLessEqual(qreal a, qreal b)
{
    return fuzzyEqual(a, b) || a < b;
}

bool fuzzyGreaterEqual(qreal a, qreal b)
{
    return fuzzyEqual(a, b) || a > b;
}

using namespace Gwenview;

struct LineEditSelectionKeeper {
    LineEditSelectionKeeper(QLineEdit *lineEdit)
        : m_lineEdit(lineEdit)
    {
        Q_ASSERT(m_lineEdit);
        m_cursorPos = m_lineEdit->cursorPosition();
    }

    ~LineEditSelectionKeeper()
    {
        m_lineEdit->end(false);
        m_lineEdit->cursorBackward(true, m_lineEdit->text().length() - m_cursorPos);
    }

private:
    QLineEdit *m_lineEdit;
    int m_cursorPos;
};

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
    if (fuzzyEqual(m_minimum, minimum)) {
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
    if (fuzzyEqual(m_maximum, maximum)) {
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
    , validator(new ZoomValidator(0, 0, q, this, q))
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
    const int percentLength = QString(locale().percent()).length();
    setMinimumContentsLength(locale().toString(9999).length() + percentLength);

    connect(lineEdit(), &QLineEdit::textEdited, this, [this, d](const QString &text) {
        const bool startsWithNumber = text.constBegin()->isNumber();
        int matchedIndex = -1;
        if (startsWithNumber) {
            matchedIndex = findText(text, Qt::MatchFixedString);
        } else {
            // check if there is more than 1 match
            for (int i = 0, n = count(); i < n; ++i) {
                if (itemText(i).startsWith(text, Qt::CaseInsensitive)) {
                    if (matchedIndex != -1) {
                        // there is more than 1 match
                        return;
                    }
                    matchedIndex = i;
                }
            }
        }
        if (matchedIndex != -1) {
            LineEditSelectionKeeper selectionKeeper(lineEdit());
            updateCurrentIndex();
            if (matchedIndex == currentIndex()) {
                updateDisplayedText();
            } else {
                activateAndChangeZoomTo(matchedIndex);
            }
        } else if (startsWithNumber) {
            bool ok = false;
            qreal value = valueFromText(text, &ok);
            if (ok && value > 0 && fuzzyLessEqual(value, maximum())) {
                // emulate autocompletion for valid values that aren't predefined
                while (value < minimum()) {
                    value *= 10;
                    if (value > maximum()) {
                        // autocompletion cannot be emulated for this value
                        return;
                    }
                }
                LineEditSelectionKeeper selectionKeeper(lineEdit());
                if (fuzzyEqual(value, d->value)) {
                    updateDisplayedText();
                } else {
                    d->lastCustomZoomValue = value;
                    activateAndChangeZoomTo(-1);
                }
            }
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
    if (fuzzyEqual(this->minimum(), minimum)) {
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
    if (fuzzyEqual(this->maximum(), maximum)) {
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
    if (fuzzyGreaterEqual(value, maximum)) {
        addItem(textFromValue(maximum), QVariant::fromValue(maximum));
    }
}

qreal ZoomComboBox::valueFromText(const QString &text, bool *ok) const
{
    Q_D(const ZoomComboBox);

    const QLocale l = locale();
    QString s = text;
    s.remove(l.groupSeparator());
    if (s.endsWith(l.percent())) {
        s = s.chopped(1);
    }

    return l.toDouble(s, ok) / 100.0;
}

QString ZoomComboBox::textFromValue(const qreal value) const
{
    Q_D(const ZoomComboBox);

    QLocale l = locale();
    l.setNumberOptions(QLocale::OmitGroupSeparator);

    return l.toString(qRound(value * 100)).append(l.percent());
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

void ZoomComboBox::focusOutEvent(QFocusEvent *event)
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

    QComboBox::focusOutEvent(event);
}

void ZoomComboBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp: {
        updateCurrentIndex();
        if (currentIndex() != -1) {
            break;
        }
        moveCurrentIndex(event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown);
        return;
    }
    default:
        break;
    }

    QComboBox::keyPressEvent(event);
}

void ZoomComboBox::wheelEvent(QWheelEvent *event)
{
    updateCurrentIndex();
    if (currentIndex() != -1) {
        // Everything should work as expected.
        QComboBox::wheelEvent(event);
        return;
    }

    moveCurrentIndex(event->angleDelta().y() < 0);
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

void ZoomComboBox::moveCurrentIndex(bool moveUp)
{
    // There is no exact match for the current zoom value in the
    // ComboBox. We need to find the closest matches, so scrolling
    // works as expected.
    Q_D(ZoomComboBox);

    int newIndex;
    if (moveUp) {
        // switch to a larger item
        newIndex = count() - 1;
        for (int i = 2; i < newIndex; ++i) {
            const QVariant data = itemData(i);
            qreal value;
            if (data.value<QAction *>() == d->mActualSizeAction) {
                value = 1;
            } else {
                value = data.value<qreal>();
            }
            if (value > d->value) {
                newIndex = i;
                break;
            }
        }
    } else {
        // switch to a smaller item
        newIndex = 1;
        for (int i = count() - 1; i > newIndex; --i) {
            const QVariant data = itemData(i);
            qreal value;
            if (data.value<QAction *>() == d->mActualSizeAction) {
                value = 1;
            } else {
                value = data.value<qreal>();
            }
            if (value < d->value) {
                newIndex = i;
                break;
            }
        }
    }
    setCurrentIndex(newIndex);
    Q_EMIT activated(newIndex);
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

#include "moc_zoomcombobox.cpp"

#include "moc_zoomcombobox_p.cpp"
