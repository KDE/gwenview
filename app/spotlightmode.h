/*
This file is part of the KDE project
SPDX-License-Identifier: GPL-2.0-or-later

Project idea and initial maintainer: Aurélien Gâteau <agateau@kde.org>
SPDX-FileCopyrightText: 2024 Ravil Saifullin <saifullin.dev@gmail.com>

*/
#ifndef SPOTLIGHTMODE_H
#define SPOTLIGHTMODE_H

// Qt
#include <QHBoxLayout>
#include <QWidget>

// KF
#include <KActionCollection>

namespace Gwenview
{
struct SpotlightModePrivate;
class SpotlightMode : public QHBoxLayout
{
    Q_OBJECT
public:
    SpotlightMode(QWidget *parent, KActionCollection *actionCollection);
    void setVisibleSpotlightModeQuitButton(bool visibe);
    ~SpotlightMode() override;

private:
    void emitButtonQuitClicked();
    SpotlightModePrivate *const d;
};

} // namespace

#endif /* SPOTLIGHTMODE_H */
