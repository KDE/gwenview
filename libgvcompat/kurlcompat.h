#ifndef KURLCOMPAT_H
#define KURLCOMPAT_H   

#include <kurl.h>

bool operator<(const KURL& u1, const KURL& u2)
{
  int i;
  if (!u2.isValid())
  {
     if (!u1.isValid())
     {
        i = u1.protocol().compare(u2.protocol());
        return (i < 0);
     }
     return false;
  }
  if (!u1.isValid())
     return true;

  i = u1.protocol().compare(u2.protocol());
  if (i) return (i < 0);

  i = u1.host().compare(u2.host());
  if (i) return (i < 0);

  if (u1.port() != u2.port()) return (u1.port() < u2.port());

  i = u1.path().compare(u2.path());
  if (i) return (i < 0);

  i = u1.query().compare(u2.query());
  if (i) return (i < 0);

  i = u1.ref().compare(u2.ref());
  if (i) return (i < 0);

  i = u1.user().compare(u2.user());
  if (i) return (i < 0);

  i = u1.pass().compare(u2.pass());
  if (i) return (i < 0);

  return false;
}

#endif /* KURLCOMPAT_H */
