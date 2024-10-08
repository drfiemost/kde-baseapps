/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt3Support module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef Q3TL_H
#define Q3TL_H

#include <algorithm>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

template <typename T, typename LessThan>
Q_OUTOFLINE_TEMPLATE void qHeapSortPushDown(T *heap, int first, int last, LessThan lessThan)
{
    int r = first;
    while (r <= last / 2) {
        if (last == 2 * r) {
            // node r has only one child
            if (lessThan(heap[2 * r], heap[r]))
                std::swap(heap[r], heap[2 * r]);
            r = last;
        } else {
            // node r has two children
            if (lessThan(heap[2 * r], heap[r]) && !lessThan(heap[2 * r + 1], heap[2 * r])) {
                // swap with left child
                std::swap(heap[r], heap[2 * r]);
                r *= 2;
            } else if (lessThan(heap[2 * r + 1], heap[r])
                       && lessThan(heap[2 * r + 1], heap[2 * r])) {
                // swap with right child
                std::swap(heap[r], heap[2 * r + 1]);
                r = 2 * r + 1;
            } else {
                r = last;
            }
        }
    }
}

template <typename BiIterator, typename T, typename LessThan>
Q_OUTOFLINE_TEMPLATE void qHeapSortHelper(BiIterator begin, BiIterator end, const T & /* dummy */, LessThan lessThan)
{
    BiIterator it = begin;
    uint n = 0;
    while (it != end) {
        ++n;
        ++it;
    }
    if (n == 0)
        return;

    // Create the heap
    BiIterator insert = begin;
    T *realheap = new T[n];
    T *heap = realheap - 1;
    int size = 0;
    for(; insert != end; ++insert) {
        heap[++size] = *insert;
        int i = size;
        while (i > 1 && lessThan(heap[i], heap[i / 2])) {
            std::swap(heap[i], heap[i / 2]);
            i /= 2;
        }
    }

    // Now do the sorting
    for (int i = n; i > 0; i--) {
        *begin++ = heap[1];
        if (i > 1) {
            heap[1] = heap[i];
            qHeapSortPushDown(heap, 1, i - 1, lessThan);
        }
    }

    delete[] realheap;
}

template <typename BiIterator, typename T>
inline void qHeapSortHelper(BiIterator begin, BiIterator end, const T &dummy)
{
    qHeapSortHelper(begin, end, dummy, std::less<T>());
}

template <typename BiIterator, typename LessThan>
inline void qHeapSort(BiIterator begin, BiIterator end, LessThan lessThan)
{
    if (begin != end)
        qHeapSortHelper(begin, end, *begin, lessThan);
}

template <typename BiIterator>
inline void qHeapSort(BiIterator begin, BiIterator end)
{
    if (begin != end)
        qHeapSortHelper(begin, end, *begin);
}

template <typename Container>
inline void qHeapSort(Container &c)
{
#ifdef Q_CC_BOR
    // Work around Borland 5.5 optimizer bug
    c.detach();
#endif
    if (!c.empty())
        qHeapSortHelper(c.begin(), c.end(), *c.begin());
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q3TL_H
