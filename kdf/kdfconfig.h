/*
* kdfconfig.h
*
* Copyright (c) 1999 Michael Kropfberger <michael.kropfberger@gmx.net>
*               2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KDFCONFIG_H
#define KDFCONFIG_H

#include "stdoption.h"
#include "kdfwidget.h"

class QCloseEvent;
class QLabel;
class QCheckBox;
class QLCDNumber;
class QSlider;
class QPixmap;
class QTreeWidget;
class QTreeWidgetItem;

class Column;

class KLineEdit;

class KDFConfigWidget : public QWidget
{
    Q_OBJECT
    
    public:
        explicit KDFConfigWidget( QWidget *parent=0, bool init=false);
        ~KDFConfigWidget();
        
        public slots:
        void loadSettings( void );
        void applySettings( void );
        void defaultsBtnClicked( void );
        
        protected slots:
        void slotChanged();
        
        private slots:
        void toggleListText( QTreeWidgetItem *item, int column );
    
    protected:
        void closeEvent( QCloseEvent * );
    
    private:
        QLCDNumber *mLCD;
        KLineEdit  *mFileManagerEdit;
        QCheckBox  *mOpenMountCheck;
        QCheckBox  *mPopupFullCheck;
        CStdOption mStd;
        
        QPixmap iconVisible;
        QPixmap iconHidden;
        
        QTreeWidget * m_listWidget;
        QList<Column> m_columnList;
        
        QSlider * m_updateSlider;

    Q_SIGNALS:
        void configChanged();
};

#endif

