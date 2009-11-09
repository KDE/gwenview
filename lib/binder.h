// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#ifndef BINDER_H
#define BINDER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

namespace Gwenview {

/**
 * Makes it possible to "connect" a parameter-less signal with a member
 * function which accepts one argument.  The argument must be known at
 * connection time.
 *
 * Example:
 *
 * Assuming a class like this:
 *
 * class Receiver {
 * public:
 *   void doSomething(const Param& p);
 * };
 *
 * This code:
 *
 * connect(emitter, SIGNAL(somethingHappened()),
 *   CREATE_BINDER(parent, Receiver, Param, receiver, doSomething, p),
 *   SLOT(run())
 *   );
 *
 * Will result in receiver->doSomething(p) being called when emitter emits the
 * somethingHappened() signal.
 *
 * parent must inherit from QObject: the connection will last until parent is
 * deleted.
 *
 * Using this system avoids the need of creating an helper slot and adding a
 * member to the Receiver class to store the argument of the method to call.
 */
#define CREATE_BINDER(parent, ReceiverClass, ArgClass, receiver, method, arg) \
	(new Binder<ReceiverClass, ArgClass>(parent, receiver, &ReceiverClass::method, arg))->qObject


class BinderQObject;


class GWENVIEWLIB_EXPORT AbstractBinder {
public:
	AbstractBinder(QObject* parent);
	virtual ~AbstractBinder() {}

	BinderQObject* qObject;

protected:
	virtual void run() = 0;

	friend class BinderQObject;
};


template <class Receiver, class Arg>
class Binder : public AbstractBinder {
public:
	typedef void (Receiver::*Method)(const Arg&);
	Binder(QObject* parent, Receiver* receiver, Method method, Arg arg)
	: AbstractBinder(parent)
	, mReceiver(receiver)
	, mMethod(method)
	, mArg(arg)
	{}

protected:
	virtual void run() {
		(mReceiver->*mMethod)(mArg);
	}

private:
	Receiver* mReceiver;
	Method mMethod;
	Arg mArg;
};


class GWENVIEWLIB_EXPORT BinderQObject : public QObject {
	Q_OBJECT
public:
	BinderQObject(QObject* parent, AbstractBinder* binder);
	~BinderQObject();

public Q_SLOTS:
	void run();

private:
	AbstractBinder* mBinder;
};

} // namespace

#endif /* BINDER_H */
