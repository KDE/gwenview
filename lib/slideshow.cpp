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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "slideshow.h"

// STL
#include <algorithm>
#include <ctime>

// Qt
#include <QAction>
#include <QTimer>

// KF
#include <KLocalizedString>

// Local
#include "gwenview_lib_debug.h"
#include <gwenviewconfig.h>
#include <lib/gvdebug.h>

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

enum State {
    Paused,
    Started,
    WaitForEndOfUrl,
};

/**
 * This class generate random numbers which are not the same between two runs
 * of Gwenview. See bug #132334
 */
class RandomNumberGenerator
{
public:
    RandomNumberGenerator()
        : mSeed(time(nullptr))
    {
    }

    int operator()(int n)
    {
        return rand_r(&mSeed) % n;
    }

private:
    unsigned int mSeed;
};

struct SlideShowPrivate {
    QTimer *mTimer;
    State mState;
    QVector<QUrl> mUrls;
    QVector<QUrl> mShuffledUrls;
    QVector<QUrl>::ConstIterator mStartIt;
    QUrl mCurrentUrl;
    QUrl mLastShuffledUrl;

    QAction *mLoopAction;
    QAction *mRandomAction;

    QUrl findNextUrl()
    {
        if (GwenviewConfig::random()) {
            return findNextRandomUrl();
        } else {
            return findNextOrderedUrl();
        }
    }

    QUrl findNextOrderedUrl()
    {
        QVector<QUrl>::ConstIterator it = std::find(mUrls.constBegin(), mUrls.constEnd(), mCurrentUrl);
        GV_RETURN_VALUE_IF_FAIL2(it != mUrls.constEnd(), QUrl(), "Current url not found in list.");

        ++it;
        if (GwenviewConfig::loop()) {
            // Looping, if we reach the end, start again
            if (it == mUrls.constEnd()) {
                it = mUrls.constBegin();
            }
        } else {
            // Not looping, have we reached the end?
            // FIXME: stopAtEnd
            if (/*(it==mUrls.end() && GwenviewConfig::stopAtEnd()) ||*/ it == mStartIt) {
                it = mUrls.constEnd();
            }
        }

        if (it != mUrls.constEnd()) {
            return *it;
        } else {
            return QUrl();
        }
    }

    void initShuffledUrls()
    {
        mShuffledUrls = mUrls;
        RandomNumberGenerator generator;
        std::random_shuffle(mShuffledUrls.begin(), mShuffledUrls.end(), generator);
        // Ensure the first url is different from the previous last one, so that
        // last url does not stay visible twice longer than usual
        if (mLastShuffledUrl == mShuffledUrls.first() && mShuffledUrls.count() > 1) {
            qSwap(mShuffledUrls[0], mShuffledUrls[1]);
        }
        mLastShuffledUrl = mShuffledUrls.last();
    }

    QUrl findNextRandomUrl()
    {
        if (mShuffledUrls.empty()) {
            if (GwenviewConfig::loop()) {
                initShuffledUrls();
            } else {
                return QUrl();
            }
        }

        QUrl url = mShuffledUrls.last();
        mShuffledUrls.pop_back();

        return url;
    }

    void updateTimerInterval()
    {
        mTimer->setInterval(int(GwenviewConfig::interval() * 1000));
    }

    void doStart()
    {
        if (MimeTypeUtils::urlKind(mCurrentUrl) == MimeTypeUtils::KIND_VIDEO) {
            LOG("mState = WaitForEndOfUrl");
            // Just in case
            mTimer->stop();
            mState = WaitForEndOfUrl;
        } else {
            LOG("mState = Started");
            mTimer->start();
            mState = Started;
        }
    }
};

SlideShow::SlideShow(QObject *parent)
    : QObject(parent)
    , d(new SlideShowPrivate)
{
    d->mState = Paused;

    d->mTimer = new QTimer(this);
    connect(d->mTimer, &QTimer::timeout, this, &SlideShow::goToNextUrl);

    d->mLoopAction = new QAction(this);
    d->mLoopAction->setText(i18nc("@item:inmenu toggle loop in slideshow", "Loop"));
    d->mLoopAction->setCheckable(true);
    connect(d->mLoopAction, &QAction::triggered, this, &SlideShow::updateConfig);

    d->mRandomAction = new QAction(this);
    d->mRandomAction->setText(i18nc("@item:inmenu toggle random order in slideshow", "Random"));
    d->mRandomAction->setCheckable(true);
    connect(d->mRandomAction, &QAction::toggled, this, &SlideShow::slotRandomActionToggled);
    connect(d->mRandomAction, &QAction::triggered, this, &SlideShow::updateConfig);

    d->mLoopAction->setChecked(GwenviewConfig::loop());
    d->mRandomAction->setChecked(GwenviewConfig::random());
}

SlideShow::~SlideShow()
{
    GwenviewConfig::self()->save();
    delete d;
}

QAction *SlideShow::loopAction() const
{
    return d->mLoopAction;
}

QAction *SlideShow::randomAction() const
{
    return d->mRandomAction;
}

void SlideShow::start(const QList<QUrl> &urls)
{
    d->mUrls.resize(urls.size());
    std::copy(urls.begin(), urls.end(), d->mUrls.begin());

    d->mStartIt = std::find(d->mUrls.constBegin(), d->mUrls.constEnd(), d->mCurrentUrl);
    if (d->mStartIt == d->mUrls.constEnd()) {
        qCWarning(GWENVIEW_LIB_LOG) << "Current url not found in list, aborting.\n";
        return;
    }

    if (GwenviewConfig::random()) {
        d->initShuffledUrls();
    }

    d->updateTimerInterval();
    d->mTimer->setSingleShot(false);
    d->doStart();
    Q_EMIT stateChanged(true);
}

void SlideShow::setInterval(int intervalInSeconds)
{
    GwenviewConfig::setInterval(double(intervalInSeconds));
    d->updateTimerInterval();
    Q_EMIT intervalChanged(intervalInSeconds);
}

int SlideShow::interval() const
{
    return GwenviewConfig::interval();
}

int SlideShow::position() const
{
    // TODO: also support videos

    // QTimer::remainingTime() returns -1 if inactive
    // and there are moments where mState == Started but timer already done but not yet next url reached
    // so handle that
    if (d->mState == Started) {
        if (d->mTimer->isActive()) {
            return interval() * 1000 - d->mTimer->remainingTime();
        }
        // already timeout reached, but not yet progressed to next url
        return interval();
    }

    return 0;
}

void SlideShow::pause()
{
    LOG("Stopping timer");
    d->mTimer->stop();
    d->mState = Paused;
    Q_EMIT stateChanged(false);
}

void SlideShow::resumeAndGoToNextUrl()
{
    LOG("");
    if (d->mState == WaitForEndOfUrl) {
        goToNextUrl();
    }
}

void SlideShow::goToNextUrl()
{
    LOG("");
    QUrl url = d->findNextUrl();
    LOG("url:" << url);
    if (!url.isValid()) {
        pause();
        return;
    }
    Q_EMIT goToUrl(url);
}

void SlideShow::setCurrentUrl(const QUrl &url)
{
    LOG(url);
    if (d->mCurrentUrl == url) {
        return;
    }
    d->mCurrentUrl = url;
    // Restart timer to avoid showing new url for the remaining time of the old
    // url
    if (d->mState != Paused) {
        d->doStart();
    }
}

bool SlideShow::isRunning() const
{
    return d->mState != Paused;
}

void SlideShow::updateConfig()
{
    GwenviewConfig::setLoop(d->mLoopAction->isChecked());
    GwenviewConfig::setRandom(d->mRandomAction->isChecked());
}

void SlideShow::slotRandomActionToggled(bool on)
{
    if (on && d->mState != Paused) {
        d->initShuffledUrls();
    }
}

} // namespace
