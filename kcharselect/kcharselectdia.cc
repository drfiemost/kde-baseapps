/******************************************************************/
/* KCharSelectDia - (c) by Reginald Stadlbauer 1999               */
/* Author: Reginald Stadlbauer                                    */
/* E-Mail: reggie@kde.org                                         */
/*         RTL support by Bryce Nesbitt                           */
/******************************************************************/

#include "kcharselectdia.moc"

#include <stdlib.h>

#include <kdialog.h>
#include <kpopupmenu.h>
#include <kapplication.h>
#include <kaccel.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <kstdguiitem.h>


/******************************************************************/
/* class KCharSelectDia                                           */
/******************************************************************/

//==================================================================
KCharSelectDia::KCharSelectDia(QWidget *parent,const char *name,
			       const QChar &_chr,const QString &_font,
			       int _tableNum, bool direction)
  : KMainWindow(parent,name), vChr(_chr), vFont(_font)
{
  setCaption(QString::null); // Standard caption

  QWidget *mainWidget = new QWidget(this);
  setCentralWidget(mainWidget);

  grid = new QGridLayout( mainWidget, 3, 4, KDialog::marginHint(), KDialog::spacingHint() );

  // Add character selection widget from library kdeui
  charSelect = new KCharSelect(mainWidget,"",vFont,vChr,_tableNum);
  charSelect->resize(charSelect->sizeHint());
  connect(charSelect,SIGNAL(highlighted(const QChar &)),
	  SLOT(charChanged(const QChar &)));
  connect(charSelect,SIGNAL(activated(const QChar &)),
	  SLOT(add(const QChar &)));
  connect(charSelect,SIGNAL(fontChanged(const QString &)),
	  SLOT(fontSelected(const QString &)));
  grid->addMultiCellWidget(charSelect, 0, 0, 0, 3);

  // Build line editor
  lined = new QLineEdit(mainWidget);
  lined->resize(lined->sizeHint());

  QFont font = lined->font();
  font.setFamily( vFont );
  lined->setFont( font );

  connect(lined,SIGNAL(textChanged(const QString &)),
	  SLOT(lineEditChanged()));
  grid->addMultiCellWidget(lined, 1, 1, 0, 3);

  // Build some buttons
  bHelp = new KPushButton( KStdGuiItem::help(), mainWidget );
  connect(bHelp,SIGNAL(clicked()),this,SLOT(help()));
  bHelp->setFixedSize( bHelp->sizeHint() );
  grid->addWidget( bHelp, 2, 0 );

  QSpacerItem *space = new QSpacerItem( 20, 20, QSizePolicy::Expanding );
  grid->addItem( space, 2, 1 );

  bClear = new KPushButton( KStdGuiItem::clear(), mainWidget );
  connect(bClear,SIGNAL(clicked()),this,SLOT(clear()));
  bClear->setFixedSize( bClear->sizeHint() );
  grid->addWidget( bClear, 2, 2 );

  bClip = new KPushButton( KGuiItem( i18n( "&To Clipboard" ),
            "editcopy" ), mainWidget );
  bClip->setFixedSize( bClip->sizeHint() );
  connect(bClip,SIGNAL(clicked()),this,SLOT(toClip()));
  grid->addWidget( bClip, 2, 3 );

  // Build menu
  int id;
  KAccel *keys = new KAccel( this );

  QPopupMenu *file = new QPopupMenu( this );
  id = file->insertItem( SmallIcon( "exit" ), KStdGuiItem::quit().text(),
            this, SLOT(_exit()));
  keys->changeMenuAccel(file, id, KStdAccel::Quit);

  keys->insert( KStdAccel::Paste, this, SLOT(fromClip()));
  keys->insert( KStdAccel::Copy , this, SLOT(toClip()));
  keys->insert( KStdAccel::Quit , this, SLOT(_exit()));
  keys->insert( KStdAccel::Help , this, SLOT(help()));

  QPopupMenu *edit = new QPopupMenu( this );
  id = edit->insertItem( SmallIcon( "editcopy" ), i18n("&To Clipboard"),
            this, SLOT(toClip()) );
  keys->changeMenuAccel(edit, id, KStdAccel::Copy);
  id = edit->insertItem( i18n("To Clipboard &UTF-8"),
            this, SLOT(toClipUTF8()) );
  id = edit->insertItem( i18n("To Clipboard &HTML"),
            this, SLOT(toClipHTML()) );
  id = edit->insertItem( SmallIcon( "editpaste"), i18n("From Clipboard"),
            this, SLOT(fromClip()) );
  keys->changeMenuAccel(edit, id, KStdAccel::Paste);
  id = edit->insertItem( i18n("From Clipboard UTF-8"),
            this, SLOT(fromClipUTF8()) );
  i18n("From Clipboard HTML");      // Intended for future use
  id = edit->insertSeparator();
  id = edit->insertItem( SmallIcon( "locationbar_erase" ), KStdGuiItem::clear().text(),
            this, SLOT(clear())     );
  id = edit->insertItem( i18n("&Flip"), this, SLOT(flipText())  );
  id = edit->insertItem( i18n("&Alignment"),
            this, SLOT(toggleEntryDirection()) );

  menuBar()->insertItem( i18n("&File"), file );
  menuBar()->insertItem( i18n("&Edit"), edit );
  menuBar()->insertItem( KStdGuiItem::help().text(), helpMenu() );

  charSelect->setFocus();

  resize(grid->sizeHint());

  entryDirection = direction;
  if( entryDirection )
    lined->setAlignment( Qt::AlignRight );
  else
    lined->setAlignment( Qt::AlignLeft );
}

//==================================================================
void KCharSelectDia::charChanged(const QChar &_chr)
{
  vChr = _chr;
}

//==================================================================
void KCharSelectDia::fontSelected(const QString &_font)
{
  charSelect->setFont(_font);

  QFont font = lined->font();
  font.setFamily( _font );
  lined->setFont( font );

  vFont = _font;
}

//==================================================================
void KCharSelectDia::add(const QChar &_chr)
{
  QString str;
  int cursorPos;

  charChanged(_chr);

  str       = lined->text();
  cursorPos = lined->cursorPosition();
  str.insert( cursorPos, vChr );
  lined->setText(str);
  cursorPos++;
  lined->setCursorPosition( cursorPos );
}

//==================================================================
void KCharSelectDia::toClip()
{
  QClipboard *cb = QApplication::clipboard();
  cb->setSelectionMode( true );
  cb->setText(lined->text());
  cb->setSelectionMode( false );
  cb->setText(lined->text());
}

//==================================================================
// UTF-8 is rapidly becoming the favored 8-bit encoding for
// Unicode (iso10646-1).
//
void KCharSelectDia::toClipUTF8()
{
  QClipboard *cb = QApplication::clipboard();
  QString str = lined->text();
  cb->setText(str.utf8());
}

//==================================================================
//  Put valid HTML 4.0 into the clipboard.  Valid ISO-8859-1 Latin 1
//  characters are left undisturbed.  Everything else, including the
//  "c0 control characters" often used by Windows, are clipped
//  as a HTML entity.
//
void KCharSelectDia::toClipHTML()
{
  QClipboard *cb = QApplication::clipboard();
  QString input;
  QString html;
  QString tempstring;
  QChar   tempchar;
  uint i;

  input = lined->text();
  for(i=0; i< input.length(); i++ )
    {
      tempchar = input.at(i);
      if(  tempchar.latin1() && ((tempchar.unicode() < 128) || (tempchar.unicode() >= 128+32)) )
        {
          html.append(input.at(i));
        }
      else
        {
          html.append(tempstring.sprintf("&#x%x;", tempchar.unicode()));
        }
    }
  cb->setText(html);
}

//==================================================================
//
void KCharSelectDia::fromClip()
{
  QClipboard *cb = QApplication::clipboard();
  lined->setText( cb->text() );
}

//==================================================================
// UTF-8 is rapidly becoming the favored 8-bit encoding for
// Unicode (iso10646-1).  This function is handy for decoding
// UTF-8 found in legacy applications, consoles, filenames, webpages,
// etc.
//
void KCharSelectDia::fromClipUTF8()
{
  QClipboard *cb = QApplication::clipboard();
  QString str = cb->text();

  lined->setText( str.fromUtf8( str.latin1() ) );
}

//==================================================================
//  Reverse the text held in the line edit buffer.  This is crucial
//  for dealing with visual vs. logical representations of
//  right to left languages, and handy for working around all
//  manner of legacy character order issues.
//
void KCharSelectDia::flipText()
{
  QString input;
  QString output;
  uint i;

  input = lined->text();
  for(i=0; i< input.length(); i++ )
    {
      output.prepend( input.at(i) );
    }
  lined->setText(output);
}

//==================================================================
void KCharSelectDia::toggleEntryDirection()
{
    entryDirection ^= 1;
    if( entryDirection )
        lined->setAlignment( Qt::AlignRight );
    else
        lined->setAlignment( Qt::AlignLeft );
}

//==================================================================
void KCharSelectDia::lineEditChanged()
{
    if( entryDirection )
      {
        if(lined->cursorPosition())
            lined->setCursorPosition( lined->cursorPosition() - 1 );
      }
}

//==================================================================
void KCharSelectDia::_exit()
{
  KConfig *config = kapp->config();

  config->setGroup("General");
  config->writeEntry("selectedFont",vFont);
  config->writeEntry("char",static_cast<int>(vChr.unicode()));
  config->writeEntry("table",charSelect->tableNum());
  config->writeEntry("entryDirection",entryDirection);
  config->sync();

  delete this;
  exit(0);
}

//==================================================================
void KCharSelectDia::clear()
{
  lined->clear();
}

//==================================================================
void KCharSelectDia::help()
{
  kapp->invokeHelp();
}
