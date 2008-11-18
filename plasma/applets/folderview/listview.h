/*
 *   Copyright © 2008 Fredrik Höglund <fredrik@kde.org>
 *   Copyright © 2008 Rafael Fernández López <ereslibre@kde.org>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public License
 *   along with this library; see the file COPYING.LIB.  If not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QGraphicsWidget>
#include <QAbstractItemDelegate>
#include <QPointer>
#include <QCache>
#include <QTime>
#include <QBasicTimer>


class KUrl;
class KDirModel;
class KFileItemDelegate;
class KFilePreviewGenerator;
class KNewMenu;
class QItemSelectionModel;
class ProxyModel;
class QStyleOptionViewItemV4;
class QScrollBar;

namespace Plasma
{
    class ScrollBar;
}

class ListView : public QGraphicsWidget
{
    Q_OBJECT

public:
    ListView(QGraphicsWidget *parent = 0);
    ~ListView();

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void setSelectionModel(QItemSelectionModel *model);
    QItemSelectionModel *selectionModel() const;

    void setItemDelegate(KFileItemDelegate *delegate);
    KFileItemDelegate *itemDelegate() const;

    void setIconSize(const QSize &gridSize);
    QSize iconSize() const;

    void setWordWrap(bool on);
    bool wordWrap() const;

    void setDrawShadows(bool on);
    bool drawShadows() const;

    QRect visibleArea() const;
    QModelIndex indexAt(const QPointF &point) const;
    QRect visualRect(const QModelIndex &index) const;

signals:
    void activated(const QModelIndex &index);
    void contextMenuRequest(QWidget *widget, const QPoint &screenPos);

protected:
    void startDrag(const QPointF &pos, QWidget *widget);
    QRect scrollBackbufferContents();
    void updateTextShadows(const QColor &textColor);
    void markAreaDirty(const QRect &rect);
    void updateScrollBar();
    void updateSizeHint();

    QPointF mapToViewport(const QPointF &point) const;
    QRectF mapToViewport(const QRectF &rect) const;
    QPointF mapFromViewport(const QPointF &point) const;
    QRectF mapFromViewport(const QRectF &rect) const;
    QStyleOptionViewItemV4 viewOptions() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private slots:
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsRemoved(const QModelIndex &parent, int first, int last);
    void modelReset();
    void layoutChanged();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void scrollBarValueChanged(int value);

private:
    KFileItemDelegate *m_delegate;
    QPointer<KDirModel> m_dirModel;
    QPointer<ProxyModel> m_model;
    Plasma::ScrollBar *m_scrollBar;
    QPixmap m_pixmap;
    QPixmap m_topFadeTile;
    QPixmap m_bottomFadeTile;
    QRegion m_dirtyRegion;
    QPointer<QItemSelectionModel> m_selectionModel;
    QSize m_iconSize;
    int m_lastScrollValue;
    int m_rowHeight;
    QPersistentModelIndex m_hoveredIndex;
    QPersistentModelIndex m_pressedIndex;
    bool m_viewScrolled;
    bool m_dragInProgress;
    bool m_wordWrap;
    bool m_drawShadows;
};

#endif // LISTVIEW_H
