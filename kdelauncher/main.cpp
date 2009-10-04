/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <kwebpage.h>
#include <kwebview.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include <qwebframe.h>
#include <qwebsettings.h>
#if QT_VERSION >= 0x040600
#include <qwebelement.h>
#endif

#include <QDebug>
#if QT_VERSION >= 0x040400 && !defined(QT_NO_PRINTER)
#include <QPrintPreviewDialog>
#endif

#include <QtUiTools/QUiLoader>

#include <QAction>
#include <QCompleter>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#include <QStringListModel>
#include <QTextStream>
#include <QToolBar>
#include <QToolTip>
#include <QVector>

class WebPage : public KWebPage
{
public:
    WebPage(QWidget *parent) : KWebPage(parent) {}

    virtual KWebPage *createWindow(QWebPage::WebWindowType);
    virtual QObject* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(const QString& url = QString()): currentZoom(100) {
        view = new KWebView(this);
        setCentralWidget(view);

        view->setPage(new WebPage(view));

        connect(view, SIGNAL(loadFinished(bool)),
                this, SLOT(loadFinished()));
        connect(view, SIGNAL(titleChanged(const QString&)),
                this, SLOT(setWindowTitle(const QString&)));
        connect(view->page(), SIGNAL(linkHovered(const QString&, const QString&, const QString &)),
                this, SLOT(showLinkHover(const QString&, const QString&)));
        connect(view->page(), SIGNAL(windowCloseRequested()), this, SLOT(deleteLater()));

        setupUI();

#if QT_VERSION >= 0x040600
        QUrl qurl = view->guessUrlFromString(url);
#else
        QUrl qurl(url);
#endif
        if (qurl.isValid()) {
            urlEdit->setText(qurl.toEncoded());
            view->load(qurl);

            // the zoom values are chosen to be like in Mozilla Firefox 3
            zoomLevels << 30 << 50 << 67 << 80 << 90;
            zoomLevels << 100;
            zoomLevels << 110 << 120 << 133 << 150 << 170 << 200 << 240 << 300;
        }
    }

    KWebPage* webPage() const {
        return view->page();
    }

    KWebView* webView() const {
        return view;
    }

protected slots:

    void changeLocation() {
        QString string = urlEdit->text();
#if QT_VERSION >= 0x040600
        QUrl url = view->guessUrlFromString(string);
#else
        QUrl url(string);
#endif
        if (url.isRelative())
            url = QUrl("http://" + string + "/");
        urlEdit->setText(url.toEncoded());
        view->load(url);
        view->setFocus(Qt::OtherFocusReason);
    }

    void loadFinished() {
        urlEdit->setText(view->url().toString());

        QUrl::FormattingOptions opts;
        opts |= QUrl::RemoveScheme;
        opts |= QUrl::RemoveUserInfo;
        opts |= QUrl::StripTrailingSlash;
        QString s = view->url().toString(opts);
        s = s.mid(2);
        if (s.isEmpty())
            return;

        if (!urlList.contains(s))
            urlList += s;
        urlModel.setStringList(urlList);
    }

    void showLinkHover(const QString &link, const QString &toolTip) {
        statusBar()->showMessage(link);
#ifndef QT_NO_TOOLTIP
        if (!toolTip.isEmpty())
            QToolTip::showText(QCursor::pos(), toolTip);
#endif
    }

    void zoomIn() {
        int i = zoomLevels.indexOf(currentZoom);
        Q_ASSERT(i >= 0);
        if (i < zoomLevels.count() - 1)
            currentZoom = zoomLevels[i + 1];

        view->setZoomFactor(qreal(currentZoom)/100.0);
    }

    void zoomOut() {
        int i = zoomLevels.indexOf(currentZoom);
        Q_ASSERT(i >= 0);
        if (i > 0)
            currentZoom = zoomLevels[i - 1];

        view->setZoomFactor(qreal(currentZoom)/100.0);
    }

    void resetZoom()
    {
       currentZoom = 100;
       view->setZoomFactor(1.0);
    }

    void toggleZoomTextOnly(bool b)
    {
        view->page()->settings()->setAttribute(QWebSettings::ZoomTextOnly, b);
    }

    void print() {
#if QT_VERSION >= 0x040400 && !defined(QT_NO_PRINTER)
        QPrintPreviewDialog dlg(this);
        connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                view, SLOT(print(QPrinter *)));
        dlg.exec();
#endif
    }

    void setEditable(bool on) {
        view->page()->setContentEditable(on);
        formatMenuAction->setVisible(on);
    }

    void dumpHtml() {
        qDebug() << "HTML: " << view->page()->mainFrame()->toHtml();
    }

    void selectElements() {
        bool ok;
        QString str = QInputDialog::getText(this, i18n("Select elements"), i18n("Choose elements"),
                                            QLineEdit::Normal, "a", &ok);
        if (ok && !str.isEmpty()) {
#if QT_VERSION >= 0x040600
            QList<QWebElement> result =  view->page()->mainFrame()->findAllElements(str);
            foreach (QWebElement e, result)
                e.setStyleProperty("background-color", "yellow");
            statusBar()->showMessage(i18n("%1 element(s) selected",result.count()), 5000);
#endif
        }
    }

public slots:

    void newWindow(const QString &url = QString()) {
        MainWindow *mw = new MainWindow(url);
        mw->show();
    }

private:

    QVector<int> zoomLevels;
    int currentZoom;

    // create the status bar, tool bar & menu
    void setupUI() {
        progress = new QProgressBar(this);
        progress->setRange(0, 100);
        progress->setMinimumSize(100, 20);
        progress->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        progress->hide();
        statusBar()->addPermanentWidget(progress);

        connect(view, SIGNAL(loadProgress(int)), progress, SLOT(show()));
        connect(view, SIGNAL(loadProgress(int)), progress, SLOT(setValue(int)));
        connect(view, SIGNAL(loadFinished(bool)), progress, SLOT(hide()));

        urlEdit = new QLineEdit(this);
        urlEdit->setSizePolicy(QSizePolicy::Expanding, urlEdit->sizePolicy().verticalPolicy());
        connect(urlEdit, SIGNAL(returnPressed()),
                SLOT(changeLocation()));
        QCompleter *completer = new QCompleter(this);
        urlEdit->setCompleter(completer);
        completer->setModel(&urlModel);

        QToolBar *bar = addToolBar("Navigation");
        bar->addAction(view->pageAction(QWebPage::Back));
        bar->addAction(view->pageAction(QWebPage::Forward));
        bar->addAction(view->pageAction(QWebPage::Reload));
        bar->addAction(view->pageAction(QWebPage::Stop));
        bar->addWidget(urlEdit);

        QMenu *fileMenu = menuBar()->addMenu(i18n("&File"));
        QAction *newWindow = fileMenu->addAction(i18n("New Window"), this, SLOT(newWindow()));
#if QT_VERSION >= 0x040400
        fileMenu->addAction(i18n("Print"), this, SLOT(print()));
#endif
        fileMenu->addAction(i18n("Close"), this, SLOT(close()));

        QMenu *editMenu = menuBar()->addMenu(i18n("&Edit"));
        editMenu->addAction(view->pageAction(QWebPage::Undo));
        editMenu->addAction(view->pageAction(QWebPage::Redo));
        editMenu->addSeparator();
        editMenu->addAction(view->pageAction(QWebPage::Cut));
        editMenu->addAction(view->pageAction(QWebPage::Copy));
        editMenu->addAction(view->pageAction(QWebPage::Paste));
        editMenu->addSeparator();
        QAction *setEditable = editMenu->addAction(i18n("Set Editable"), this, SLOT(setEditable(bool)));
        setEditable->setCheckable(true);

        QMenu *viewMenu = menuBar()->addMenu(i18n("&View"));
        viewMenu->addAction(view->pageAction(QWebPage::Stop));
        viewMenu->addAction(view->pageAction(QWebPage::Reload));
        viewMenu->addSeparator();
        QAction *zoomIn = viewMenu->addAction(i18n("Zoom &In"), this, SLOT(zoomIn()));
        QAction *zoomOut = viewMenu->addAction(i18n("Zoom &Out"), this, SLOT(zoomOut()));
        QAction *resetZoom = viewMenu->addAction(i18n("Reset Zoom"), this, SLOT(resetZoom()));
        QAction *zoomTextOnly = viewMenu->addAction(i18n("Zoom Text Only"), this, SLOT(toggleZoomTextOnly(bool)));
        zoomTextOnly->setCheckable(true);
        zoomTextOnly->setChecked(false);
        viewMenu->addSeparator();
        viewMenu->addAction(i18n("Dump HTML"), this, SLOT(dumpHtml()));

        QMenu *formatMenu = new QMenu(i18n("F&ormat"));
        formatMenuAction = menuBar()->addMenu(formatMenu);
        formatMenuAction->setVisible(false);
        formatMenu->addAction(view->pageAction(QWebPage::ToggleBold));
        formatMenu->addAction(view->pageAction(QWebPage::ToggleItalic));
        formatMenu->addAction(view->pageAction(QWebPage::ToggleUnderline));
        QMenu *writingMenu = formatMenu->addMenu(i18n("Writing Direction"));
        writingMenu->addAction(view->pageAction(QWebPage::SetTextDirectionDefault));
        writingMenu->addAction(view->pageAction(QWebPage::SetTextDirectionLeftToRight));
        writingMenu->addAction(view->pageAction(QWebPage::SetTextDirectionRightToLeft));

        newWindow->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
        view->pageAction(QWebPage::Back)->setShortcut(QKeySequence::Back);
        view->pageAction(QWebPage::Stop)->setShortcut(Qt::Key_Escape);
        view->pageAction(QWebPage::Forward)->setShortcut(QKeySequence::Forward);
        view->pageAction(QWebPage::Reload)->setShortcut(QKeySequence::Refresh);
        view->pageAction(QWebPage::Undo)->setShortcut(QKeySequence::Undo);
        view->pageAction(QWebPage::Redo)->setShortcut(QKeySequence::Redo);
        view->pageAction(QWebPage::Cut)->setShortcut(QKeySequence::Cut);
        view->pageAction(QWebPage::Copy)->setShortcut(QKeySequence::Copy);
        view->pageAction(QWebPage::Paste)->setShortcut(QKeySequence::Paste);
        zoomIn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
        zoomOut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
        resetZoom->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
        view->pageAction(QWebPage::ToggleBold)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
        view->pageAction(QWebPage::ToggleItalic)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
        view->pageAction(QWebPage::ToggleUnderline)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));

        QMenu *toolsMenu = menuBar()->addMenu(i18n("&Tools"));
        toolsMenu->addAction(i18n("Select elements..."), this, SLOT(selectElements()));

    }

    KWebView *view;
    QLineEdit *urlEdit;
    QProgressBar *progress;

    QAction *formatMenuAction;

    QStringList urlList;
    QStringListModel urlModel;
};

KWebPage *WebPage::createWindow(QWebPage::WebWindowType)
{
    MainWindow *mw = new MainWindow;
    return mw->webPage();
}

QObject *WebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    QUiLoader loader;
    return loader.createWidget(classId, view());
}

class URLLoader : public QObject
{
    Q_OBJECT
public:
    URLLoader(KWebView* view, const QString& inputFileName)
        : m_view(view)
        , m_stdOut(stdout)
    {
        init(inputFileName);
    }

public slots:
    void loadNext()
    {
        QString qstr;
        if (getUrl(qstr)) {
            QUrl url(qstr, QUrl::StrictMode);
            if (url.isValid()) {
                m_stdOut << "Loading " << qstr << " ......" << endl;
                m_view->load(url);
            } else
                loadNext();
        } else
            disconnect(m_view, 0, this, 0);
    }

private:
    void init(const QString& inputFileName)
    {
        QFile inputFile(inputFileName);
        if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&inputFile);
            QString line;
            while (true) {
                line = stream.readLine();
                if (line.isNull())
                    break;
                m_urls.append(line);
            }
        } else {
            qDebug() << "Can't open list file";
            exit(0);
        }
        m_index = 0;
        inputFile.close();
    }

    bool getUrl(QString& qstr)
    {
        if (m_index == m_urls.size())
            return false;

        qstr = m_urls[m_index++];
        return true;
    }

private:
    QVector<QString> m_urls;
    int m_index;
    KWebView* m_view;
    QTextStream m_stdOut;
};

#include "main.moc"

int main(int argc, char **argv)
{
    KAboutData about("KDELauncher", 0, ki18n("KDELauncher"), "0.0000013");
    KCmdLineArgs::init(argc, argv, &about);

    KApplication app;
    QString url = QString("%1/%2").arg(QDir::homePath()).arg(QLatin1String("index.html"));

    QWebSettings::setMaximumPagesInCache(4);

    QWebSettings::setObjectCacheCapacities((16*1024*1024) / 8, (16*1024*1024) / 8, 16*1024*1024);

    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    const QStringList args = app.arguments();

    if (args.contains(QLatin1String("-r"))) {
        // robotized
        QString listFile = args.at(2);
        if (!(args.count() == 3) && QFile::exists(listFile)) {
            qDebug() << "Usage: KDELauncher -r listfile";
            exit(0);
        }
        MainWindow window;
        KWebView *view = window.webView();
        URLLoader loader(view, listFile);
        QObject::connect(view, SIGNAL(loadFinished(bool)), &loader, SLOT(loadNext()));
        loader.loadNext();
        window.show();
        return app.exec();
    } else {
        if (args.count() > 1)
            url = args.at(1);

        MainWindow window(url);

        // Opens every given urls in new windows
        for (int i = 2; i < args.count(); i++)
            window.newWindow(args.at(i));

        window.show();
        return app.exec();
    }
}
