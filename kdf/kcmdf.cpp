/*
  kcmdf.cpp - KcmDiskFree

  written 1998 by Michael Kropfberger <michael.kropfberger@gmx.net>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
  */

//
// 1999-12-05 Espen Sand 
// Modified to use KCModule instead of the old and obsolete 
// KControlApplication
//


#include <kapp.h>
#include <kdialog.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include <qframe.h>
#include <qlayout.h>

#include "kcmdf.h"
#include "kcmodule.h"

KDiskFreeWidget::KDiskFreeWidget( QWidget *parent, const char *name )
 : KCModule( parent, name )
{
  setButtons(Help|Ok);

  QVBoxLayout *topLayout = new QVBoxLayout( this );

  mPage = new QFrame( this, "page" );
  topLayout->addWidget( mPage, 10 );
  QVBoxLayout *vbox = new QVBoxLayout( mPage, KDialog::spacingHint() );
  mKdf = new KDFWidget( mPage, "kdf", false );
  vbox->addWidget( mKdf, 10 );
}

void KDiskFreeWidget::load( void )
{
  //
  // 1999-12-05 Espen Sand
  // I don't use this one because 1) The widgets will do a 
  // loadSettings() on startup and 2) Reset button is not used.
  //
}


void KDiskFreeWidget::save( void )
{
  mKdf->applySettings();
}


void KDiskFreeWidget::defaults( void )
{
  mKdf->loadSettings();
}

extern "C"
{
  KCModule* create_kdf( QWidget *parent, const char * /*name*/ )
  {
    KGlobal::locale()->insertCatalogue("kdf");
    return new KDiskFreeWidget( parent );
  }
}

#include "kcmdf.moc"
