/*
  kwikdisk.cpp - KDiskFree

  written 1999 by Michael Kropfberger <michael.kropfberger@gmx.net>
  
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


#include <qlabel.h>
#include <qpixmap.h>
#include <qbitmap.h>

#include <kapp.h>
#include <kwm.h>
#include <kiconloader.h>
#include <kmsgbox.h>

#include "kwikdisk.h"
#include "kwikdisk.moc"


#define DEFAULT_FREQ 60
#define DEFAULT_FILEMGR_COMMAND "kfmclient openURL %m"
//#define DEFAULT_FILEMGR_COMMAND "kvt -e mc %m"


/***************************************************************/
/***************************************************************/
/***************************************************************/
MyToolTip::MyToolTip(QWidget *parent, QToolTipGroup * group= 0 ) 
      : QToolTip(parent,group)
{
  tipping=FALSE;
};

void MyToolTip::setPossibleTip(const QRect &the_rect,const char *the_text)
{
  //debug("setPossibleTip");
  rect=the_rect;
  text=the_text;
  setTipping(TRUE);
};

void MyToolTip::maybeTip(const QPoint &p)
{
  if ((!tipping) || (text==0) )return;
  //   debug("maybeTip in %i:%i [%s]  for: %ix%i+%i+%i",p.x(),p.y()
  //              ,text,rect.x(),rect.y(),rect.width(),rect.height());
  tip(rect,text);
}; 

/***************************************************************/
/***************************************************************/
/***************************************************************/
MyPopupMenu::MyPopupMenu(QWidget *parent=0, const char *name=0)
  : QPopupMenu(parent,name)
{
  toolTip=new MyToolTip(this);
  toolTipRow=-1;
};

void MyPopupMenu::mouseMoveEvent(QMouseEvent *e)
{
  //debug("mouse moved...");

  int row = findRow( e->pos().y() );		// ask table for row
  int col = findCol( e->pos().x() );		// ask table for column
  if ((row != -1) && (col != -1) && (row!=toolTipRow)) {
    //debug("mouse moved... we are now in row %i  col %i",row,col);
      toolTipRow=row;
      int cellPos=0;
      for (int j=0;j<row;j++) cellPos += cellHeight(j);
      toolTip->setPossibleTip(QRect(0,cellPos
                             ,totalWidth(),cellHeight(row)+2)
                             ,toolTips[idAt(row)]);
    };

  QPopupMenu::mouseMoveEvent(e);  //SuperCall
}

void MyPopupMenu::setToolTip(int id, const char *text)
{
  const char *tmp;
  tmp=toolTips.find(id);
  if (tmp!=0) {
    toolTips.remove(id);
    delete tmp;
  }
  toolTips.insert(id,text);
};



/***************************************************************/
/***************************************************************/
/***************************************************************/
DockWidget::DockWidget(QWidget *parent, const char *name=0)
  : QLabel(parent,name)
{
  connect(&diskList,SIGNAL(readDFDone()),this,SLOT(updateDFDone()) );
  connect(&diskList , SIGNAL(criticallyFull(DiskEntry*)),
           this, SLOT (criticallyFull(DiskEntry*)) );

  clickMenu=0;

  //CONFIGURATION WINDOW
  tabconf=new QTabDialog(); CHECK_PTR(tabconf);
  mntconf=new MntConfigWidget(tabconf,"mntconf");CHECK_PTR(mntconf);
  kdfconf=new KDFConfigWidget(tabconf,"kdfconf");CHECK_PTR(kdfconf);

   config = kapp->getConfig();
   loadSettings();
};

/***************************************************************************
  * saves KConfig and starts the df-cmd
**/
void DockWidget::applySettings()
{
  //debug("DockWidget::applySettings");
}

void DockWidget::confApplySettings()
{
  //debug("DockWidget::confApplySettings");
  this->applySettings();
  mntconf->applySettings();
  kdfconf->applySettings();
  this->loadSettings();  //get the new changes
}

/***************************************************************************
  * reads the KConfig
**/
void DockWidget::loadSettings()
{
  //debug("DockWidget::loadSettings");
 config->setGroup("KDFConfig");
 updateFreq=config->readNumEntry("UpdateFrequency",DEFAULT_FREQ);
 fileMgr=config->readEntry("FileManagerCommand",DEFAULT_FILEMGR_COMMAND);
 popupIfFull=config->readBoolEntry("PopupIfFull",popupIfFull);
 openFileMgrOnMount=config->readBoolEntry("OpenFileMgrOnMount"
                                          ,openFileMgrOnMount);

 //setUpdateFreq(updateFreq);
 // updateDF();
}

void DockWidget::confLoadSettings()
{
  //debug("DockWidget::confLoadSettings");
 mntconf->loadSettings();
 kdfconf->loadSettings(); 
}

 
/**************************************************************************
  * connected with diskList
**/
void DockWidget::criticallyFull(DiskEntry *disk )
{
  //debug("DockWidget::criticallyFull");
  if (popupIfFull) {
    QString s;
    s.sprintf("Device [%s] on [%s] is getting critically full!",
             disk->deviceName().data(),
             disk->mountPoint().data());
       KMsgBox::message(this,klocale->translate("KwikDisk"),
              klocale->translate(s.data())
               ,KMsgBox::EXCLAMATION);
  }
}


/***************************************************************************
  * resets the timer for automatic df-refreshes
**/
void DockWidget::setUpdateFreq(int freq)
{
  //debug("DockWidget::setUpdateFreq");
  killTimers(); //kills !all! running timers
  updateFreq=freq;

  if (updateFreq > 0)  //0 sets to NO_AUTO_UPDATE   ;)
    startTimer( updateFreq * 1000 );

  applySettings();
}

/***************************************************************************
  * Update (reread) all disk-dependencies
**/
void DockWidget::timerEvent(QTimerEvent *) 
{ 
  //debug("DockWidget::timerEvent");
  updateDF();
};


void DockWidget::startKDF() 
{
  system("kdf &");
};

void DockWidget::mousePressEvent(QMouseEvent *e)
{ 
  updateDF();
};

void DockWidget::sysCallError(DiskEntry *disk, int errno)
{ 
  if (errno!=0)
     KMsgBox::message(this,klocale->translate("KwikDisk"),
              disk->lastSysError(),KMsgBox::STOP);
};

/***************************************************************************
  * checks fstab & df 
**/
void DockWidget::updateDF()
{
  readingDF=TRUE;
  diskList.readFSTAB();
  diskList.readDF(); 
};

void DockWidget::toggleMount( )
{
  //debug("DockWidget::toggleMount");
  DiskEntry *disk;
  
    if (!readingDF) {
      if (!disk->toggleMount())
         KMsgBox::message(this,klocale->translate("KDiskFree"),
              disk->lastSysError(),KMsgBox::STOP);
      else 
        if ((openFileMgrOnMount) && (!disk->mounted())) {
           //open fileManager on MountPoint
           QString cmdS;
           if ( !fileMgr.isEmpty() ) {
           cmdS=fileMgr;
           int pos=cmdS.find("%m");
           if ( pos > 0 ) {
              cmdS=cmdS.replace(pos,2
                           ,(const char *)disk->mountPoint() );
             cmdS.append(" &");
           } else //no %m found; just add path on the back
             cmdS=cmdS+" "+(const char *)disk->mountPoint()+" &";
           debug("fileMgr-cmd: [%s]",(const char *)cmdS);
           system(cmdS);
       }//if 

	} 
      updateDF();
    }
}
void DockWidget::updateDFDone()
{ 
  //debug("DockWidget::updateDFDone() %p",clickMenu);
  // pops up the menu

  if (clickMenu!=0) delete clickMenu;
  clickMenu = new MyPopupMenu; CHECK_PTR(clickMenu);

  DiskEntry* disk;
  QString *toolTipText;
  QString entryName;
  KIconLoader *loader = kapp->getIconLoader();
  QPixmap *pix;

  int id;
  for (disk=diskList.first();disk!=0;disk=diskList.next()) {
    toolTipText = new QString;
    toolTipText->sprintf("(%s) %s on %s",disk->fsType().data()
                                        ,disk->deviceName().data()
                                        ,disk->mountPoint().data());
    if (disk->mounted())  toolTipText->prepend("UNMOUNT");
    else toolTipText->prepend("MOUNT");
    entryName=disk->mountPoint().data();
    if (disk->mounted()) entryName+=QString("   [")
                                  +disk->prettyKBAvail().data()
                                  +QString("]");

    id=clickMenu->insertItem("",disk, SLOT(toggleMount()));
    //    id=clickMenu->insertItem("",this, SLOT(toggleMount()));

    pix = new QPixmap(loader->loadMiniIcon(disk->iconName()));
 
    if ((getuid()!=0) && ( -1==disk->mountOptions().find("user",0,FALSE) )) {
      //clickMenu->setItemEnabled(id,FALSE);  MntConfigWidget   *mntconf;
          // special root icon, normal user can�t mount
         QPainter *qp;
         QBitmap *bm=new QBitmap(*(pix->mask()));
         int w=1;  //width of the rect
         if (bm != 0) { //a mask exists, draw the rect on the mask first
           qp=new QPainter(bm);
           qp->setPen(QPen(white,w));
           qp->drawRect(0,0,bm->width(),bm->height());
           qp->end();
           delete qp;
           pix->setMask(*bm);
         }
          qp=new QPainter(pix);
          qp->setPen(QPen(red,w));
          qp->drawRect(0,0,pix->width(),pix->height());
          qp->end();
          delete qp;
	 

       clickMenu->disconnectItem(id,disk,SLOT(toggleMount()));
       *toolTipText=klocale->translate("sorry, not root...");
    };
    clickMenu->changeItem(*pix,entryName,id);
    
    connect(disk, SIGNAL(sysCallError(DiskEntry *, int) ),
            this, SLOT(sysCallError(DiskEntry *, int)) );

    clickMenu->setToolTip(id,toolTipText->data());
  } // every device found
  readingDF=FALSE;

  clickMenu->insertSeparator();
  id=clickMenu->insertItem(loader->loadMiniIcon("kdf.xpm")
                         ,klocale->translate("&Start KDiskFree")
                         ,this, SLOT(startKDF()),0);
  id=clickMenu->insertItem(loader->loadMiniIcon("kfloppy.xpm")
                         ,klocale->translate("&Configure KwikDisk")
                         ,this, SLOT(settingsBtnClicked()),0);
  id=clickMenu->insertItem(loader->loadMiniIcon("kdehelp.xpm")
                         ,klocale->translate("&Help")
                         ,this, SLOT(invokeHTMLHelp()),0);
  clickMenu->insertSeparator();
  id=clickMenu->insertItem(loader->loadMiniIcon("delete.xpm")
                         ,klocale->translate("&Quit KwikDisk")
                         ,this, SLOT(quit()),0);

  clickMenu->move(-1000,-1000); 
  clickMenu->show(); 
  clickMenu->hide(); 
  QRect g = KWM::geometry( this->winId() ); 
  if ( g.x() > QApplication::desktop()->width()/2 &&
       g.y()+clickMenu->height() > QApplication::desktop()->height() ) 
      clickMenu->popup(QPoint( g.x(), g.y() - clickMenu->height())); 
  else
      clickMenu->popup(QPoint( g.x() + g.width(), g.y() + g.height())); 

   setCursor(ArrowCursor);
};

/***************************************************************************
  * pops up the SettingsBox if the settingsBtn is clicked
**/
void DockWidget::settingsBtnClicked()
{
   
   //DEL OLD CONFWINDOW
    delete kdfconf;
    delete mntconf;
    delete tabconf;

    //CONFIGURATION WINDOW
      tabconf=new QTabDialog(); CHECK_PTR(tabconf);
      tabconf->setCaption(klocale->translate("KDiskFree/KwikDisk Configuration"));

      mntconf=new MntConfigWidget(tabconf,"mntconf");CHECK_PTR(mntconf);
      kdfconf=new KDFConfigWidget(tabconf,"kdfconf");CHECK_PTR(kdfconf);
      kdfconf->setMinimumSize(460,200);
      tabconf->setApplyButton(klocale->translate("&Apply"));
      connect(tabconf,SIGNAL(applyButtonPressed()),
            this,SLOT(confApplySettings()));
      tabconf->setCancelButton(klocale->translate("&Cancel"));
      connect(tabconf,SIGNAL(cancelButtonPressed()),
            this,SLOT(confLoadSettings()));
      tabconf->addTab(kdfconf,klocale->translate("&General Settings"));
      tabconf->addTab(mntconf,klocale->translate("(U)&MountCommands"));
      tabconf->setMinimumSize(460,200);
  
     tabconf->show();
}


void DockWidget::invokeHTMLHelp() {
  kapp->invokeHTMLHelp("kcontrol/kdf/index.html","");
};

/***************************************************************/
KwikDiskTopLevel::KwikDiskTopLevel(QWidget *, const char *name=0)
      : KTopLevelWidget(name) 
{
    dockIcon=new DockWidget (this,"docked_icon");
    KIconLoader *loader = kapp->getIconLoader();
    dockIcon->setPixmap(QPixmap(loader->loadMiniIcon("kdf.xpm")));
    setView(dockIcon);
    this->resize(24,24);
};


/***************************************************************/
int main(int argc, char **argv)
{
  KApplication app(argc, argv,"kdf");
  
     KwikDiskTopLevel *ktl = new KwikDiskTopLevel();
     CHECK_PTR(ktl);
     ktl->setCaption("KwikDisk");
     app.setMainWidget(ktl);
     KWM::setDockWindow(ktl->winId());
     ktl->show();

  return app.exec();
};
