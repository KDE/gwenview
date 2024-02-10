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
