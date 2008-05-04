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
#include "nepomukmetadatabackend.h"

// Qt
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

// KDE
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
	RetrieveTask(NepomukMetaDataBackEnd* backEnd, const KUrl& url)
	: Task(url), mBackEnd(backEnd) {}

	virtual void execute() {
		QString urlString = mUrl.url();
		Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());

		MetaData metaData;
		metaData.mRating = resource.rating();
		metaData.mDescription = resource.description();
		Q_FOREACH(const Nepomuk::Tag& tag, resource.tags()) {
			metaData.mTags << tag.resourceUri().toString();
		}
		mBackEnd->emitMetaDataRetrieved(mUrl, metaData);
	}

	NepomukMetaDataBackEnd* mBackEnd;
};


struct StoreTask : public Task {
	StoreTask(const KUrl& url, const MetaData& metaData)
	: Task(url), mMetaData(metaData) {}

	virtual void execute() {
		QString urlString = mUrl.url();
		Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
		resource.setRating(mMetaData.mRating);
		resource.setDescription(mMetaData.mDescription);
		QList<Nepomuk::Tag> tags;
		Q_FOREACH(const MetaDataTag& uri, mMetaData.mTags) {
			tags << Nepomuk::Tag(uri);
		}
		resource.setTags(tags);
	}

	MetaData mMetaData;
};


typedef QQueue<Task*> TaskQueue;

class MetaDataThread : public QThread {
public:
	MetaDataThread()
	: mDeleting(false)
	{}

	~MetaDataThread() {
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


struct NepomukMetaDataBackEndPrivate {
	MetaDataThread mThread;
};


NepomukMetaDataBackEnd::NepomukMetaDataBackEnd(QObject* parent)
: AbstractMetaDataBackEnd(parent)
, d(new NepomukMetaDataBackEndPrivate) {
	d->mThread.start();
}


NepomukMetaDataBackEnd::~NepomukMetaDataBackEnd() {
	delete d;
}


void NepomukMetaDataBackEnd::storeMetaData(const KUrl& url, const MetaData& metaData) {
	StoreTask* task = new StoreTask(url, metaData);
	d->mThread.enqueueTask(task);
}


void NepomukMetaDataBackEnd::retrieveMetaData(const KUrl& url) {
	RetrieveTask* task = new RetrieveTask(this, url);
	d->mThread.enqueueTask(task);
}


void NepomukMetaDataBackEnd::emitMetaDataRetrieved(const KUrl& url, const MetaData& metaData) {
	emit metaDataRetrieved(url, metaData);
}


QString NepomukMetaDataBackEnd::labelForTag(const MetaDataTag& uri) const {
	Nepomuk::Tag tag(uri);
	Q_ASSERT(tag.exists());
	return tag.label();
}


MetaDataTag NepomukMetaDataBackEnd::tagForLabel(const QString& label) const {
	Nepomuk::Tag tag(label);
	if (!tag.exists()) {
		// Not found, create the tag
		tag.setLabel(label);
		tag.addIdentifier(label);
	}
	return tag.resourceUri().toString();
}


} // namespace
