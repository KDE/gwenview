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
 * @internal
 *
 * Necessary helper class because a QObject class cannot be a template
 */
class GWENVIEWLIB_EXPORT BinderInternal : public QObject {
	Q_OBJECT
public:
	explicit BinderInternal(QObject* parent);
	~BinderInternal();

protected Q_SLOTS:
	virtual void callMethod() {}
};

/**
 * Makes it possible to "connect" a parameter-less signal with a slot
 * which accepts one argument.  The argument must be known at connection time.
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
 * Binder<Receiver, Param>::bind(emitter, SIGNAL(somethingHappened()), receiver, &Receiver::doSomething, p)
 *
 * Will result in receiver->doSomething(p) being called when emitter emits the
 * somethingHappened() signal.
 *
 * Just like a regular QObject connection, the connection will last until
 * either emitter or receiver are deleted.
 *
 * Using this system avoids the need of creating an helper slot and adding a
 * member to the Receiver class to store the argument of the method to call.
 *
 * Note: the method does not need to be a slot.
 */
template <class Receiver, class Arg>
class GWENVIEWLIB_EXPORT Binder : public BinderInternal
{
public:
	typedef void (Receiver::*Method)(const Arg&);
	static void bind(QObject* emitter, const char* signal, Receiver* receiver, Method method, Arg arg) {
		Binder<Receiver, Arg>* binder = new Binder<Receiver, Arg>(emitter);
		binder->mReceiver = receiver;
		binder->mMethod = method;
		binder->mArg = arg;
		QObject::connect(emitter, signal, binder, SLOT(callMethod()));
		QObject::connect(receiver, SIGNAL(destroyed(QObject*)), binder, SLOT(deleteLater()));
	}

protected:
	void callMethod() {
		(mReceiver->*mMethod)(mArg);
	}

private:
	Binder(QObject* emitter)
	: BinderInternal(emitter)
	{}

	Receiver* mReceiver;
	Method mMethod;
	Arg mArg;
};

} // namespace

#endif /* BINDER_H */
