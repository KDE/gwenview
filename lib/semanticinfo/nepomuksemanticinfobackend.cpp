// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "nepomuksemanticinfobackend.moc"

// Qt
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

// KDE
#include <kdebug.h>
#include <kurl.h>

// Nepomuk
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>

#include <Soprano/Vocabulary/Xesam>

// Local

namespace Gwenview {

struct Task {
	Task(const KUrl& url): mUrl(url) {}
	virtual ~Task() {}
	virtual void execute() = 0;

	KUrl mUrl;
};


struct RetrieveTask : public Task {
	RetrieveTask(NepomukSemanticInfoBackEnd* backEnd, const KUrl& url)
	: Task(url), mBackEnd(backEnd) {}

	virtual void execute() {
		QString urlString = mUrl.url();
		Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());

		SemanticInfo semanticInfo;
		semanticInfo.mRating = resource.rating();
		semanticInfo.mDescription = resource.description();
		Q_FOREACH(const Nepomuk::Tag& tag, resource.tags()) {
			semanticInfo.mTags << tag.resourceUri().toString();
		}
		mBackEnd->emitSemanticInfoRetrieved(mUrl, semanticInfo);
	}

	NepomukSemanticInfoBackEnd* mBackEnd;
};


struct StoreTask : public Task {
	StoreTask(const KUrl& url, const SemanticInfo& semanticInfo)
	: Task(url), mSemanticInfo(semanticInfo) {}

	virtual void execute() {
		QString urlString = mUrl.url();
		Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
		resource.setRating(mSemanticInfo.mRating);
		resource.setDescription(mSemanticInfo.mDescription);
		QList<Nepomuk::Tag> tags;
		Q_FOREACH(const SemanticInfoTag& uri, mSemanticInfo.mTags) {
			tags << Nepomuk::Tag(uri);
		}
		resource.setTags(tags);
	}

	SemanticInfo mSemanticInfo;
};


typedef QQueue<Task*> TaskQueue;

class SemanticInfoThread : public QThread {
public:
	SemanticInfoThread()
	: mDeleting(false)
	{}

	~SemanticInfoThread() {
		{
			QMutexLocker locker(&mMutex);
			mDeleting = true;
		}
		// Notify the thread so that it doesn't stay blocked waiting for mQueueNotEmpty
		mQueueNotEmpty.wakeAll();

		wait();
		qDeleteAll(mTaskQueue);
	}


	void enqueueTask(Task* task) {
		{
			QMutexLocker locker(&mMutex);
			mTaskQueue.enqueue(task);
		}
		mQueueNotEmpty.wakeAll();
	}


	virtual void run() {
		while (true) {
			Task* task;
			{
				QMutexLocker locker(&mMutex);
				if (mDeleting) {
					return;
				}
				if (mTaskQueue.isEmpty()) {
					mQueueNotEmpty.wait(&mMutex);
				}
				if (mDeleting) {
					return;
				}
				task = mTaskQueue.dequeue();
			}
			task->execute();
		}
	}


private:
	TaskQueue mTaskQueue;
	QMutex mMutex;
	QWaitCondition mQueueNotEmpty;
	bool mDeleting;
};


struct NepomukSemanticInfoBackEndPrivate {
	SemanticInfoThread mThread;
};


NepomukSemanticInfoBackEnd::NepomukSemanticInfoBackEnd(QObject* parent)
: AbstractSemanticInfoBackEnd(parent)
, d(new NepomukSemanticInfoBackEndPrivate) {
	d->mThread.start();
}


NepomukSemanticInfoBackEnd::~NepomukSemanticInfoBackEnd() {
	delete d;
}


TagSet NepomukSemanticInfoBackEnd::allTags() const {
	QList<Nepomuk::Tag> list = Nepomuk::Tag::allTags();

	TagSet set;
	Q_FOREACH(const Nepomuk::Tag& tag, list) {
		set << tag.resourceUri().toString();
	}

	return set;
}


void NepomukSemanticInfoBackEnd::storeSemanticInfo(const KUrl& url, const SemanticInfo& semanticInfo) {
	StoreTask* task = new StoreTask(url, semanticInfo);
	d->mThread.enqueueTask(task);
}


void NepomukSemanticInfoBackEnd::retrieveSemanticInfo(const KUrl& url) {
	RetrieveTask* task = new RetrieveTask(this, url);
	d->mThread.enqueueTask(task);
}


void NepomukSemanticInfoBackEnd::emitSemanticInfoRetrieved(const KUrl& url, const SemanticInfo& semanticInfo) {
	emit semanticInfoRetrieved(url, semanticInfo);
}


QString NepomukSemanticInfoBackEnd::labelForTag(const SemanticInfoTag& uri) const {
	Nepomuk::Tag tag(uri);
	if (!tag.exists()) {
		kError() << "No tag for uri" << uri << ". This should not happen!";
		return QString();
	}
	return tag.label();
}


SemanticInfoTag NepomukSemanticInfoBackEnd::tagForLabel(const QString& label) const {
	Nepomuk::Tag tag(label);
	if (!tag.exists()) {
		// Not found, create the tag
		tag.setLabel(label);
	}
	return tag.resourceUri().toString();
}


} // namespace
