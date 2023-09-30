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

#ifndef Q3DRAGOBJECT_H
#define Q3DRAGOBJECT_H

#include <QtCore/qobject.h>
#include <QtGui/qcolor.h>
#include <QtGui/qmime.h>
#include <QtGui/qimage.h>
#include <QtCore/qlist.h>
#include <QByteArray>
#include <QtGui/qpixmap.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QWidget;
class Q3ImageDrag;
class Q3TextDrag;
class QImage;
class QPixmap;

class Q3DragObject : public QObject, public QMimeSource {
    Q_OBJECT
public:
    Q3DragObject(QWidget * dragSource = nullptr, const char *name = nullptr);
    virtual ~Q3DragObject();

    bool drag();
    bool dragMove();
    void dragCopy();
    void dragLink();

    virtual void setPixmap(QPixmap);
    virtual void setPixmap(QPixmap, const QPoint& hotspot);
    QPixmap getPixmap() const;
    QPoint pixmapHotSpot() const;

    QWidget * source();
    static QWidget * target();

    enum DragMode { DragDefault, DragCopy, DragMove, DragLink, DragCopyOrMove };

protected:
    virtual bool drag(DragMode);

    QPixmap pixmap;
    QPoint hot;
    // store default cursors
    QPixmap *pm_cursor;

private:
    friend class QDragMime;
    Q_DISABLE_COPY(Q3DragObject)
};

class Q3StoredDrag: public Q3DragObject {
    Q_OBJECT
public:
    Q3StoredDrag(const char *mimeType, QWidget *dragSource = nullptr, const char *name = nullptr);
    ~Q3StoredDrag();

    virtual void setEncodedData(const QByteArray &);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

protected:
    const char* fmt;
    QByteArray enc;

private:
    Q_DISABLE_COPY(Q3StoredDrag)
};

class Q3TextDrag: public Q3DragObject {
    Q_OBJECT
public:
    Q3TextDrag(const QString &, QWidget *dragSource = nullptr, const char *name = nullptr);
    Q3TextDrag(QWidget * dragSource = nullptr, const char * name = nullptr);
    ~Q3TextDrag();

    void setText(const QString &);
    void setSubtype(const QString &);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QString& s);
    static bool decode(const QMimeSource* e, QString& s, QString& subtype);

protected:
    QString txt;
    QString subtype;
    QByteArray fmt;

private:
    Q_DISABLE_COPY(Q3TextDrag)
};

class Q3ImageDrag: public Q3DragObject {
    Q_OBJECT
public:
    Q3ImageDrag(QImage image, QWidget * dragSource = nullptr, const char * name = nullptr);
    Q3ImageDrag(QWidget * dragSource = nullptr, const char * name = nullptr);
    ~Q3ImageDrag();

    virtual void setImage(QImage image);

    const char * format(int i) const;
    virtual QByteArray encodedData(const char*) const;

    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QImage& i);
    static bool decode(const QMimeSource* e, QPixmap& i);

protected:
    QImage img;
    QList<QByteArray> ofmts;

private:
    Q_DISABLE_COPY(Q3ImageDrag)
};


class Q3UriDrag: public Q3StoredDrag {
    Q_OBJECT

public:
    Q3UriDrag(const QList<QByteArray> &uris, QWidget * dragSource = nullptr, const char * name = nullptr);
    Q3UriDrag(QWidget * dragSource = nullptr, const char * name = nullptr);
    ~Q3UriDrag();

    void setFileNames(const QStringList & fnames);
    inline void setFileNames(const QString & fname) { setFileNames(QStringList(fname)); }
    void setFilenames(const QStringList & fnames) { setFileNames(fnames); }
    inline void setFilenames(const QString & fname) { setFileNames(QStringList(fname)); }
    void setUnicodeUris(const QStringList & uuris);
    virtual void setUris(const QList<QByteArray> &uris);

    static QString uriToLocalFile(const char*);
    static QByteArray localFileToUri(const QString&);
    static QString uriToUnicodeUri(const char*);
    static QByteArray unicodeUriToUri(const QString&);
    static bool canDecode(const QMimeSource* e);
    static bool decode(const QMimeSource* e, QList<QByteArray>& i);
    static bool decodeToUnicodeUris(const QMimeSource* e, QStringList& i);
    static bool decodeLocalFiles(const QMimeSource* e, QStringList& i);

private:
    Q_DISABLE_COPY(Q3UriDrag)
};

class Q3ColorDrag : public Q3StoredDrag
{
    Q_OBJECT
    QColor color;

public:
    Q3ColorDrag(const QColor &col, QWidget *dragsource = nullptr, const char *name = nullptr);
    Q3ColorDrag(QWidget * dragSource = nullptr, const char * name = nullptr);
    void setColor(const QColor &col);

    static bool canDecode(QMimeSource *);
    static bool decode(QMimeSource *, QColor &col);

private:
    Q_DISABLE_COPY(Q3ColorDrag)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q3DRAGOBJECT_H
