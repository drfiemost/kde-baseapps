/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//#include "konq_treepart.h"
#include "konq_sidebartreemodule.h"
#include <kdebug.h>
#include <kdirnotify.h>
#include <kio/paste.h>
#include <konq_operations.h>
#include <kprotocolinfo.h>
#include <kconfiggroup.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QtCore/QMimeData>
#include <konqmimedata.h>
#include <kdesktopfile.h>

void KonqSidebarTreeTopLevelItem::init()
{
    QString desktopFile = m_path;
    if ( isTopLevelGroup() )
        desktopFile += "/.directory";
    KDesktopFile cfg(  desktopFile );
    m_comment = cfg.desktopGroup().readEntry( "Comment" );
}

void KonqSidebarTreeTopLevelItem::setOpen( bool open )
{
    if (open && module())
        module()->openTopLevelItem( this );
    KonqSidebarTreeItem::setOpen( open );
}

void KonqSidebarTreeTopLevelItem::itemSelected()
{
    kDebug() << "KonqSidebarTreeTopLevelItem::itemSelected";
    const QMimeData *data = QApplication::clipboard()->mimeData();
    const bool paste = m_bTopLevelGroup && data->hasUrls();
    tree()->enableActions(true, true, paste);
}

bool KonqSidebarTreeTopLevelItem::acceptsDrops( const QStringList & formats )
{
    return formats.contains("text/uri-list") &&
        ( m_bTopLevelGroup || !externalURL().isEmpty() );
}

void decodePayload( const QByteArray& payload, KUrl::List &uris )
{
    int c=0;
    const char* d = payload.data();
    while (c < payload.size() && d[c]) {
        int f = c;
        // Find line end
        while (c < payload.size() && d[c] && d[c]!='\r'
                && d[c] != '\n')
            c++;
        QByteArray s(d+f, c-f+1);
        if ( s[0] != '#' ) // non-comment?
            uris.append(KUrl(s));
        // Skip junk
        while (c < payload.size() && d[c] &&
                (d[c]=='\n' || d[c]=='\r'))
            c++;
    }
}

bool dropEventDecode( const QDropEvent *e, KUrl::List &uris )
{
    // x-kde4-urilist is the same format as text/uri-list, but contains
    // KDE-aware urls, like media:/ and system:/, whereas text/uri-list is resolved to
    // local files.
    if ( e->mimeData()->hasFormat( "application/x-kde4-urilist" ) ) {
        QByteArray payload = e->mimeData()->data( "application/x-kde4-urilist" );
        if ( payload.size() ) {
            decodePayload(payload, uris);
            return !uris.isEmpty();
        }
    }

    QByteArray payload = e->mimeData()->data("text/uri-list");
    if (payload.size()) {
        decodePayload(payload, uris);
    }
    for (KUrl url: uris)
    {
      if ( !url.isValid() )
      {
        uris.clear();
        break;
      }
      uris.append( url );
    }
    return !uris.isEmpty();
}

void KonqSidebarTreeTopLevelItem::drop( QDropEvent * ev )
{
    if ( m_bTopLevelGroup )
    {
        // When dropping something to "Network" or its subdirs, we want to create
        // a desktop link, not to move/copy/link - except for .desktop files :-}
        KUrl::List lst;
        if ( dropEventDecode( ev, lst ) ) // Are they urls ?
        {
            KUrl::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
            {
                tree()->addUrl(this, *it);
            }
        } else
            kError() << "No URL !?  " << endl;
    }
    else // Top level item, not group
    {
        if ( !externalURL().isEmpty() )
            KonqOperations::doDrop( KFileItem(), externalURL(), ev, tree() );
    }
}

bool KonqSidebarTreeTopLevelItem::populateMimeData( QMimeData* mimeData, bool move )
{
    KUrl::List lst;
    lst.append( KUrl(path()) );

    KonqMimeData::populateMimeData( mimeData, KUrl::List(), lst, move );

#if 0
    const QPixmap * pix = pixmap(0);
    if (pix)
    {
        QPoint hotspot( pix->width() / 2, pix->height() / 2 );
        drag->setPixmap( *pix, hotspot );
    }
#endif

    return true;
}

void KonqSidebarTreeTopLevelItem::middleButtonClicked()
{
    if ( !m_bTopLevelGroup )
        emit tree()->createNewWindow( m_externalURL );
    // Do nothing for toplevel groups
}

void KonqSidebarTreeTopLevelItem::rightButtonPressed()
{
    KUrl url;
    url.setPath( m_path );
    // We don't show "edit file type" (useless here) and "properties" (shows the wrong name,
    // i.e. the filename instead of the Name field). There's the Rename item for that.
    // Only missing thing is changing the URL of a link. Hmm...

    if ( !module() || !module()->handleTopLevelContextMenu( this, QCursor::pos() ) )
    {
        tree()->showToplevelContextMenu();
    }
}


void KonqSidebarTreeTopLevelItem::trash()
{
    delOperation( KonqOperations::TRASH );
}

void KonqSidebarTreeTopLevelItem::del()
{
    delOperation( KonqOperations::DEL );
}

void KonqSidebarTreeTopLevelItem::delOperation( KonqOperations::Operation method )
{
    KUrl url( m_path );
    KUrl::List lst;
    lst.append(url);

    KonqOperations::del(tree(), method, lst);
}

void KonqSidebarTreeTopLevelItem::paste()
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) ) {
        move = KonqMimeData::decodeIsCutSelection( data );
        kDebug(1201) << "move (from clipboard data) = " << move;
    }

    KUrl destURL;
    if ( m_bTopLevelGroup )
        destURL.setPath( m_path );
    else
        destURL = m_externalURL;

    KIO::pasteClipboard( destURL, 0L,move );
}

void KonqSidebarTreeTopLevelItem::rename()
{
    tree()->rename( this, 0 );
}

void KonqSidebarTreeTopLevelItem::rename( const QString & name )
{
    KUrl url(m_path);

    // Adjust the Name field of the .directory or desktop file
    QString desktopFile = m_path;
    if ( isTopLevelGroup() )
        desktopFile += "/.directory";
    KDesktopFile cfg( desktopFile );
    cfg.desktopGroup().writeEntry( "Name", name );
    cfg.sync();

    // Notify about the change
    KUrl::List lst;
    lst.append(url);
    OrgKdeKDirNotifyInterface::emitFilesChanged( lst.toStringList() );
}

QString KonqSidebarTreeTopLevelItem::toolTipText() const
{
    return m_comment;
}

