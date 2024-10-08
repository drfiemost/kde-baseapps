/***************************************************************************
 * Copyright (C) 2014 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef DOLPHIN_RECENT_TABS_MENU_H
#define DOLPHIN_RECENT_TABS_MENU_H

#include <KActionMenu>
#include <KUrl>

class DolphinTabPage;
class QAction;

class DolphinRecentTabsMenu : public KActionMenu
{
    Q_OBJECT

public:
    explicit DolphinRecentTabsMenu(QObject* parent);

public slots:
    void rememberClosedTab(const KUrl& url, const QByteArray& state);
    void undoCloseTab();

signals:
    void restoreClosedTab(const QByteArray& state);
    void closedTabsCountChanged(unsigned int count);

private slots:
    void handleAction(QAction* action);

private:
    QAction* m_clearListAction;
};

#endif