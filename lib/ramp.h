// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef RAMP_H
#define RAMP_H

namespace Gwenview
{
/**
 * This class maps values on a linear ramp.
 * It's useful to do mappings like this:
 *
 * x | -oo       x1               x2     +oo
 * --+--------------------------------------
 * y |  y1       y1 (linear ramp) y2      y2
 *
 * Note that y1 can be greater than y2 if necessary
 */
class Ramp
{
public:
    Ramp(qreal x1, qreal x2, qreal y1, qreal y2)
        : mX1(x1)
        , mX2(x2)
        , mY1(y1)
        , mY2(y2)
    {
        mK = (y2 - y1) / (x2 - x1);
    }

    qreal operator()(qreal x) const
    {
        if (x < mX1) {
            return mY1;
        }
        if (x > mX2) {
            return mY2;
        }
        return mY1 + (x - mX1) * mK;
    }

private:
    qreal mX1, mX2, mY1, mY2, mK;
};

} // namespace

#endif /* RAMP_H */
