/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef KWEBKITPART_P_H
#define KWEBKITPART_P_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace KDEPrivate {
  class SearchBar;
}

namespace KParts {
  class StatusBarExtension;
}

class KUrl;
class WebView;
class WebPage;
class KUrlLabel;
class KWebKitPart;
class WebKitBrowserExtension;

class QUrl;
class QRect;
class QWidget;
class QAction;
class QWebFrame;
class QTextCodec;
class QWebHistoryItem;

class KWebKitPartPrivate : public QObject
{
   Q_OBJECT
public:
    KWebKitPartPrivate(KWebKitPart *parent);
    void init (QWidget *widget, const QString& sessionFileName);
    void initActions();

    bool emitOpenUrlNotify;
    bool contentModified;
    bool pageRestored;
    QPointer<WebView> webView;
    QPointer<WebPage> webPage;
    QPointer<KDEPrivate::SearchBar> searchBar;
    WebKitBrowserExtension *browserExtension;
    KParts::StatusBarExtension *statusBarExtension;


private Q_SLOTS:
    void slotShowSecurity();
    void slotShowSearchBar();
    void slotLoadStarted();
    void slotContentsChanged();
    void slotLoadFinished(bool);
    void slotLoadAborted(const KUrl &);

    void slotSearchForText(const QString &text, bool backward);
    void slotLinkHovered(const QString &, const QString&, const QString &);
    void slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item);
    void slotRestoreFrameState(QWebFrame *frame);
    void slotLinkMiddleOrCtrlClicked(const KUrl&);
    void slotSelectionClipboardUrlPasted(const KUrl &);
    void slotSelectionClipboardUrlPasted(const KUrl&, const QString&);

    void slotUrlChanged(const QUrl &);
    void slotWalletClosed();
    void slotShowWalletMenu();
    void slotLaunchWalletManager();
    void slotDeleteNonPasswordStorableSite();
    void slotRemoveCachedPasswords();
    void slotSetTextEncoding(QTextCodec*);
    void slotSetStatusBarText(const QString& text);
    void slotWindowCloseRequested();
    void slotGeometryChangeRequested(const QRect &);

private:
    KWebKitPart *q;
    KUrlLabel *statusBarWalletLabel;
    bool hasCachedFormData;
};

#endif // KWEBKITPART_P_H
