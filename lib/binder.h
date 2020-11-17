// vim: set tabstop=4 shiftwidth=4 expandtab:
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

namespace Gwenview
{

/**
 * @internal
 *
 * Necessary helper class because a QObject class cannot be a template
 */
class GWENVIEWLIB_EXPORT BinderInternal : public QObject
{
    Q_OBJECT
public:
    explicit BinderInternal(QObject* parent);
    ~BinderInternal() override;

protected Q_SLOTS:
    virtual void callMethod()
    {}
};

/**
 * The Binder and BinderRef classes make it possible to "connect" a
 * parameter-less signal with a slot which accepts one argument. The
 * argument must be known at connection time.
 *
 * Example:
 *
 * Assuming a class like this:
 *
 * class Receiver
 * {
 * public:
 *   void doSomething(Param* p);
 * };
 *
 * This code:
 *
 * Binder<Receiver, Param*>::bind(emitter, SIGNAL(somethingHappened()), receiver, &Receiver::doSomething, p)
 *
 * Will result in receiver->doSomething(p) being called when emitter emits
 * the somethingHappened() signal.
 *
 * Just like a regular QObject connection, the connection will last until
 * either emitter or receiver are deleted.
 *
 * Using this system avoids creating an helper slot and adding a member to
 * the Receiver class to store the argument of the method to call.
 *
 * To call a method which accept a pointer or a value argument, use Binder.
 * To call a method which accept a const reference argument, use BinderRef.
 *
 * Note: the method does not need to be a slot.
 */
template <class Receiver, class Arg, typename MethodArg>
class BaseBinder : public BinderInternal
{
public:
    typedef void (Receiver::*Method)(MethodArg);
    static void bind(QObject* emitter, const char* signal, Receiver* receiver, Method method, MethodArg arg)
    {
        BaseBinder<Receiver, Arg, MethodArg>* binder = new BaseBinder<Receiver, Arg, MethodArg>(emitter);
        binder->mReceiver = receiver;
        binder->mMethod = method;
        binder->mArg = arg;
        QObject::connect(emitter, signal, binder, SLOT(callMethod()));
        QObject::connect(receiver, SIGNAL(destroyed(QObject*)), binder, SLOT(deleteLater()));
    }

protected:
    void callMethod() override
    {
        (mReceiver->*mMethod)(mArg);
    }

private:
    BaseBinder(QObject* emitter)
    : BinderInternal(emitter)
    , mReceiver(nullptr)
    , mMethod(nullptr)
    {}

    Receiver* mReceiver;
    Method mMethod;
    Arg mArg;
};

template <class Receiver, class Arg>
class Binder : public BaseBinder<Receiver, Arg, Arg>
{};

template <class Receiver, class Arg>
class BinderRef : public BaseBinder<Receiver, Arg, const Arg&>
{};

} // namespace

#endif /* BINDER_H */
