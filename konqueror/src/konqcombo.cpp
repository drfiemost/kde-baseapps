/* This file is part of the KDE project
   Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2007 Fredrik Höglund <fredrik@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "konqcombo.h"

// Qt
#include <QPainter>
#include <QStyle>
#include <QDrag>
#include <QPixmap>
#include <QKeyEvent>
#include <QItemDelegate>
#include <QListWidgetItem>
#include <QtCore/QEvent>

// KDE
#include <kapplication.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kcompletionbox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <klineedit.h>
#include <konqpixmapprovider.h>
#include <kstandardshortcut.h>
#include <konqmainwindow.h>
#include <kstringhandler.h>

// Local
#include "konqview.h"
#include "KonquerorAdaptor.h"
#include "konqueror_interface.h"
#include "konqhistorymanager.h"

KConfig * KonqCombo::s_config = 0L;
const int KonqCombo::temporary = 0;

static QString titleOfURL( const QString& urlStr )
{
    KUrl url( urlStr );
    KonqHistoryList historylist = KonqHistoryManager::kself()->entries();
    KonqHistoryList::iterator historyentry = historylist.findEntry( url );
    if ( historyentry == historylist.end() && !url.url().endsWith('/') ) {
        url.adjustPath(KUrl::AddTrailingSlash);
        historyentry = historylist.findEntry( url );
    }
    return ( historyentry != historylist.end() ? (*historyentry).title : QString() );
}

///////////////////////////////////////////////////////////////////////////////

class KonqListWidgetItem : public QListWidgetItem
{
public:
    enum ItemType { KonqItemType = 0x1845D5CC };

    KonqListWidgetItem( QListWidget *parent = 0 );
    KonqListWidgetItem( const QString &text, QListWidget *parent = 0 );

    QVariant data( int role ) const;

    bool reuse( const QString &newText );

private:
    mutable bool lookupPending;
};

///////////////////////////////////////////////////////////////////////////////

class KonqComboItemDelegate : public QItemDelegate
{
public:
    KonqComboItemDelegate( QObject *parent ) : QItemDelegate( parent ) {}
    QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
};

///////////////////////////////////////////////////////////////////////////////

class KonqComboLineEdit : public KLineEdit
{
public:
    KonqComboLineEdit( QWidget *parent=0 );
    void setCompletedItems( const QStringList& items, bool );

protected:
    void mouseDoubleClickEvent( QMouseEvent *e );
};

class KonqComboCompletionBox : public KCompletionBox
{
public:
    KonqComboCompletionBox( QWidget *parent );
    void setItems( const QStringList& items );
    void insertStringList( const QStringList & list, int index = -1 );
};

KonqCombo::KonqCombo( QWidget *parent )
          : KHistoryComboBox( parent ),
            m_returnPressed( false ),
            m_permanent( false ),
	    m_pageSecurity( KonqMainWindow::NotCrypted )
{
    setLayoutDirection(Qt::LeftToRight);
    setInsertPolicy( NoInsert );
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLength );

    Q_ASSERT( s_config );

    KConfigGroup locationBarGroup( s_config, "Location Bar" );
    setMaxCount( locationBarGroup.readEntry("Maximum of URLs in combo", 20 ));

    // We should also connect the completionBox' highlighted signal to
    // our setEditText() slot, because we're handling the signals ourselves.
    // But we're lazy and let KCompletionBox do this and simply switch off
    // handling of signals later.
    setHandleSignals( true );

    KonqComboLineEdit *edit = new KonqComboLineEdit( this );
    edit->setHandleSignals( true );
    edit->setCompletionBox( new KonqComboCompletionBox( edit ) );
    setLineEdit( edit );
    setItemDelegate( new KonqComboItemDelegate( this ) );

    connect( edit, SIGNAL(textEdited(QString)),
             this, SLOT(slotTextEdited(QString)) );

    completionBox()->setTabHandling(true); // #167135
    completionBox()->setItemDelegate( new KonqComboItemDelegate( this ) );

    // Make the lineedit consume the Qt::Key_Enter event...
    setTrapReturnKey( true );

    // Connect to the returnPressed signal when completionMode == CompletionNone. #314736
    slotCompletionModeChanged(completionMode());

    connect( KonqHistoryManager::kself(), SIGNAL(cleared()), SLOT(slotCleared()) );
    connect( this, SIGNAL(cleared()), SLOT(slotCleared()) );
    connect( this, SIGNAL(highlighted(int)), SLOT(slotSetIcon(int)) );
    connect( this, SIGNAL(activated(QString)), SLOT(slotActivated(QString)) );
    connect( this, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
             this, SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
}

KonqCombo::~KonqCombo()
{
}

void KonqCombo::init( KCompletion *completion )
{
    setCompletionObject( completion, false ); //KonqMainWindow handles signals
    setAutoDeleteCompletionObject( false );
    setCompletionMode( completion->completionMode() );

    // We use Ctrl+T for new tab, so we need something else for substring completion
    // TODO: how to make that shortcut configurable? If we add a KAction we need to
    // call the KLineEdit code, which we can't do. Well, we could send a keyevent...
    setKeyBinding(KCompletionBase::SubstringCompletion, KShortcut(Qt::Key_F7));

    loadItems();
}

void KonqCombo::slotTextEdited( const QString &text )
{
  QString txt = text;
  txt.remove( QChar('\n') );
  txt.remove( QChar(QChar::LineSeparator) );
  txt.remove( QChar(QChar::ParagraphSeparator) );

  if ( txt != text ) {
      lineEdit()->setText( txt );
  }
}

void KonqCombo::setURL( const QString& url )
{
    //kDebug() << url << "returnPressed=" << m_returnPressed;
    setTemporary( url );

    if ( m_returnPressed ) { // Really insert...
        m_returnPressed = false;
        QDBusMessage message = QDBusMessage::createSignal( KONQ_MAIN_PATH, "org.kde.Konqueror.Main", "addToCombo" );
        message << url;
        QDBusConnection::sessionBus().send( message );
    }
    // important security consideration: always display the beginning
    // of the url rather than its end to prevent spoofing attempts.
    lineEdit()->setCursorPosition( 0 );
}

void KonqCombo::setTemporary( const QString& text )
{
    setTemporary( text, KonqPixmapProvider::self()->pixmapFor(text) );
}

void KonqCombo::setTemporary( const QString& url, const QPixmap& pix )
{
    //kDebug() << url << "temporary=" << temporary;

    // Insert a temporary item when we don't have one yet
    if ( count() == 0 )
      insertItem( pix, url, temporary, titleOfURL( url ) );
    else
    {
        if (url != temporaryItem())
          applyPermanent();

        updateItem( pix, url, temporary, titleOfURL( url ) );
    }

    setCurrentIndex( temporary );
}

void KonqCombo::removeDuplicates( int index )
{
    //kDebug() << "starting index= " << index;

    QString url(temporaryItem());
    if (url.endsWith('/'))
      url.truncate(url.length()-1);

    // Remove all dupes, if available...
    for ( int i = index; i < count(); i++ )
    {
        QString item(itemText(i));
        if (item.endsWith('/'))
          item.truncate(item.length()-1);

        if ( item == url )
            removeItem( i );
    }
}

// called via DBUS in all instances
void KonqCombo::insertPermanent( const QString& url )
{
    //kDebug() << "url=" << url;
    saveState();
    setTemporary( url );
    m_permanent = true;
    restoreState();
}

// called right before a new (different!) temporary item will be set. So we
// insert an item that was marked permanent properly at position 1.
void KonqCombo::applyPermanent()
{
    if ( m_permanent && !temporaryItem().isEmpty() ) {

        // Remove as many items as needed to honor maxCount()
        int index = count();
        while ( count() >= maxCount() )
            removeItem( --index );

        QString item = temporaryItem();
        insertItem( KonqPixmapProvider::self()->pixmapFor( item ), item, 1, titleOfURL( item ) );
        //kDebug() << url;

        // Remove all duplicates starting from index = 2
        removeDuplicates( 2 );
        m_permanent = false;
    }
}

void KonqCombo::insertItem( const QString &text, int index, const QString& title )
{
    KHistoryComboBox::insertItem( index, text, title );
}

void KonqCombo::insertItem( const QPixmap &pixmap, const QString& text, int index, const QString& title )
{
    KHistoryComboBox::insertItem( index, pixmap, text, title );
}

void KonqCombo::updateItem( const QPixmap& pix, const QString& t, int index, const QString& title )
{
    // No need to flicker
    if (itemText( index ) == t &&
        (!itemIcon(index).isNull() && itemIcon(index).pixmap(iconSize()).serialNumber() == pix.serialNumber()))
        return;

    // kDebug() << "item=" << t << "index=" << index;

    setItemText( index, t );
    setItemIcon( index, pix );
    setItemData( index, title );

    update();
}

void KonqCombo::saveState()
{
    m_cursorPos = cursorPosition();
    m_currentText = currentText();
    m_selectedText = lineEdit()->selectedText();
    m_currentIndex = currentIndex();
}

void KonqCombo::restoreState()
{
    setTemporary( m_currentText );
    if (m_selectedText.isEmpty()) {
        lineEdit()->setCursorPosition( m_cursorPos );
    } else {
        const int index = m_currentText.indexOf(m_selectedText);
        if (index == -1) {
            lineEdit()->setCursorPosition( m_cursorPos );
        } else {
            lineEdit()->setSelection(index, m_selectedText.length());
        }
    }
}

void KonqCombo::updatePixmaps()
{
    saveState();

    setUpdatesEnabled( false );
    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    for ( int i = 1; i < count(); i++ ) {
        setItemIcon( i, prov->pixmapFor( itemText( i ) ) );
    }
    setUpdatesEnabled( true );
    repaint();

    restoreState();
}

void KonqCombo::loadItems()
{
    clear();
    int i = 0;

    KConfigGroup historyConfigGroup( s_config, "History" ); // delete the old 2.0.x completion
    historyConfigGroup.writeEntry( "CompletionItems", "unused" );

    KConfigGroup locationBarGroup( s_config, "Location Bar" );
    const QStringList items = locationBarGroup.readPathEntry( "ComboContents", QStringList() );
    QStringList::ConstIterator it = items.begin();
    QString item;
    while ( it != items.end() ) {
        item = *it;
        if ( !item.isEmpty() ) { // only insert non-empty items
            insertItem( KonqPixmapProvider::self()->pixmapFor( item, KIconLoader::SizeSmall ),
                        item, i++, titleOfURL( item ) );
        }
        ++it;
    }

    if ( count() > 0 )
        m_permanent = true; // we want the first loaded item to stay
}

void KonqCombo::slotSetIcon( int index )
{
    if( itemIcon( index ).isNull())
        // on-demand icon loading
        setItemIcon( index, KonqPixmapProvider::self()->pixmapFor( itemText( index ),
                     KIconLoader::SizeSmall ) );
    update();
}

void KonqCombo::getStyleOption(QStyleOptionComboBox* comboOpt)
{
   //We only use this for querying metrics,so it's rough..
   comboOpt->init(this);
   comboOpt->editable = isEditable();
   comboOpt->frame    = hasFrame();
   comboOpt->iconSize = iconSize();
   comboOpt->currentIcon = itemIcon(currentIndex());
   comboOpt->currentText = currentText();
}

void KonqCombo::popup()
{
  for( int i = 0; i < count(); ++i )
    {
        if( itemIcon( i ).isNull() )
        {
            // on-demand icon loading
            setItemIcon( i, KonqPixmapProvider::self()->pixmapFor( itemText( i ),
                         KIconLoader::SizeSmall ) );
        }
    }
    KHistoryComboBox::showPopup();
}

void KonqCombo::saveItems()
{
    QStringList items;
    int i = m_permanent ? 0 : 1;

    for ( ; i < count(); i++ )
        items.append( itemText( i ) );

    KConfigGroup locationBarGroup( s_config, "Location Bar" );
    locationBarGroup.writePathEntry( "ComboContents", items );
    KonqPixmapProvider::self()->save( locationBarGroup, "ComboIconCache", items );

    s_config->sync();
}

void KonqCombo::clearTemporary( bool makeCurrent )
{
    applyPermanent();
    setItemText( temporary, QString() ); // ### default pixmap?
    if ( makeCurrent )
      setCurrentIndex( temporary );
}

bool KonqCombo::eventFilter( QObject *o, QEvent *ev )
{
    // Handle Ctrl+Del/Backspace etc better than the Qt widget, which always
    // jumps to the next whitespace.
    QLineEdit *edit = lineEdit();
    if ( o == edit ) {
        const int type = ev->type();
        if (type == QEvent::KeyPress) {
            QKeyEvent *e = static_cast<QKeyEvent *>( ev );

            KShortcut key( e->key() | e->modifiers() );

            if ( key == KStandardShortcut::deleteWordBack() ||
                 key == KStandardShortcut::deleteWordForward() ||
                 ((e->modifiers() & Qt::ControlModifier) &&
                   (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) ) ) {
                selectWord(e);
                e->accept();
                return true;
            }
        }

        else if ( type == QEvent::MouseButtonDblClick ) {
            edit->selectAll();
            return true;
        }
    }
    return KComboBox::eventFilter( o, ev );
}

void KonqCombo::keyPressEvent( QKeyEvent *e )
{
    KHistoryComboBox::keyPressEvent( e );
    // we have to set it as temporary, otherwise we wouldn't get our nice
    // pixmap. Yes, QComboBox still sucks.
    KShortcut key( e->key() | e->modifiers() );
    if ( key == KStandardShortcut::rotateUp() ||
         key == KStandardShortcut::rotateDown() )
         setTemporary( currentText() );
}

/*
   Handle Ctrl+Cursor etc better than the Qt widget, which always
   jumps to the next whitespace. This code additionally jumps to
   the next [/#?:], which makes more sense for URLs. The list of
   chars that will stop the cursor are '/', '.', '?', '#', ':'.
*/
void KonqCombo::selectWord(QKeyEvent *e)
{
    QLineEdit* edit = lineEdit();
    QString text = edit->text();
    int pos = edit->cursorPosition();
    int pos_old = pos;
    int count = 0;

    // TODO: make these a parameter when in kdelibs/kdeui...
    QList<QChar> chars;
    chars << QChar('/') << QChar('.') << QChar('?') << QChar('#') << QChar(':');
    bool allow_space_break = true;

    if( e->key() == Qt::Key_Left || e->key() == Qt::Key_Backspace ) {
        do {
            pos--;
            count++;
            if( allow_space_break && text[pos].isSpace() && count > 1 )
                break;
        } while( pos >= 0 && (chars.indexOf(text[pos]) == -1 || count <= 1) );

        if( e->modifiers() & Qt::ShiftModifier ) {
                  edit->cursorForward(true, 1-count);
        }
        else if(  e->key() == Qt::Key_Backspace ) {
            edit->cursorForward(false, 1-count);
            QString text = edit->text();
            int pos_to_right = edit->text().length() - pos_old;
            QString cut = text.left(edit->cursorPosition()) + text.right(pos_to_right);
            edit->setText(cut);
            edit->setCursorPosition(pos_old-count+1);
        }
        else {
            edit->cursorForward(false, 1-count);
        }
     }
     else if( e->key() == Qt::Key_Right || e->key() == Qt::Key_Delete ){
        do {
            pos++;
            count++;
                  if( allow_space_break && text[pos].isSpace() )
                      break;
        } while( pos < text.length() && chars.indexOf(text[pos]) == -1 );

        if( e->modifiers() & Qt::ShiftModifier ) {
            edit->cursorForward(true, count+1);
        }
        else if(  e->key() == Qt::Key_Delete ) {
            edit->cursorForward(false, -count-1);
            QString text = edit->text();
            int pos_to_right = text.length() - pos - 1;
            QString cut = text.left(pos_old) +
               (pos_to_right > 0 ? text.right(pos_to_right) : QString() );
            edit->setText(cut);
            edit->setCursorPosition(pos_old);
        }
        else {
            edit->cursorForward(false, count+1);
        }
    }
}

void KonqCombo::slotCleared()
{
    QDBusMessage message = QDBusMessage::createSignal( KONQ_MAIN_PATH, "org.kde.Konqueror.Main", "comboCleared" );
    QDBusConnection::sessionBus().send( message );
}

void KonqCombo::removeURL( const QString& url )
{
    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeFromHistory( url );
    applyPermanent();
    setTemporary( currentText() );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
}

void KonqCombo::mousePressEvent( QMouseEvent *e )
{
    m_dragStart = QPoint(); // null QPoint

    if ( e->button() == Qt::LeftButton && !itemIcon( currentIndex()).isNull() ) {
        // check if the pixmap was clicked
        int x = e->pos().x();
        QStyleOptionComboBox comboOpt;
        getStyleOption(&comboOpt);
        int x0 = QStyle::visualRect( layoutDirection(), rect(),
            style()->subControlRect( QStyle::CC_ComboBox, &comboOpt, QStyle::SC_ComboBoxEditField,
                                     this ) ).x();

        if ( x > x0 + 2 && x < lineEdit()->x() ) {
            m_dragStart = e->pos();
            return; // don't call KComboBox::mousePressEvent!
        }
    }

    QStyleOptionComboBox optCombo;
    optCombo.initFrom(this);
    if ( e->button() == Qt::LeftButton && m_pageSecurity != KonqMainWindow::NotCrypted &&
         style()->subElementRect( QStyle::SE_ComboBoxFocusRect, &optCombo, this ).contains( e->pos() ) )
        emit showPageSecurity();

    KComboBox::mousePressEvent( e );
}

void KonqCombo::mouseMoveEvent( QMouseEvent *e )
{
    KComboBox::mouseMoveEvent( e );
    if ( m_dragStart.isNull() || currentText().isEmpty() )
        return;

    if ( e->buttons() & Qt::LeftButton &&
         (e->pos() - m_dragStart).manhattanLength() >
         KGlobalSettings::dndEventDelay() )
    {
        KUrl url( currentText() );
        if ( url.isValid() )
        {
            QDrag* drag = new QDrag(this);
            QMimeData* mime = new QMimeData();
            url.populateMimeData(mime);
            drag->setMimeData(mime);
            QPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText(),
                                                                 KIconLoader::SizeMedium );
            if ( !pix.isNull() )
                drag->setPixmap( pix );
            drag->start();
        }
    }
}

void KonqCombo::slotActivated( const QString& text )
{
    applyPermanent();
    m_returnPressed = true;
    emit activated( text, qApp->keyboardModifiers() );
}

void KonqCombo::setConfig( KConfig *kc )
{
    s_config = kc;
}

void KonqCombo::paintEvent( QPaintEvent *pe )
{
    QComboBox::paintEvent( pe );

    QLineEdit *edit = lineEdit();

    QStyleOptionComboBox comboOpt;
    getStyleOption(&comboOpt);
    QRect re = style()->subControlRect( QStyle::CC_ComboBox, &comboOpt,
                                        QStyle::SC_ComboBoxEditField, this );
    re = QStyle::visualRect(layoutDirection(), rect(), re);

    if (m_pageSecurity!= KonqMainWindow::NotCrypted) {
        QPainter p( this );
        p.setClipRect( re );

        QPixmap pix = SmallIcon(QLatin1String(m_pageSecurity == KonqMainWindow::Encrypted ? "security-high" : "security-medium"));

        QRect r = edit->geometry();
        r.setRight( re.right() - pix.width() - 2 );
        if ( r != edit->geometry() )
            edit->setGeometry( r );

        p.drawPixmap( re.right() - pix.width() -1 , re.y() + ( re.height() - pix.height() ) / 2, pix );
        p.setClipping( false );
    } else {
        QRect r = edit->geometry();
        r.setRight( re.right() );
        if ( r != edit->geometry() )
            edit->setGeometry( r );
    }
}

void KonqCombo::setPageSecurity( int pageSecurity )
{
    int ops = m_pageSecurity;
    m_pageSecurity = pageSecurity;
    if (ops != pageSecurity)
        update();
}

bool KonqCombo::hasSufficientContrast(const QColor &c1, const QColor &c2)
{
    // Taken from khtml/misc/helper.cc
#define HUE_DISTANCE 40
#define CONTRAST_DISTANCE 10

    int h1, s1, v1, h2, s2, v2;
    int hdist = -CONTRAST_DISTANCE;
    c1.getHsv(&h1,&s1,&v1);
    c2.getHsv(&h2,&s2,&v2);
    if(h1!=-1 && h2!=-1) { // grey values have no hue
        hdist = std::abs(h1-h2);
        if (hdist > 180) hdist = 360-hdist;
        if (hdist < HUE_DISTANCE) {
            hdist -= HUE_DISTANCE;
            // see if they are high key or low key colours
            bool hk1 = h1>=45 && h1<=225;
            bool hk2 = h2>=45 && h2<=225;
            if (hk1 && hk2)
                hdist = (5*hdist)/3;
            else if (!hk1 && !hk2)
                hdist = (7*hdist)/4;
        }
        hdist = std::min(hdist, HUE_DISTANCE*2);
    }
    return hdist + (std::abs(s1-s2)*128)/(160+std::min(s1,s2)) + std::abs(v1-v2) > CONTRAST_DISTANCE;
}

void KonqCombo::slotReturnPressed()
{
    slotActivated(currentText());
}

void KonqCombo::slotCompletionModeChanged(KGlobalSettings::Completion mode)
{
    if (mode == KGlobalSettings::CompletionNone)
        connect(this, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
    else
        disconnect(this, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
}


///////////////////////////////////////////////////////////////////////////////

KonqListWidgetItem::KonqListWidgetItem( QListWidget *parent )
    : QListWidgetItem( parent, KonqItemType ), lookupPending( true )
{
}

KonqListWidgetItem::KonqListWidgetItem( const QString &text, QListWidget *parent )
    : QListWidgetItem( text, parent, KonqItemType ), lookupPending( true )
{
}

QVariant KonqListWidgetItem::data( int role ) const
{
    if ( lookupPending && role != Qt::DisplayRole )
    {
        QString title = titleOfURL( text() );
        QPixmap pixmap;

        KonqPixmapProvider *provider = KonqPixmapProvider::self();

        if ( !title.isEmpty() )
            pixmap = provider->pixmapFor( text(), KIconLoader::SizeSmall );
        else if ( text().indexOf( "://" ) == -1 ) {
            title = titleOfURL( "http://"+text() );
            if ( !title.isEmpty() )
                pixmap = provider->pixmapFor( "http://"+text(), KIconLoader::SizeSmall );
            else
                pixmap = provider->pixmapFor( text(), KIconLoader::SizeSmall );
        }

        const_cast<KonqListWidgetItem*>( this )->setIcon( pixmap );
        const_cast<KonqListWidgetItem*>( this )->setData( Qt::UserRole, title );

        lookupPending = false;
    }

    return QListWidgetItem::data( role );
}

bool KonqListWidgetItem::reuse(const QString &newText)
{
    if ( text() == newText )
        return false;

    lookupPending = true;
    setText( newText );
    return true;
}

///////////////////////////////////////////////////////////////////////////////

QSize KonqComboItemDelegate::sizeHint( const QStyleOptionViewItem &option,
                                       const QModelIndex &index ) const
{
    Q_UNUSED(index);
    int vMargin = QApplication::style()->pixelMetric( QStyle::PM_FocusFrameVMargin );

    QSize size( 1, std::max( option.fontMetrics.lineSpacing(), option.decorationSize.height() ) );
    size.rheight() += vMargin * 2;

    return size.expandedTo( QApplication::globalStrut() );
}

void KonqComboItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index ) const
{
    QIcon icon = qvariant_cast<QIcon>( index.data( Qt::DecorationRole ) );
    QString url = index.data( Qt::DisplayRole ).toString();
    QString title = index.data( Qt::UserRole ).toString();

    QIcon::Mode mode = option.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled;
    const QSize size = icon.actualSize( option.decorationSize, mode );
    QPixmap pixmap = icon.pixmap( size, mode );

    QStyleOptionViewItem opt( option );

    painter->save();

    // Draw the item background

    // ### When Qt 4.4 is released we need to change this code to draw the background
    //     by calling QStyle::drawPrimitive() with PE_PanelItemViewRow.
    if ( opt.state & QStyle::State_Selected ) {
        painter->fillRect( option.rect, option.palette.brush( QPalette::Highlight ) );
        painter->setPen( QPen( option.palette.brush( QPalette::HighlightedText ), 0 ) );
    }

    int hMargin = QApplication::style()->pixelMetric( QStyle::PM_FocusFrameHMargin );
    int vMargin = QApplication::style()->pixelMetric( QStyle::PM_FocusFrameVMargin );

    const QRect bounding = option.rect.adjusted( hMargin, vMargin, -hMargin, -vMargin );
    const QSize textSize( bounding.width() - pixmap.width() - 2, bounding.height() );
    const QRect pixmapRect = QStyle::alignedRect( option.direction, Qt::AlignLeft | Qt::AlignVCenter,
                                                  pixmap.size(), bounding );
    const QRect textRect = QStyle::alignedRect( option.direction, Qt::AlignRight, textSize, bounding );

    if ( !pixmap.isNull() )
        painter->drawPixmap( pixmapRect.topLeft(), pixmap );

    QSize titleSize( ( bounding.width() / 3 ) - 1, textRect.height() );
    if (title.isEmpty()) {
        // Don't truncate the urls when there is no title to show (e.g. local files)
        // Ideally we would do this globally for all items - reserve space for a title for all, or for none
        titleSize = QSize();
    }
    const QSize urlSize( textRect.width() - titleSize.width() - 2, textRect.height() );
    const QRect titleRect = QStyle::alignedRect( option.direction, Qt::AlignRight, titleSize, textRect );
    const QRect urlRect   = QStyle::alignedRect( option.direction, Qt::AlignLeft, urlSize, textRect );

    if ( !url.isEmpty() ) {
        QString squeezedText = option.fontMetrics.elidedText( url, Qt::ElideMiddle, urlRect.width() );
        painter->drawText( urlRect, Qt::AlignLeft | Qt::AlignVCenter, squeezedText );
    }

    if ( !title.isEmpty() ) {
        QString squeezedText = option.fontMetrics.elidedText( title, Qt::ElideRight, titleRect.width() );
        QFont font = painter->font();
        font.setItalic( true );
        painter->setFont( font );
        QColor color = painter->pen().color();
        color.setAlphaF(.75);
        painter->setPen(color);
        painter->drawText( titleRect, Qt::AlignLeft | Qt::AlignVCenter, squeezedText );
    }

    painter->restore();
}

///////////////////////////////////////////////////////////////////////////////

KonqComboLineEdit::KonqComboLineEdit( QWidget *parent )
                  :KLineEdit( parent )
{
    setClearButtonShown( true );
}

void KonqComboLineEdit::mouseDoubleClickEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::LeftButton ) {
	selectAll();
	return;
    }
    KLineEdit::mouseDoubleClickEvent( e );
}

// KDE5 TODO: get rid of this duplicated code, and just reimplement completionBox()

void KonqComboLineEdit::setCompletedItems( const QStringList& items, bool )
{
    QString txt;
    KonqComboCompletionBox *completionbox = static_cast<KonqComboCompletionBox*>( completionBox() );

    if ( completionbox && completionbox->isVisible() )
        // The popup is visible already - do the matching on the initial string,
        // not on the currently selected one.
        txt = completionbox->cancelledText();
    else
        txt = text();

    if ( !items.isEmpty() && !(items.count() == 1 && txt == items.first()) ) {
        if ( !completionBox( false ) ) {
            setCompletionBox( new KonqComboCompletionBox( this ) );
            completionbox = static_cast<KonqComboCompletionBox*>( completionBox() );
        }

        if ( completionbox->isVisible() ) {

            // This code is copied from KLineEdit::setCompletedItems
            QListWidgetItem* currentItem = completionbox->currentItem();

            QString currentSelection;
            if ( currentItem != 0 ) {
                currentSelection = currentItem->text();
            }

            completionbox->setItems( items );
            const QList<QListWidgetItem*> matchedItems = completionbox->findItems(currentSelection, Qt::MatchExactly);
            QListWidgetItem* matchedItem = matchedItems.isEmpty() ? 0 : matchedItems.first();

            if (matchedItem) {
                const bool blocked = completionbox->blockSignals(true);
                completionbox->setCurrentItem(matchedItem);
                completionbox->blockSignals(blocked);
            } else {
                completionbox->setCurrentRow(-1);
            }
        }
        else { // completion box not visible yet -> show it
            if ( !txt.isEmpty() )
                completionbox->setCancelledText( txt );
            completionbox->setItems( items );
            completionbox->popup();
        }

        if ( autoSuggest() ) {
            int index = items.first().indexOf( txt );
            QString newText = items.first().mid( index );
            setUserSelection( false );
            setCompletedText( newText, true );
        }
    }
    else
        if ( completionbox && completionbox->isVisible() )
            completionbox->hide();
}

///////////////////////////////////////////////////////////////////////////////

KonqComboCompletionBox::KonqComboCompletionBox( QWidget *parent )
                       :KCompletionBox( parent )
{
    setLayoutDirection(Qt::LeftToRight);
}

void KonqComboCompletionBox::setItems( const QStringList& items )
{
    bool block = signalsBlocked();
    blockSignals( true );

    int rowIndex = 0;

    if ( count() == 0 )
        insertStringList( items );
    else {
        //Keep track of whether we need to change anything,
        //so we can avoid a repaint for identical updates,
        //to reduce flicker
        bool dirty = false;

        QStringList::ConstIterator it = items.constBegin();
        const QStringList::ConstIterator itEnd = items.constEnd();

        for ( ; it != itEnd; ++it) {
            if ( rowIndex < count() ) {
                const bool changed = (static_cast<KonqListWidgetItem*>(item(rowIndex)))->reuse( *it );
                dirty = dirty || changed;
            }
            else {
                dirty = true;
                //Inserting an item is a way of making this dirty
                addItem( new KonqListWidgetItem( *it ) );
            }
            rowIndex++;
        }

        //If there is an unused item, mark as dirty -> less items now
        if ( rowIndex < count() )
            dirty = true;

        while ( rowIndex < count() ) {
            delete item(rowIndex);
        }

        //TODO KDE 4 - Port this
        //if ( dirty )
        //    triggerUpdate( false );
    }

    if ( isVisible() && size().height() != sizeHint().height() )
        sizeAndPosition();

    blockSignals( block );
}

void KonqComboCompletionBox::insertStringList( const QStringList & list, int index )
{
    if ( index < 0 )
        index = count();

    foreach ( const QString &text, list )
        insertItem( index++, new KonqListWidgetItem( text ) );
}
#include "konqcombo.moc"
