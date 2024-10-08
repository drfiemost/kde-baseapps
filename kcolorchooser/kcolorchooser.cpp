/*
This file is part of KDE 

  Copyright (C) 1998-2000 Waldo Bastian (bastian@kde.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include <kcolordialog.h>
#include <kcolormimedata.h>
#include <khelpmenu.h>

#include <QtGui/QClipboard>

static const char description[] =
	I18N_NOOP("KDE Color Chooser");

static const char version[] = "v1.0.1";

	
int main(int argc, char *argv[])
{
  KAboutData aboutData("kcolorchooser", 0, ki18n("KColorChooser"),
		version, ki18n(description), KAboutData::License_BSD,
		ki18n("(c) 2000, Waldo Bastian"));
  aboutData.addAuthor(ki18n("Waldo Bastian"),KLocalizedString(), "bastian@kde.org");
  aboutData.setProductName("kdelibs/kdeui");
  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("print", ki18n("Print the selected color to stdout"));
  options.add("color <color>", ki18n("Set initially selected color"));
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;
  
  KColorDialog dlg;

  KHelpMenu *help = new KHelpMenu(&dlg, &aboutData);

  QColor color = KColorMimeData::fromMimeData( QApplication::clipboard()->mimeData( QClipboard::Clipboard ));
  if (!color.isValid()) {
    color = Qt::blue; // Just a color
  }
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->isSet("color")) {
    QColor c = QColor(args->getOption("color"));
    if (c.isValid())
      color = c;
  }
  dlg.setButtons(KDialog::Help | KDialog::Close);
  dlg.setButtonMenu(KDialog::Help, (QMenu *)(help->menu()));
  dlg.setColor(color);

  app.connect(&dlg, SIGNAL(finished()), SLOT(quit()));

  dlg.show();
  app.exec();

  const  QColor c = dlg.color();
  if ( args->isSet("print") && c.isValid() ) {
      std::cout << c.name().toUtf8().constData() << std::endl;
  }
  args->clear();
}  
