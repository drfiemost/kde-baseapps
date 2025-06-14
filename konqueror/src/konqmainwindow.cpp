/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2000-2005 David Faure <faure@kde.org>
   Copyright (C) 2007 Eduardo Robles Elvira <edulix@gmail.com>
   Copyright (C) 2007 Daniel García Moreno <danigm@gmail.com>

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

#include "konqmainwindow.h"
#include "konqmouseeventfilter.h"
#include "konqclosedwindowsmanager.h"
#include "konqsessionmanager.h"
#include "konqsessiondlg.h"
#include "konqdraggablelabel.h"
#include "konqcloseditem.h"
#include "konqapplication.h"
#include "konqguiclients.h"
#include "KonqMainWindowAdaptor.h"
#include "KonquerorAdaptor.h"
#include "konqview.h"
#include "konqrun.h"
#include "konqmisc.h"
#include "konqviewmanager.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqactions.h"
#include "konqsettingsxt.h"
#include "konqextensionmanager.h"
#include "konqueror_interface.h"
#include "delayedinitializer.h"
#include "konqextendedbookmarkowner.h"
#include "konqframevisitor.h"
#include "konqbookmarkbar.h"
#include "konqundomanager.h"
#include "konqhistorydialog.h"
#include <config-konqueror.h>
#include <kstringhandler.h>

#include <konq_events.h>
#include <konqpixmapprovider.h>
#include <konq_operations.h>
#include <kbookmarkmanager.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <kzip.h>
#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <kde_file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QtCore/QFile>
#include <QClipboard>
#include <QtCore/QArgument>
#include <QLayout>
#include <QStackedWidget>
#include <QtCore/QFileInfo>
#ifdef Q_WS_X11
#include <QX11Info>
#endif
#include <QtCore/QEvent>
#include <QKeyEvent>
#include <QtCore/QByteRef>
#include <QtCore/QList>
#include <QPixmap>
#include <QLineEdit>

#include <kaboutdata.h>
#include <ktoolbar.h>
#include <konqbookmarkmenu.h>
#include <kcmultidialog.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kedittoolbar.h>
#include <klocalizedstring.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <knewfilemenu.h>
#include <konq_popupmenu.h>
#include "konqsettings.h"
#include "konqanimatedlogo_p.h"
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kstandardshortcut.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <ksycoca.h>
#include <ktemporaryfile.h>
#include <ktogglefullscreenaction.h>
#include <ktoolbarpopupaction.h>
#include <kurlcompletion.h>
#include <kurlrequesterdialog.h>
#include <kurlrequester.h>
#include <kmimetypetrader.h>
#include <kwindowsystem.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kprocess.h>
#include <kio/scheduler.h>
#include <kio/netaccess.h>
#include <kparts/browseropenorsavequestion.h>
#include <kacceleratormanager.h>
#include <kuser.h>
#include <kxmlguifactory.h>
#include <netwm.h>
#include <sonnet/configdialog.h>
#ifdef KDE_MALLINFO_MALLOC
#include <malloc.h>
#endif

#include <sys/time.h>
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#endif
#include <kauthorized.h>
#include <ktoolinvocation.h>
#include <QtDBus/QtDBus>
#include <kconfiggroup.h>

template class QList<QPixmap*>;
template class QList<KToggleAction*>;

KBookmarkManager* s_bookmarkManager = 0;
QList<KonqMainWindow*> *KonqMainWindow::s_lstViews = 0;
KConfig * KonqMainWindow::s_comboConfig = 0;
KCompletion * KonqMainWindow::s_pCompletion = 0;

bool KonqMainWindow::s_preloaded = false;
KonqMainWindow* KonqMainWindow::s_preloadedWindow = 0;
static qint64 s_initialMemoryUsage = -1;
static time_t s_startupTime;
static int s_preloadUsageCount;

KonqOpenURLRequest KonqOpenURLRequest::null;

static qint64 current_memory_usage( int* limit = NULL );

static unsigned short int s_closedItemsListLength = 10;
static unsigned long s_konqMainWindowInstancesCount = 0;

static void raiseWindow(KonqMainWindow* window)
{
    if (!window)
        return;

    if (window->isMinimized()) {
        KWindowSystem::unminimizeWindow(window->winId());
    }
    window->activateWindow();
    window->raise();
}

KonqExtendedBookmarkOwner::KonqExtendedBookmarkOwner(KonqMainWindow *w)
{
   m_pKonqMainWindow = w;
}

KonqMainWindow::KonqMainWindow( const KUrl &initialURL, const QString& xmluiFile)
    : KParts::MainWindow()
    , m_paClosedItems(0)
    , m_fullyConstructed(false)
    , m_bLocationBarConnected(false)
    , m_bURLEnterLock(false)
    , m_urlCompletionStarted(false)
    , m_prevMenuBarVisible(true)
    , m_goBuffer(0)
    , m_pBookmarkMenu(0)
    , m_configureDialog(0)
    , m_pURLCompletion(0)
    , m_isPopupWithProxyWindow(false)
{
  incInstancesCount();
  setPreloadedFlag( false );

  if ( !s_lstViews )
    s_lstViews = new QList<KonqMainWindow*>;

  s_lstViews->append( this );

  KonqMouseEventFilter::self(); // create it

  m_pChildFrame = 0;
  m_pActiveChild = 0;
  m_workingTab = 0;
  (void) new KonqMainWindowAdaptor( this );
  m_paBookmarkBar = 0;

  m_viewModesGroup = new QActionGroup(this);
  m_viewModesGroup->setExclusive(true);
  connect(m_viewModesGroup, SIGNAL(triggered(QAction*)),
          this, SLOT(slotViewModeTriggered(QAction*)),
          Qt::QueuedConnection); // Queued so that we don't delete the action from the code that triggered it.

    // This has to be called before any action is created for this mainwindow
    setComponentData(KGlobal::mainComponent(), false /*don't load plugins yet*/);

  m_pViewManager = new KonqViewManager( this );

  m_viewModeMenu = 0;
  m_openWithMenu = 0;
  m_paCopyFiles = 0;
  m_paMoveFiles = 0;
  m_bookmarkBarInitialized = false;

  m_toggleViewGUIClient = new ToggleViewGUIClient( this );

  m_pBookmarksOwner = new KonqExtendedBookmarkOwner(this);

  // init history-manager, load history, get completion object
  if ( !s_pCompletion ) {
      s_bookmarkManager = KBookmarkManager::userBookmarksManager();

      // let the KBookmarkManager know that we are a browser, equals to "keditbookmarks --browser"
      s_bookmarkManager->setEditorOptions("konqueror", true);

      KonqHistoryManager* mgr = new KonqHistoryManager(s_bookmarkManager);
      s_pCompletion = mgr->completionObject();

    // setup the completion object before createGUI(), so that the combo
    // picks up the correct mode from the HistoryManager (in slotComboPlugged)
    int mode = KonqSettings::settingsCompletionMode();
    s_pCompletion->setCompletionMode( static_cast<KGlobalSettings::Completion>(mode) );
  }
  connect(KParts::HistoryProvider::self(), SIGNAL(cleared()), SLOT(slotClearComboHistory()));

  KonqPixmapProvider *prov = KonqPixmapProvider::self();
  if ( !s_comboConfig ) {
      s_comboConfig = new KConfig("konq_history", KConfig::NoGlobals);
      KonqCombo::setConfig( s_comboConfig );
      KConfigGroup locationBarGroup( s_comboConfig, "Location Bar" );
      prov->load( locationBarGroup, "ComboIconCache" );
  }

  connect( prov, SIGNAL(changed()), SLOT(slotIconsChanged()) );

  m_pUndoManager = new KonqUndoManager(this);
  connect( m_pUndoManager, SIGNAL(undoAvailable(bool)),
           this, SLOT(slotUndoAvailable(bool)) );

  initCombo();
  initActions();

  connect( KGlobalSettings::self(), SIGNAL(kdisplayFontChanged()), SLOT(slotReconfigure()));

  //load the xmlui file specified in the profile or the default konqueror.rc
  setXMLFile( KonqViewManager::normalizedXMLFileName(xmluiFile) );

  setStandardToolBarMenuEnabled( true );

  createGUI( 0 );

  m_combo->setParent( toolBar("locationToolBar") );
  m_combo->setFont( KGlobalSettings::generalFont() );
  m_combo->show();

  checkDisableClearButton();

  connect(toolBarMenuAction(),SIGNAL(triggered()),this,SLOT(slotForceSaveMainWindowSettings()) );

  if ( !m_toggleViewGUIClient->empty() )
    plugActionList( QLatin1String( "toggleview" ), m_toggleViewGUIClient->actions() );
  else
  {
    delete m_toggleViewGUIClient;
    m_toggleViewGUIClient = 0;
  }

  m_bNeedApplyKonqMainWindowSettings = true;

  if ( !initialURL.isEmpty() ) {
      openFilteredUrl( initialURL.url() );
  } else {
      // silent
      m_bNeedApplyKonqMainWindowSettings = false;
  }

    if ( !initialGeometrySet() )
        resize( 700, 480 );

    //kDebug() << this << "done";

  if( s_initialMemoryUsage == -1 )
  {
      s_initialMemoryUsage = current_memory_usage();
      s_startupTime = time( NULL );
      s_preloadUsageCount = 0;
  }
  KonqSessionManager::self();
  m_fullyConstructed = true;
}

KonqMainWindow::~KonqMainWindow()
{
    //kDebug() << this;

    delete m_pViewManager;
    m_pViewManager = 0;

    if (s_lstViews) {
        s_lstViews->removeAll(this);
        if (s_lstViews->isEmpty()) {
            delete s_lstViews;
            s_lstViews = 0;
        }
    }

    qDeleteAll(m_openWithActions);
    m_openWithActions.clear();

  delete m_pBookmarkMenu;
  delete m_paBookmarkBar;
  delete m_pBookmarksOwner;
  delete m_pURLCompletion;
  delete m_paClosedItems;

  if ( s_lstViews == 0 ) {
      delete s_comboConfig;
      s_comboConfig = 0;
  }

  delete m_configureDialog;
  m_configureDialog = 0;
  delete m_combo;
  m_combo = 0;
  delete m_locationLabel;
  m_locationLabel = 0;
  m_pUndoManager->disconnect();
  delete m_pUndoManager;
  decInstancesCount();

    //kDebug() << this << "done";
}

void KonqMainWindow::incInstancesCount()
{
    s_konqMainWindowInstancesCount++;
}

void KonqMainWindow::decInstancesCount()
{
    s_konqMainWindowInstancesCount--;
}

QWidget * KonqMainWindow::createContainer( QWidget *parent, int index, const QDomElement &element, QAction* &containerAction )
{
  QWidget *res = KParts::MainWindow::createContainer( parent, index, element, containerAction );

  static QString nameBookmarkBar = QLatin1String( "bookmarkToolBar" );
  static QString tagToolBar = QLatin1String( "ToolBar" );
  if ( res && (element.tagName() == tagToolBar) && (element.attribute( "name" ) == nameBookmarkBar) )
  {
    Q_ASSERT( ::qobject_cast<KToolBar*>( res ) );
    if (!KAuthorized::authorizeKAction("bookmarks")) {
        delete res;
        return 0;
    }

    if ( !m_bookmarkBarInitialized ) {
        // The actual menu needs a different action collection, so that the bookmarks
        // don't appear in kedittoolbar
        m_bookmarkBarInitialized = true;
        DelayedInitializer *initializer = new DelayedInitializer( QEvent::Show, res );
        connect( initializer, SIGNAL(initialize()), this, SLOT(initBookmarkBar()) );
    }
  }

  if (res && element.tagName() == QLatin1String("Menu")) {
      const QString& menuName = element.attribute("name");
      if (menuName == "edit" || menuName == "tools") {
          Q_ASSERT(qobject_cast<QMenu*>(res));
          KAcceleratorManager::manage(static_cast<QMenu *>(res));
      }
  }

  return res;
}

void KonqMainWindow::initBookmarkBar()
{
  KToolBar * bar = qFindChild<KToolBar *>( this, "bookmarkToolBar" );

  if (!bar) return;

  const bool wasVisible = bar->isVisible();

  delete m_paBookmarkBar;
  m_paBookmarkBar = new KBookmarkBar( s_bookmarkManager, m_pBookmarksOwner, bar, this );

  // hide if empty
  if (bar->actions().count() == 0 || !wasVisible)
     bar->hide();
}

void KonqMainWindow::removeContainer( QWidget *container, QWidget *parent, QDomElement &element, QAction* containerAction )
{
  static QString nameBookmarkBar = QLatin1String( "bookmarkToolBar" );
  static QString tagToolBar = QLatin1String( "ToolBar" );

  if ( element.tagName() == tagToolBar && element.attribute( "name" ) == nameBookmarkBar )
  {
    Q_ASSERT( ::qobject_cast<KToolBar*>( container ) );
    if (m_paBookmarkBar)
      m_paBookmarkBar->clear();
  }

  KParts::MainWindow::removeContainer( container, parent, element, containerAction );
}

// Detect a name filter (e.g. *.txt) in the url.
// Note that KShortURIFilter does the same, but we have no way of getting it from there
//
// Note: this removes the filter from the URL.
QString KonqMainWindow::detectNameFilter( KUrl & url )
{
    if ( !KProtocolManager::supportsListing(url) )
        return QString();

    // Look for wildcard selection
    QString nameFilter;
    QString path = url.path();
    int lastSlash = path.lastIndexOf( '/' );
    if ( lastSlash > -1 )
    {
        if ( !url.query().isEmpty() && lastSlash == path.length()-1 ) {  //  In /tmp/?foo, foo isn't a query
            path += url.query(); // includes the '?'
        }
        QString fileName = path.mid( lastSlash + 1 );
        if ( fileName.indexOf( '*' ) != -1 || fileName.indexOf( '[' ) != -1 || fileName.indexOf( '?' ) != -1 )
        {
            // Check that a file or dir with all the special chars in the filename doesn't exist
            // (NetAccess::exists has a fast path for local files)
            if (!KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, this)) {
                nameFilter = fileName;
                url.setFileName( QString() );
                url.setQuery( QString() );
                kDebug() << "Found wildcard. nameFilter=" << nameFilter << "  New url=" << url;
            }
        }
    }

    return nameFilter;
}

void KonqMainWindow::openFilteredUrl(const QString & url, const KonqOpenURLRequest & req)
{
    // Filter URL to build a correct one
    if (m_currentDir.isEmpty() && m_currentView)
       m_currentDir = m_currentView->url().path( KUrl::AddTrailingSlash );

    KUrl filteredURL ( KonqMisc::konqFilteredURL( this, url, m_currentDir ) );
    kDebug() << "url" << url << "filtered into" << filteredURL;

    if ( filteredURL.isEmpty() ) // initially empty, or error (e.g. ~unknown_user)
        return;

    m_currentDir.clear();

    openUrl(0, filteredURL, QString(), req);

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqViewManager::doSetActivePart
    if (m_currentView)
        m_currentView->setFocus();
}

void KonqMainWindow::openFilteredUrl(const QString & _url, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req( _url );
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;

    openFilteredUrl( _url, req );
}

void KonqMainWindow::openFilteredUrl(const QString & _url,  const QString& _mimeType, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req( _url );
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;
    req.args.setMimeType(_mimeType);

    openFilteredUrl( _url, req );
}

void KonqMainWindow::openUrl(KonqView *_view, const KUrl &_url,
                             const QString &_mimeType, const KonqOpenURLRequest& _req,
                             bool trustedSource)
{
#ifndef NDEBUG // needed for req.debug()
    kDebug() << "url=" << _url << "mimeType=" << _mimeType
                 << "_req=" << _req.debug() << "view=" << _view;
#endif

    // We like modifying args in this method :)
    KUrl url(_url);
    QString mimeType(_mimeType);
    KonqOpenURLRequest req(_req);

    if (mimeType.isEmpty())
        mimeType = req.args.mimeType();

    if (url.protocol() != QLatin1String("error")) {
        if (!url.isValid()) {
            // I think we can't really get here anymore; I tried and didn't succeed.
            // URL filtering catches this case before hand, and in cases without filtering
            // (e.g. HTML link), the url is empty here, not invalid.
            // But just to be safe, let's keep this code path, even if it can't show the typed string.
            url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, url.url() /*empty*/, url.url() /*empty*/);
        } else if (!KProtocolInfo::isKnownProtocol(url)) {
            url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_UNSUPPORTED_PROTOCOL, url.protocol(), url.url());
        }
    }

    if (url.url() == "about:blank" || url.protocol() == "error") {
        mimeType = "text/html";
    }

    const QString nameFilter = detectNameFilter(url);
    if (!nameFilter.isEmpty()) {
      req.nameFilter = nameFilter;
      url.setFileName( QString() );
    }

    QLineEdit* edit = comboEdit();
    if (edit) {
      edit->setModified(false);
    }

    KonqView *view = _view;

    // When clicking a 'follow active' view (e.g. view is the sidebar),
    // open the URL in the active view
    if (view && view->isFollowActive())
        view = m_currentView;

    if (!view && !req.browserArgs.newTab())
        view = m_currentView; /* Note, this can be 0, e.g. on startup */
    else if (!view && req.browserArgs.newTab()) {

        // The URL should be opened in a new tab. Let's create the tab right away,
        // it gives faster user feedback (#163628). For a short while (kde-4.1-beta1)
        // I removed this entire block so that we wouldn't end up with a useless tab when
        // launching an external application for this mimetype. But user feedback
        // in all cases is more important than empty tabs in some cases.
        view = m_pViewManager->addTab("text/html",
                                      QString(),
                                      false,
                                      req.openAfterCurrentPage);
        if (view) {
            view->setCaption(i18nc("@title:tab", "Loading..."));
            view->setLocationBarURL(_url);
            if (!req.browserArgs.frameName.isEmpty())
                view->setViewName(req.browserArgs.frameName); // #44961

            if (req.newTabInFront)
                m_pViewManager->showTab(view);

            updateViewActions(); //A new tab created -- we may need to enable the "remove tab" button (#56318)
        } else {
            req.browserArgs.setNewTab(false);
        }
    }

  const QString oldLocationBarURL = locationBarURL();
  if ( view )
  {
    if ( view == m_currentView )
    {
      //will do all the stuff below plus GUI stuff
      abortLoading();
    }
    else
    {
      view->stop();
      // Don't change location bar if not current view
    }
  }

  // Fast mode for local files: do the stat ourselves instead of letting KRun do it.
  if ( mimeType.isEmpty() && url.isLocalFile() )
  {
    KDE_struct_stat buff;
    if ( KDE::stat( url.toLocalFile(), &buff ) != -1 )
        mimeType = KMimeType::findByUrl( url, buff.st_mode )->name();
  }

    if (url.isLocalFile()) {
        // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
        // zip:/<path>/ when clicking on a zip file, etc.
        // The .protocol file specifies the mimetype that the kioslave handles.
        // Note that we don't use mimetype inheritance since we don't want to
        // open OpenDocument files as zip folders...
        // Also note that we do this here and not in openView anymore,
        // because in the case of foo.bz2 we don't know the final mimetype, we need a konqrun...
        const QString protocol = KProtocolManager::protocolForArchiveMimetype(mimeType);
        if (!protocol.isEmpty() && KonqFMSettings::settings()->shouldEmbed(mimeType)) {
            url.setProtocol( protocol );
            if (mimeType == "application/x-webarchive") {
                url.addPath("index.html");
                mimeType = "text/html";
            } else {
                if (KProtocolManager::outputType(url) == KProtocolInfo::T_FILESYSTEM) {
                    url.adjustPath(KUrl::AddTrailingSlash);
                    mimeType = "inode/directory";
                } else
                    mimeType.clear();
            }
        }

        // Redirect to the url in Type=Link desktop files
        if (mimeType == "application/x-desktop") {
            KDesktopFile df(url.toLocalFile());
            if (df.hasLinkType()) {
                url = df.readUrl();
                mimeType.clear(); // to be determined again
            }
        }
    }


    //kDebug() << "trying openView for" << url << "( mimeType" << mimeType << ")";
  if ( ( !mimeType.isEmpty() && mimeType != "application/octet-stream") ||
       url.url() == "about:" || url.url().startsWith("about:konqueror") || url.url() == "about:plugins" )
  {
    KService::Ptr offer = KMimeTypeTrader::self()->preferredService(mimeType, "Application");
    const bool associatedAppIsKonqueror = isMimeTypeAssociatedWithSelf( mimeType, offer );
    // If the associated app is konqueror itself, then make sure we try to embed before bailing out.
    if ( associatedAppIsKonqueror )
      req.forceAutoEmbed = true;

    // Built-in view ?
    if ( !openView( mimeType, url, view /* can be 0 */, req ) )
    {
        //kDebug() << "openView returned false";
        // Are we following another view ? Then forget about this URL. Otherwise fire app.
        if ( !req.followMode )
        {
            //kDebug() << "we were not following. Fire app.";
            // The logic below is similar to BrowserRun::handleNonEmbeddable(),
            // but we don't have a BrowserRun instance here, and since it uses
            // some virtual methods [like save, for KHTMLRun], we can't just
            // move all the logic to static methods... catch 22...

            if ( !url.isLocalFile() && !trustedSource && KonqRun::isTextExecutable( mimeType ) )
                mimeType = "text/plain"; // view, don't execute
            // Remote URL: save or open ?
            QString protClass = KProtocolInfo::protocolClass(url.protocol());
            bool open = url.isLocalFile() || protClass==":local" || KProtocolInfo::isHelperProtocol(url);
            if ( !open ) {
                KParts::BrowserOpenOrSaveQuestion dlg(this, url, mimeType);
                dlg.setFeatures(KParts::BrowserOpenOrSaveQuestion::ServiceSelection);
                const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askOpenOrSave();
                if (res == KParts::BrowserOpenOrSaveQuestion::Save)
                    KParts::BrowserRun::saveUrl( url, QString(), this, req.args );
                open = (res == KParts::BrowserOpenOrSaveQuestion::Open);
                if (open) {
                    offer = dlg.selectedService();
                }
            }
            if ( open )
            {
                if ( associatedAppIsKonqueror && refuseExecutingKonqueror(mimeType) )
                    return;
                KUrl::List lst;
                lst.append(url);
                //kDebug() << "Got offer" << (offer ? offer->name() : QString("0"));
                const bool allowExecution = trustedSource || KParts::BrowserRun::allowExecution( mimeType, url );
                if ( allowExecution )
                {
                    const bool isExecutable = KonqRun::isExecutable( mimeType );
                    // Open with no offer means the user clicked on "Open With..." button.
                    if (!offer && !isExecutable) {
                        (void) KRun::displayOpenWithDialog(lst, this);
                    } else if (isExecutable || !KRun::run( *offer, lst, this ) ) {
                        setLocationBarURL( oldLocationBarURL ); // Revert to previous locationbar URL
                        (void)new KRun( url, this );
                    }
                }
            }
        }
    }
  }
  else // no known mimeType, use KonqRun
  {
      bool earlySetLocationBarURL = false;
      if (!view && !m_currentView) // no view yet, e.g. starting with url as argument
          earlySetLocationBarURL = true;
      else if (view == m_currentView && view->url().isEmpty()) // opening in current view
          earlySetLocationBarURL = true;
      if (req.browserArgs.newTab()) // it's going into a new tab anyway
          earlySetLocationBarURL = false;
      if (earlySetLocationBarURL) {
          // Show it for now in the location bar, but we'll need to store it in the view
          // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
          kDebug() << "url=" << url;
          setLocationBarURL( url );
      }

      kDebug() << "Creating new konqrun for" << url << "req.typedUrl=" << req.typedUrl;

      KonqRun * run = new KonqRun( this, view /* can be 0 */, url, req, trustedSource );

      // Never start in external browser
      run->setEnableExternalBrowser(false);

      if ( view )
        view->setRun( run );

      if ( view == m_currentView )
        startAnimation();

      connect( run, SIGNAL(finished()), this, SLOT(slotRunFinished()) );
  }
}

// When opening a new view, for @p mimeType, prefer the part used in @p currentView, if possible.
// Testcase: window.open after manually switching to another web engine, and with "open popups in tabs" off.
static QString preferredService(KonqView* currentView, const QString& mimeType)
{
    if (currentView && !mimeType.isEmpty() && currentView->supportsMimeType(mimeType)) {
        return currentView->service()->desktopEntryName();
    }
    return QString();
}

bool KonqMainWindow::openView( QString mimeType, const KUrl &_url, KonqView *childView, const KonqOpenURLRequest& req )
{
  // Second argument is referring URL
  if ( !KAuthorized::authorizeUrlAction("open", childView ? childView->url() : KUrl(), _url) )
  {
     QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, _url.prettyUrl());
     KMessageBox::queuedMessageBox( this, KMessageBox::Error, msg );
     return true; // Nothing else to do.
  }

  if ( KonqRun::isExecutable( mimeType ) )
     return false; // execute, don't open
  // Contract: the caller of this method should ensure the view is stopped first.

#ifndef NDEBUG
  kDebug() << mimeType << _url << "childView=" << childView << "req:" << req.debug();
#endif
  bool bOthersFollowed = false;

  if ( childView )
  {
    // If we're not already following another view (and if we are not reloading)
    if ( !req.followMode && !req.args.reload() && !m_pViewManager->isLoadingProfile() )
    {
      // When clicking a 'follow active' view (e.g. childView is the sidebar),
      // open the URL in the active view
      // (it won't do anything itself, since it's locked to its location)
      if ( childView->isFollowActive() && childView != m_currentView )
      {
        abortLoading();
        setLocationBarURL( _url );
        KonqOpenURLRequest newreq;
        newreq.forceAutoEmbed = true;
        newreq.followMode = true;
        newreq.args = req.args;
        newreq.browserArgs = req.browserArgs;
        bOthersFollowed = openView( mimeType, _url, m_currentView, newreq );
      }
      // "link views" feature, and "sidebar follows active view" feature
      bOthersFollowed = makeViewsFollow(_url, req.args, req.browserArgs, mimeType, childView) || bOthersFollowed;
    }
    if ( childView->isLockedLocation() && !req.args.reload() /* allow to reload a locked view*/ )
      return bOthersFollowed;
  }

    KUrl url(_url);

    // In case we open an index.html, we want the location bar
    // to still display the original URL (so that 'up' uses that URL,
    // and since that's what the user entered).
    // changePart will take care of setting and storing that url.
    QString originalURL = url.pathOrUrl();
    if (!req.nameFilter.isEmpty()) { // keep filter in location bar
        if (!originalURL.endsWith('/'))
            originalURL += '/';
        originalURL += req.nameFilter;
    }

    QString serviceName = req.serviceName; // default: none provided
    const QString urlStr = url.url();
    if ( urlStr == "about:" || urlStr.startsWith("about:konqueror") || urlStr == "about:plugins" ) {
        mimeType = "KonqAboutPage"; // not KParts/ReadOnlyPart, it fills the Location menu ! :)
        serviceName = "konq_aboutpage";
        originalURL = req.typedUrl.isEmpty() ? QString() : req.typedUrl;
    }
    else if ( urlStr == "about:blank" && req.typedUrl.isEmpty() ) {
        originalURL.clear();
    }


    bool forceAutoEmbed = req.forceAutoEmbed || req.userRequestedReload;
    if (!req.typedUrl.isEmpty()) // the user _typed_ the URL, he wants it in Konq.
        forceAutoEmbed = true;
    if (url.protocol() == "about" || url.protocol() == "error")
        forceAutoEmbed = true;
    // Related to KonqFactory::createView
    if (!forceAutoEmbed && !KonqFMSettings::settings()->shouldEmbed(mimeType)) {
        kDebug() << "KonqFMSettings says: don't embed this servicetype";
        return false;
    }
    // Do we even have a part to embed? Otherwise don't ask, since we'd ask twice.
    if (!forceAutoEmbed) {
        KService::List partServiceOffers;
        KonqFactory::getOffers(mimeType, &partServiceOffers);
        if (partServiceOffers.isEmpty()) {
            kDebug() << "No part available for" << mimeType;
            return false;
        }
    }

    // If the protocol doesn't support writing (e.g. HTTP) then we might want to save instead of just embedding.
    // So (if embedding would succeed, hence the checks above) we ask the user
    // Otherwise the user will get asked 'open or save' in openUrl anyway.
    if (!forceAutoEmbed && !KProtocolManager::supportsWriting(url)) {
        QString suggestedFileName;
        KonqRun* run = childView ? childView->run() : 0;
        int attachment = 0;
        if (run) {
            suggestedFileName = run->suggestedFileName();
            attachment = (run->serverSuggestsSave()) ? KParts::BrowserRun::AttachmentDisposition : KParts::BrowserRun::InlineDisposition;
        }

        KParts::BrowserOpenOrSaveQuestion dlg(this, url, mimeType);
        dlg.setSuggestedFileName(suggestedFileName);
        const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askEmbedOrSave(attachment);
        if (res == KParts::BrowserOpenOrSaveQuestion::Embed)
            forceAutoEmbed = true;
        else if (res == KParts::BrowserOpenOrSaveQuestion::Cancel)
            return true; // handled, don't do anything else
        else { // Save
            KParts::BrowserRun::saveUrl(url, suggestedFileName, this, req.args);
            return true; // handled
        }
    }

  bool ok = true;
  if ( !childView )
  {
      if (req.browserArgs.newTab())
      {
          KonqFrameTabs* tabContainer = m_pViewManager->tabContainer();
          int index = tabContainer->currentIndex();
          childView = m_pViewManager->addTab( mimeType, serviceName, false, req.openAfterCurrentPage );

          if (req.newTabInFront && childView)
          {
              if ( req.openAfterCurrentPage )
                  tabContainer->setCurrentIndex( index + 1 );
              else
                  tabContainer->setCurrentIndex( tabContainer->count()-1 );
          }
      }

      else
      {
        // Create a new view
        // createFirstView always uses force auto-embed even if user setting is "separate viewer",
        // since this window has no view yet - we don't want to keep an empty mainwindow.
        // This can happen with e.g. application/pdf from a target="_blank" link, or window.open.
        childView = m_pViewManager->createFirstView( mimeType, serviceName );

        if ( childView )
        {
            enableAllActions( true );
            m_currentView = childView;
        }
      }

      if ( !childView )
          return false; // It didn't work out.

      childView->setViewName( m_initialFrameName.isEmpty() ? req.browserArgs.frameName : m_initialFrameName );
      m_initialFrameName.clear();
  }
  else // We know the child view
  {
      if ( !childView->isLockedViewMode() ) {
          if ( ok ) {

              // When typing a new URL, the current context doesn't matter anymore
              // -> select the preferred part for a given mimetype (even if the current part can handle this mimetype).
              // This fixes the "get katepart and then type a website URL -> loaded into katepart" problem
              // (first fixed in r168902 from 2002!, see also unittest KonqHtmlTest::textThenHtml())

              if (!req.typedUrl.isEmpty() || !serviceName.isEmpty()) {
                  if (childView->isLoading()) { // Stop the view first, #282641.
                      childView->stop();
                  }
                  ok = childView->changePart( mimeType, serviceName, forceAutoEmbed );
              } else {
                  ok = childView->ensureViewSupports( mimeType, forceAutoEmbed );
              }
          }
      }
  }

  if (ok)
  {
      //kDebug() << "req.nameFilter= " << req.nameFilter;
      //kDebug() << "req.typedUrl= " << req.typedUrl;
      //kDebug() << "Browser extension? " << (childView->browserExtension() ? "YES" : "NO");
      //kDebug() << "Referrer: " << req.args.metaData()["referrer"];
      childView->setTypedURL( req.typedUrl );
      if ( childView->part() )
          childView->part()->setArguments( req.args );
      if ( childView->browserExtension() )
          childView->browserExtension()->setBrowserArguments( req.browserArgs );

      // see dolphinpart
      childView->part()->setProperty( "filesToSelect", KUrl::List(req.filesToSelect) );

      if ( !url.isEmpty() )
          childView->openUrl( url, originalURL, req.nameFilter, req.tempFile );
  }
    //kDebug() << "ok=" << ok << "bOthersFollowed=" << bOthersFollowed
    //             << "returning" << (ok || bOthersFollowed);
    return ok || bOthersFollowed;
}

static KonqView * findChildView( KParts::ReadOnlyPart *callingPart, const QString &name, KonqMainWindow *&mainWindow, KParts::BrowserHostExtension *&hostExtension, KParts::ReadOnlyPart **part )
{
    if ( !KonqMainWindow::mainWindowList() )
        return 0;

    foreach ( KonqMainWindow* window, *KonqMainWindow::mainWindowList() ) {
        KonqView *res = window->childView( callingPart, name, hostExtension, part );
        if ( res ) {
            mainWindow = window;
            return res;
        }
    }

    return 0;
}

void KonqMainWindow::slotOpenURLRequest( const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs )
{
    //kDebug() << "frameName=" << browserArgs.frameName;

  KParts::ReadOnlyPart *callingPart = static_cast<KParts::ReadOnlyPart *>( sender()->parent() );
  QString frameName = browserArgs.frameName;

  if ( !frameName.isEmpty() )
  {
    static QString _top = QLatin1String( "_top" );
    static QString _self = QLatin1String( "_self" );
    static QString _parent = QLatin1String( "_parent" );
    static QString _blank = QLatin1String( "_blank" );

    if ( frameName.toLower() == _blank )
    {
      KonqMainWindow *mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);
      mainWindow->slotCreateNewWindow( url, args, browserArgs );
      if (m_isPopupWithProxyWindow) {
          raiseWindow(mainWindow);
      }
      return;
    }

    if ( frameName.toLower() != _top &&
         frameName.toLower() != _self &&
         frameName.toLower() != _parent )
    {
      KParts::BrowserHostExtension *hostExtension = 0;
      KonqView *view = childView( callingPart, frameName, hostExtension, 0 );
      if ( !view )
      {
        KonqMainWindow *mainWindow = 0;
        view = findChildView( callingPart, frameName, mainWindow, hostExtension, 0 );

        if ( !view || !mainWindow )
        {
          slotCreateNewWindow( url, args, browserArgs );
          return;
        }

        if ( hostExtension )
          hostExtension->openUrlInFrame( url, args, browserArgs );
        else
          mainWindow->openUrlRequestHelper( view, url, args, browserArgs );
        return;
      }

      if ( hostExtension )
        hostExtension->openUrlInFrame( url, args, browserArgs );
      else
        openUrlRequestHelper( view, url, args, browserArgs );
      return;
    }
  }

  KonqView *view = browserArgs.newTab() ? 0 : childView( callingPart );
  openUrlRequestHelper( view, url, args, browserArgs );
}

//Called by slotOpenURLRequest
void KonqMainWindow::openUrlRequestHelper( KonqView *childView, const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs )
{
    //kDebug() << "url=" << url;
    KonqOpenURLRequest req;
    req.args = args;
    req.browserArgs = browserArgs;
    openUrl(childView, url, args.mimeType(), req, browserArgs.trustedSource);
}

QObject *KonqMainWindow::lastFrame( KonqView *view )
{
  QObject *nextFrame, *viewFrame;
  nextFrame = view->frame();
  viewFrame = 0;
  while ( nextFrame != 0 && !::qobject_cast<QStackedWidget*>(nextFrame) ) {
    viewFrame = nextFrame;
    nextFrame = nextFrame->parent();
  }
  return nextFrame ? viewFrame : 0;
}

// Linked-views feature, plus "sidebar follows URL opened in the active view" feature
bool KonqMainWindow::makeViewsFollow( const KUrl & url,
                                      const KParts::OpenUrlArguments& args,
                                      const KParts::BrowserArguments &browserArgs,
                                      const QString & serviceType, KonqView * senderView )
{
  if ( !senderView->isLinkedView() && senderView != m_currentView )
    return false; // none of those features apply -> return

  bool res = false;
  //kDebug() << senderView->metaObject()->className() << "url=" << url << "serviceType=" << serviceType;
  KonqOpenURLRequest req;
  req.forceAutoEmbed = true;
  req.followMode = true;
  req.args = args;
  req.browserArgs = browserArgs;
  // We can't iterate over the map here, and openUrl for each, because the map can get modified
  // (e.g. by part changes). Better copy the views into a list.
  const QList<KonqView*> listViews = m_mapViews.values();

    QObject *senderFrame = lastFrame( senderView );

    foreach (KonqView * view, listViews)
    {
        if (view == senderView)
            continue;
        bool followed = false;
        // Views that should follow this URL as both views are linked
        if (view->isLinkedView() && senderView->isLinkedView()) {
            QObject *viewFrame = lastFrame(view);

            // Only views in the same tab of the sender will follow
            if (senderFrame && viewFrame && viewFrame != senderFrame)
                continue;

            kDebug() << "sending openUrl to view" << view->part()->metaObject()->className() << "url=" << url;

            // XXX duplicate code from ::openUrl
            if (view == m_currentView) {
                abortLoading();
                setLocationBarURL(url);
            } else {
                view->stop();
            }

            followed = openView(serviceType, url, view, req);
        } else {
            // Make the sidebar follow the URLs opened in the active view
            if (view->isFollowActive() && senderView == m_currentView) {
                followed = openView(serviceType, url, view, req);
            }
        }

        // Ignore return value if the view followed but doesn't really
        // show the file contents. We still want to see that
        // file, e.g. in a separate viewer.
        // This happens in views locked to a directory mode,
        // like sidebar and konsolepart (#52161).
        const bool ignore = view->isLockedViewMode() && view->showsDirectory();
        //kDebug() << "View " << view->service()->name()
        //              << " supports dirs: " << view->showsDirectory()
        //              << " is locked-view-mode:" << view->isLockedViewMode()
        //              << " ignore=" << ignore;
        if ( !ignore )
            res = followed || res;
    }

    return res;
}

void KonqMainWindow::abortLoading()
{
    if ( m_currentView ) {
        m_currentView->stop(); // will take care of the statusbar
        stopAnimation();
    }
}

// Are there any indications that this window has a strong popup
// nature and should therefore not be embedded into a tab?
static bool isPopupWindow( const KParts::WindowArgs &windowArgs )
{
    // ### other settings to respect?
    return windowArgs.x() != -1 || windowArgs.y() != -1 ||
        windowArgs.width() != -1 || windowArgs.height() != -1 ||
        !windowArgs.isMenuBarVisible() ||
        !windowArgs.toolBarsVisible() ||
        !windowArgs.isStatusBarVisible();
}

// This is called for the javascript window.open call.
// Also called for MMB on link, target="_blank" link, MMB on folder, etc.
void KonqMainWindow::slotCreateNewWindow( const KUrl &url,
                                          const KParts::OpenUrlArguments& args,
                                          const KParts::BrowserArguments &browserArgs,
                                          const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart **part )
{
    // NOTE: 'part' may be null

    kDebug() << "url=" << url << "args.mimeType()=" << args.mimeType()
                 << "browserArgs.frameName=" << browserArgs.frameName;

    // If we are a popup window, forward the request the proxy window.
    if (m_isPopupWithProxyWindow && m_popupProxyWindow) {
        m_popupProxyWindow->slotCreateNewWindow(url, args, browserArgs, windowArgs, part);
        raiseWindow(m_popupProxyWindow);
        return;
    }

    if ( part )
        *part = 0; // Make sure to be initialized in case of failure...

    KonqMainWindow *mainWindow = 0;
    if ( !browserArgs.frameName.isEmpty() && browserArgs.frameName.toLower() != "_blank" ) {
        KParts::BrowserHostExtension *hostExtension = 0;
        KParts::ReadOnlyPart *ro_part = 0;
        KParts::BrowserExtension *be = ::qobject_cast<KParts::BrowserExtension *>(sender());
        if (be)
            ro_part = ::qobject_cast<KParts::ReadOnlyPart *>(be->parent());
        if ( findChildView( ro_part, browserArgs.frameName, mainWindow, hostExtension, part ) ) {
            // Found a view. If url isn't empty, we should open it - but this never happens currently
            // findChildView put the resulting part in 'part', so we can just return now
            //kDebug() << "frame=" << browserArgs.frameName << "-> found part=" << part << part->name();
            return;
        }
    }

    bool createTab = browserArgs.newTab();
    if (!createTab && !browserArgs.forcesNewWindow() /* explicit "Open in New Window" action, e.g. on frame or history item */) {
        if (args.actionRequestedByUser()) { // MMB or some RMB popupmenu action
            createTab = KonqSettings::mmbOpensTab();
        } else { // Javascript popup
            createTab = KonqSettings::popupsWithinTabs() &&
                        !isPopupWindow( windowArgs );
        }
    }
    kDebug() << "createTab=" << createTab << "part=" << part;

    if ( createTab && !m_isPopupWithProxyWindow ) {

        bool newtabsinfront = KonqSettings::newTabsInFront();
        if ( windowArgs.lowerWindow() || (QApplication::keyboardModifiers() & Qt::ShiftModifier))
           newtabsinfront = !newtabsinfront;
        const bool aftercurrentpage = KonqSettings::openAfterCurrentPage();

        KonqOpenURLRequest req;
        req.args = args;
        req.browserArgs = browserArgs;
        // Can we use the standard way (openUrl), or do we need the part pointer immediately?
        if (!part) {
            req.browserArgs.setNewTab(true);
            req.forceAutoEmbed = true; // testcase: MMB on link-to-PDF, when pdf setting is "show file in external browser".
            req.newTabInFront = newtabsinfront;
            req.openAfterCurrentPage = aftercurrentpage;
            openUrl(0, url, args.mimeType(), req);
        } else {
            KonqView* newView = m_pViewManager->addTab("text/html", QString(), false, aftercurrentpage);
            if (newView == 0) return;

            if (newtabsinfront)
                m_pViewManager->showTab( newView );

            openUrl( newView, url.isEmpty() ? KUrl("about:blank") : url, QString(), req );
            newView->setViewName( browserArgs.frameName );

            *part = newView->part();
        }

        // Raise the current window if the request to create the tab came from a popup
        // window, e.g. clicking on links with target = "_blank" in popup windows.
        KParts::BrowserExtension* be = qobject_cast<KParts::BrowserExtension*>(sender());
        KonqView* view = (be ? childView (qobject_cast<KParts::ReadOnlyPart*>(be->parent())) : 0);
        KonqMainWindow* window = view ? view->mainWindow() : 0;
        if (window && window->m_isPopupWithProxyWindow && !m_isPopupWithProxyWindow) {
            raiseWindow(this);
        }

        return;
    }

    KonqOpenURLRequest req;
    req.args = args;
    req.browserArgs = browserArgs;
    req.browserArgs.setNewTab(false); // we got a new window, no need for a new tab in that window
    req.forceAutoEmbed = true;
    req.serviceName = preferredService(m_currentView, args.mimeType());

    // Pass the URL to createNewWindow so that it can select the right profile for it
    // Note that it's always empty in case of window.open, though.
    mainWindow = KonqMisc::createNewWindow(url, req, false /*do not open URL*/);
    mainWindow->resetAutoSaveSettings(); // Don't autosave

    // Do we know the mimetype? If not, go to generic openUrl which will use a KonqRun.
    if ( args.mimeType().isEmpty() ) {
      mainWindow->openUrl( 0, url, QString(), req );
    } else {
        if (!mainWindow->openView(args.mimeType(), url, m_currentView, req)) {
            // we have problems. abort.
            delete mainWindow;

            if (part)
              *part = 0;
            return;
        }
    }

    KonqView * view = 0;
    // cannot use activePart/currentView, because the activation through the partmanager
    // is delayed by a singleshot timer (see KonqViewManager::setActivePart)
    // ### TODO: not true anymore
    if ( mainWindow->viewMap().count() )
    {
      MapViews::ConstIterator it = mainWindow->viewMap().begin();
      view = it.value();

      if ( part )
        *part = it.key();
    }

    // activate the view now in order to make the menuBar() hide call work
    if ( part && *part ) {
       mainWindow->viewManager()->setActivePart(*part);
    }

#ifdef Q_WS_X11
    // WORKAROUND: Clear the window state information set by KMainWindow::restoreWindowSize
    // so that the size and location settings we set below always take effect.
    KWindowSystem::clearState(mainWindow->winId(), NET::Max);
#endif

    // process the window args
    const int xPos = ((windowArgs.x() == -1) ?  mainWindow->x() : windowArgs.x());
    const int yPos = ((windowArgs.y() == -1) ?  mainWindow->y() : windowArgs.y());
    const int width = ((windowArgs.width() == -1) ?  mainWindow->width() : windowArgs.width());
    const int height = ((windowArgs.height() == -1) ?  mainWindow->height() : windowArgs.height());

    mainWindow->move ( xPos, yPos );
    mainWindow->resize( width, height );

    // Make the window open properties configurable. This is equivalent to
    // Firefox's "dom.disable_window_open_feature.*" properties. For now
    // only LocationBar visibility is configurable.
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cfg (config, "DisableWindowOpenFeatures");

    if ( !windowArgs.isMenuBarVisible() )
    {
        mainWindow->menuBar()->hide();
        mainWindow->m_paShowMenuBar->setChecked( false );
    }

    if ( !windowArgs.toolBarsVisible() )
    {
        // For security reasons the address bar is NOT hidden by default. The
        // user can override the default setup by adding a config option
        // "LocationBar=false" to the [DisableWindowOpenFeatures] section of
        // konquerorrc.
        const bool showLocationBar = cfg.readEntry("LocationBar", true);
        KToolBar* locationToolBar = mainWindow->toolBar(QLatin1String("locationToolBar"));

        Q_FOREACH (KToolBar* bar, mainWindow->findChildren<KToolBar*>()) {
            if (bar != locationToolBar || !showLocationBar) {
                bar->hide();
            }
        }

        if (locationToolBar && showLocationBar && isPopupWindow(windowArgs)) {
            // Hide all the actions of the popup window
            KActionCollection* collection = mainWindow->actionCollection();
            for (int i = 0, count = collection->count(); i < count; ++i) {
                collection->action(i)->setVisible(false);
            }

            // Show only those actions that are allowed in a popup window
            static const char* const s_allowedActions[] = {
                "go_back", "go_forward", "go_up", "reload", "hard_reload",
                "stop", "cut", "copy", "paste", "print", "fullscreen",
                "add_bookmark", "new_window", 0 };
            for (int i = 0; s_allowedActions[i]; ++i) {
                if (QAction* action = collection->action(QLatin1String(s_allowedActions[i]))) {
                    action->setVisible(true);
                }
            }

            // Make only the address widget available in the location toolbar
            locationToolBar->clear();
            QAction* action = locationToolBar->addWidget(mainWindow->m_combo);
            action->setVisible(true);

            // Make the combo box non editable and clear it of previous history
            QLineEdit* edit = (mainWindow->m_combo ? mainWindow->m_combo->lineEdit() : 0);
            if (edit) {
                mainWindow->m_combo->clear();
                mainWindow->m_combo->setCompletionMode(KGlobalSettings::CompletionNone);
                edit->setReadOnly(true);
            }

            // Store the originating window as the popup's proxy window so that
            // new tab requests in the popup window are forwarded to it.
            mainWindow->m_popupProxyWindow = this;
            mainWindow->m_isPopupWithProxyWindow = true;
        }
    }

    if ( view ) {
        if ( !windowArgs.scrollBarsVisible() )
            view->disableScrolling();
        if ( !windowArgs.isStatusBarVisible() ) {
            view->frame()->statusbar()->hide();
            mainWindow->m_paShowStatusBar->setChecked( false );
        } else {
          mainWindow->m_paShowStatusBar->setChecked( true );
        }
    }

    if ( !windowArgs.isResizable() )
        // ### this doesn't seem to work :-(
        mainWindow->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );

// Trying to show the window initially behind the current window is a bit tricky,
// as this involves the window manager, which may see things differently.
// Many WMs raise and activate new windows, which means without WM support this won't work very
// well. If the WM has support for _NET_WM_USER_TIME, it will be just set to 0 (=don't focus on show),
// and the WM should take care of it itself.
    bool wm_usertime_support = false;

#ifdef Q_WS_X11
    Time saved_last_input_time = QX11Info::appUserTime();
    if ( windowArgs.lowerWindow() )
    {
        NETRootInfo wm_info( QX11Info::display(), NET::Supported );
        wm_usertime_support = wm_info.isSupported( NET::WM2UserTime );
        if( wm_usertime_support )
        {
        // *sigh*, and I thought nobody would need QWidget::dontFocusOnShow().
        // Avoid Qt's support for user time by setting it to 0, and
        // set the property ourselves.
            QX11Info::setAppUserTime( 0 );
            KWindowSystem::setUserTime( mainWindow->winId(), 0 );
        }
        // Put below the current window before showing, in case that actually works with the WM.
        // First do complete lower(), then stackUnder(), because the latter may not work with many WMs.
        mainWindow->lower();
        mainWindow->stackUnder( this );
    }

    mainWindow->show();

    if ( windowArgs.lowerWindow() )
    {
        QX11Info::setAppUserTime( saved_last_input_time );
        if( !wm_usertime_support )
        { // No WM support. Let's try ugly tricks.
            mainWindow->lower();
            mainWindow->stackUnder( this );
            if( this->isActiveWindow())
                this->activateWindow();
        }
    }
#else // Q_WS_X11
    mainWindow->show();
#endif

    if ( windowArgs.isFullScreen() )
        mainWindow->action( "fullscreen" )->trigger();
}

void KonqMainWindow::slotNewWindow()
{
  // Use profile from current window, if set
  QString profile = m_pViewManager->currentProfile();
  if ( profile.isEmpty() )
  {
    if ( m_currentView && m_currentView->url().protocol().startsWith( "http" ) )
       profile = QLatin1String("webbrowsing");
    else
       profile = QLatin1String("filemanagement");
  }
  KonqMainWindow* mainWin = KonqMisc::createBrowserWindowFromProfile(QString(), profile);
  if (mainWin)
      mainWin->show();
}

void KonqMainWindow::slotDuplicateWindow()
{
    m_pViewManager->duplicateWindow()->show();
}

void KonqMainWindow::slotSendURL()
{
  const KUrl::List lst = currentURLs();
  QString body;
  QString fileNameList;
  for ( KUrl::List::ConstIterator it = lst.begin() ; it != lst.end() ; ++it )
  {
    if ( !body.isEmpty() ) body += '\n';
    body += (*it).prettyUrl();
    if ( !fileNameList.isEmpty() ) fileNameList += ", ";
    fileNameList += (*it).fileName();
  }
  QString subject;
  if (m_currentView && !m_currentView->showsDirectory())
    subject = m_currentView->caption();
  else // directory view
      subject = fileNameList;
  KToolInvocation::invokeMailer(QString(),QString(),QString(),
                     subject, body);
}

void KonqMainWindow::slotSendFile()
{
  const KUrl::List lst = currentURLs();
  QStringList urls;
  QString fileNameList;
  for ( KUrl::List::ConstIterator it = lst.begin() ; it != lst.end() ; ++it )
  {
    if ( !fileNameList.isEmpty() ) fileNameList += ", ";
    if ( (*it).isLocalFile() && QFileInfo((*it).toLocalFile()).isDir() )
    {
        // Create a temp dir, so that we can put the ZIP file in it with a proper name
        QString zipFileName;
        {
          //TODO This should use KTempDir
          KTemporaryFile zipFile;
          zipFile.open();
          zipFileName = zipFile.fileName();
        }

        QDir().mkdir(zipFileName);
        zipFileName = zipFileName+'/'+(*it).fileName()+".zip";
        KZip zip( zipFileName );
        if ( !zip.open( QIODevice::WriteOnly ) )
            continue; // TODO error message
        zip.addLocalDirectory( (*it).path(), QString() );
        zip.close();
        fileNameList += (*it).fileName()+".zip";
        urls.append( zipFileName );
    }
    else
    {
        fileNameList += (*it).fileName();
        urls.append( (*it).url() );
    }
  }
  QString subject;
  if ( m_currentView && !m_currentView->showsDirectory())
    subject = m_currentView->caption();
  else
    subject = fileNameList;
  KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                     QString(), //body
                     QString(),
                     urls); // attachments
}

void KonqMainWindow::slotOpenLocation()
{
    focusLocationBar();
    QLineEdit* edit = comboEdit();
    if (edit) {
        edit->selectAll();
    }
}

void KonqMainWindow::slotOpenFile()
{
    KUrl currentUrl;
    if (m_currentView && m_currentView->url().isLocalFile())
        currentUrl = m_currentView->url();
    else
        currentUrl = KUrl::fromPath(QDir::homePath());

    KUrl url = KFileDialog::getOpenUrl(currentUrl, QString(), this);
    if (!url.isEmpty())
      openFilteredUrl( url.url().trimmed() );
}

void KonqMainWindow::slotIconsChanged()
{
    kDebug();
    if (m_combo)
        m_combo->updatePixmaps();
    m_pViewManager->updatePixmaps();
    updateWindowIcon();
}

void KonqMainWindow::slotOpenWith()
{
    if (!m_currentView) return;

    KUrl::List lst;
    lst.append(m_currentView->url());

    const QString serviceName = sender()->objectName();

    const KService::List offers = m_currentView->appServiceOffers();
    KService::List::ConstIterator it = offers.begin();
    const KService::List::ConstIterator end = offers.end();
    for (; it != end; ++it ) {
        if ((*it)->desktopEntryName() == serviceName) {
            KRun::run(**it, lst, this);
            return;
        }
    }
}

void KonqMainWindow::slotViewModeTriggered(QAction* action)
{
    if (!m_currentView) return;

    // Gather data from the action, since the action will be deleted by changePart
    QString modeName = action->objectName();
    Q_ASSERT(modeName.endsWith("-viewmode"));
    modeName.chop(9);
    const QString internalViewMode = action->data().toString();

    if ( m_currentView->service()->desktopEntryName() != modeName ) {
        m_currentView->stop();
        m_currentView->lockHistory();

        // Save those, because changePart will lose them
        KUrl url = m_currentView->url();
        QString locationBarURL = m_currentView->locationBarURL();
#if 0
        // Problem: dolphinpart doesn't currently implement it. But we don't need it that much
        // now that it's the main filemanagement part for all standard modes.
        KUrl::List filesToSelect = childView->part()->property("filesToSelect").value<KUrl::List>();
#endif

        m_currentView->changePart( m_currentView->serviceType(), modeName );
        KUrl locURL( locationBarURL );
        QString nameFilter = detectNameFilter( locURL );
#if 0
        childView->part()->setProperty( "filesToSelect", KUrl::List(req.filesToSelect) );
#endif
        m_currentView->openUrl( locURL, locationBarURL, nameFilter );
    }

    if (!internalViewMode.isEmpty() && internalViewMode != m_currentView->internalViewMode()) {
        m_currentView->setInternalViewMode(internalViewMode);
    }
}

void KonqMainWindow::slotLockView()
{
  if (!m_currentView) return;

  m_currentView->setLockedLocation( m_paLockView->isChecked() );
}

void KonqMainWindow::slotStop()
{
  abortLoading();
  if ( m_currentView )
  {
    m_currentView->frame()->statusbar()->message( i18n("Canceled.") );
  }
}

void KonqMainWindow::slotLinkView()
{
    if (!m_currentView) return;

    // Can't access this action in passive mode anyway
    Q_ASSERT(!m_currentView->isPassiveMode());
    const bool mode = !m_currentView->isLinkedView();

    const QList<KonqView*> linkableViews = KonqLinkableViewsCollector::collect(this);
    if (linkableViews.count() == 2) {
        // Exactly two linkable views : link both
        linkableViews.at(0)->setLinkedView( mode );
        linkableViews.at(1)->setLinkedView( mode );
    } else { // Normal case : just this view
        m_currentView->setLinkedView( mode );
    }
}

void KonqMainWindow::slotReload( KonqView* reloadView, bool softReload )
{
    if ( !reloadView )
        reloadView = m_currentView;

    if (!reloadView || (reloadView->url().isEmpty() && reloadView->locationBarURL().isEmpty()))
        return;

    if (reloadView->isModified()) {
        if (KMessageBox::warningContinueCancel( this,
            i18n("This page contains changes that have not been submitted.\nReloading the page will discard these changes."),
            i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"view-refresh"), KStandardGuiItem::cancel(), "discardchangesreload") != KMessageBox::Continue )
            return;
    }

    KonqOpenURLRequest req( reloadView->typedUrl() );
    req.userRequestedReload = true;
    if ( reloadView->prepareReload( req.args, req.browserArgs, softReload ) )
    {
        reloadView->lockHistory();
        // Reuse current servicetype for local files, but not for remote files (it could have changed, e.g. over HTTP)
        QString serviceType = reloadView->url().isLocalFile() ? reloadView->serviceType() : QString();
        // By using locationBarURL instead of url, we preserve name filters (#54687)
        KUrl reloadUrl = reloadView->locationBarURL();
        if (reloadUrl.isEmpty()) // e.g. initial screen
            reloadUrl = reloadView->url();
        openUrl(reloadView, reloadUrl, serviceType, req);
    }
}

void KonqMainWindow::slotForceReload()
{
    // A forced reload is simply a "hard" (i.e. - not soft!) reload.
    slotReload(0L /* Current view */, false /* Not softReload*/);
}

void KonqMainWindow::slotReloadPopup()
{
    KonqFrameBase* tab = m_pViewManager->tabContainer()->tabAt(m_workingTab);
    if (tab) {
        slotReload(tab->activeChildView());
    }
}

void KonqMainWindow::slotHome(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    const QString homeURL = QDir::homePath();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();

    if (modifiers & Qt::ShiftModifier)
        req.newTabInFront = !req.newTabInFront;

    if( modifiers & Qt::ControlModifier ) // Ctrl Left/MMB
	openFilteredUrl( homeURL, req);
    else if( buttons & Qt::MidButton )
    {
        if(KonqSettings::mmbOpensTab())
	    openFilteredUrl( homeURL, req);
	else
	{
	    const KUrl finalURL = KonqMisc::konqFilteredURL(this, homeURL);
	    KonqMainWindow* mw = KonqMisc::createNewWindow(finalURL);
            mw->show();
	}
    }
    else
	openFilteredUrl( homeURL, false );
}

void KonqMainWindow::slotHome()
{
    slotHome(Qt::LeftButton, Qt::NoModifier);
}

void KonqMainWindow::slotGoHistory()
{
    if (!m_historyDialog) {
        m_historyDialog = new KonqHistoryDialog(this);
        m_historyDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_historyDialog->setModal(false);
    }
    m_historyDialog->show();
}

void KonqMainWindow::slotConfigureExtensions()
{
    KonqExtensionManager extensionManager(this, this, m_currentView ? m_currentView->part() : 0);
    extensionManager.exec();
}

void KonqMainWindow::slotConfigure()
{
    if( !m_configureDialog )
    {
        m_configureDialog = new KCMultiDialog( this );
        m_configureDialog->setObjectName( QLatin1String( "configureDialog" ) );
        connect(m_configureDialog, SIGNAL(finished()), this, SLOT(slotConfigureDone()));

        //BEGIN SYNC with initActions()
        const char* const toplevelModules[]={
                "khtml_general",
#ifndef Q_WS_WIN
                "kcmkonqyperformance",
#endif
                "bookmarks"};
        for (uint i=0;i<sizeof(toplevelModules)/sizeof(char*);++i)
            if (KAuthorized::authorizeControlModule(toplevelModules[i]))
                m_configureDialog->addModule(KCModuleInfo(QString(toplevelModules[i])+".desktop"));


        if (KAuthorized::authorizeControlModule("filebehavior") )
        {
            KPageWidgetItem * fileManagementGroup = m_configureDialog->addModule("filebehavior");
            if (fileManagementGroup) {
                fileManagementGroup->setName(i18n("File Management"));
                const char* const fmModules[]={
                    "kcmdolphinviewmodes",
                    "kcmdolphinnavigation",
                    "kcmdolphinservices",
                    "kcmdolphingeneral",
                    "filetypes",
                    "kcmtrash"};
                for (uint i=0;i<sizeof(fmModules)/sizeof(char*);++i)
                    if (KAuthorized::authorizeControlModule(fmModules[i]))
                        m_configureDialog->addModule(KCModuleInfo(QString(fmModules[i])+".desktop"),fileManagementGroup);
            } else {
                kWarning() << "Unable to load the \"File Management\" configuration module";
            }
        }

        if (KAuthorized::authorizeControlModule("khtml_behavior"))
        {
            KPageWidgetItem * webGroup = m_configureDialog->addModule("khtml_behavior");
            if (webGroup) {
                webGroup->setName(i18n("Web Browsing"));

                const char* const webModules[]={
                    "khtml_appearance",
                    "khtml_filter",
                    "ebrowsing",
                    "cache",
                    "proxy",
                    "kcmhistory",
                    "cookies",
                    "crypto",
                    "useragent",
                    "khtml_java_js",
                    "khtml_plugins"};
                for (uint i=0;i<sizeof(webModules)/sizeof(char*);++i)
                    if (KAuthorized::authorizeControlModule(webModules[i]))
                        m_configureDialog->addModule(KCModuleInfo(QString(webModules[i])+".desktop"),webGroup);
            } else {
                kWarning() << "Unable to load the \"Web Browsing\" configuration module";
            }

        }
        //END SYNC with initActions()

    }

    m_configureDialog->show();

}

void KonqMainWindow::slotConfigureDone()
{
    // Cleanup the dialog so other instances can use it..
    if ( m_configureDialog )
    {
        m_configureDialog->deleteLater();
        m_configureDialog = 0;
    }
}

void KonqMainWindow::slotConfigureSpellChecking()
{
    Sonnet::ConfigDialog dialog( KGlobal::config().data(), this);
    dialog.setWindowIcon( KIcon( "konqueror" ));
    dialog.exec();
}

void KonqMainWindow::slotConfigureToolbars()
{
    slotForceSaveMainWindowSettings();
    KEditToolBar dlg(factory(), this);
    connect(&dlg,SIGNAL(newToolBarConfig()),this,SLOT(slotNewToolbarConfig()));
    connect(&dlg,SIGNAL(newToolBarConfig()),this,SLOT(initBookmarkBar()));
    dlg.exec();
    checkDisableClearButton();
}

void KonqMainWindow::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    if ( m_toggleViewGUIClient )
      plugActionList( QLatin1String( "toggleview" ), m_toggleViewGUIClient->actions() );
    if ( m_currentView && m_currentView->appServiceOffers().count() > 0 )
      plugActionList( "openwith", m_openWithActions );

    plugViewModeActions();

    KConfigGroup cg = KGlobal::config()->group( "KonqMainWindow" );
    applyMainWindowSettings( cg );
}

void KonqMainWindow::slotUndoAvailable( bool avail )
{
  m_paUndo->setEnabled( avail );
}

void KonqMainWindow::slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart )
{
  m_mapViews.remove( oldPart );
  m_mapViews.insert( newPart, childView );

  // Remove the old part, and add the new part to the manager
  const bool wasActive = m_pViewManager->activePart() == oldPart;

  m_pViewManager->replacePart( oldPart, newPart, false );

  // Set active immediately - but only if the old part was the active one (#67956)
  if (wasActive) {
      // Note: this makes the new part active... so it calls slotPartActivated
      m_pViewManager->setActivePart(newPart);
  }

  viewsChanged();
}


void KonqMainWindow::slotRunFinished()
{
  const KonqRun *run = static_cast<const KonqRun *>( sender() );

  if ( !run->mailtoURL().isEmpty() )
  {
      KToolInvocation::invokeMailer( run->mailtoURL() );
  }

  if ( run->hasError() ) { // we had an error
      QDBusMessage message = QDBusMessage::createSignal( KONQ_MAIN_PATH, "org.kde.Konqueror.Main", "removeFromCombo" );
      message << run->url().prettyUrl();
      QDBusConnection::sessionBus().send( message );
  }

  KonqView *childView = run->childView();

  // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
  if ( run->wasMimeTypeFound() && !run->hasError() )
  {

    // We do this here and not in the constructor, because
    // we are waiting for the first view to be set up before doing this...
    // Note: this is only used when konqueror is started from command line.....
    if ( m_bNeedApplyKonqMainWindowSettings )
    {
      m_bNeedApplyKonqMainWindowSettings = false; // only once
      applyKonqMainWindowSettings();
    }

    return;
  }

    // An error happened in KonqRun - stop wheel etc.

    if (childView) {
        childView->setLoading(false);

        if (childView == m_currentView) {
            stopAnimation();

            // Revert to working URL - unless the URL was typed manually
            if (run->typedUrl().isEmpty() && childView->currentHistoryEntry()) // not typed
                childView->setLocationBarURL(childView->currentHistoryEntry()->locationBarURL);
        }
    } else { // No view, e.g. empty webbrowsing profile
        stopAnimation();
    }
}

void KonqMainWindow::applyKonqMainWindowSettings()
{
  const QStringList toggableViewsShown = KonqSettings::toggableViewsShown();
  QStringList::ConstIterator togIt = toggableViewsShown.begin();
  QStringList::ConstIterator togEnd = toggableViewsShown.end();
  for ( ; togIt != togEnd ; ++togIt )
  {
    // Find the action by name
  //    KAction * act = m_toggleViewGUIClient->actionCollection()->action( (*togIt).toLatin1() );
    QAction *act = m_toggleViewGUIClient->action( *togIt );
    if ( act )
      act->trigger();
    else
      kWarning() << "Unknown toggable view in ToggableViewsShown " << *togIt ;
  }
}

void KonqMainWindow::slotSetStatusBarText( const QString & )
{
   // Reimplemented to disable KParts::MainWindow default behaviour
   // Does nothing here, see KonqFrame
}

void KonqMainWindow::slotViewCompleted( KonqView * view )
{
  Q_ASSERT( view );

  // Need to update the current working directory
  // of the completion object every time the user
  // changes the directory!! (DA)
  if( m_pURLCompletion )
  {
    KUrl u( view->locationBarURL() );
    if( u.isLocalFile() )
      m_pURLCompletion->setDir( u.toLocalFile() );
    else
      m_pURLCompletion->setDir( u.url() );  //needs work!! (DA)
  }
}

void KonqMainWindow::slotPartActivated(KParts::Part *part)
{
    kDebug() << part
               << (part && part->componentData().isValid() && part->componentData().aboutData() ? part->componentData().aboutData()->appName() : "");

  KonqView *newView = 0;
  KonqView *oldView = m_currentView;

    if (part) {
        newView = m_mapViews.value( static_cast<KParts::ReadOnlyPart *>(part) );
        Q_ASSERT(newView);
        if (newView->isPassiveMode()) {
      // Passive view. Don't connect anything, don't change m_currentView
      // Another view will become the current view very soon
            //kDebug() << "Passive mode - return";
      return;
    }
  }

  KParts::BrowserExtension *ext = 0;

    if (oldView) {
    ext = oldView->browserExtension();
        if (ext) {
      //kDebug() << "Disconnecting extension for view" << oldView;
      disconnectExtension( ext );
    }
  }

    kDebug() << "New current view" << newView;
    m_currentView = newView;
    if (newView) {
      m_paShowStatusBar->setChecked( newView->frame()->statusbar()->isVisible() );
    }

    if (!part) {
      //kDebug() << "No part activated - returning";
    unplugViewModeActions();
      createGUI(0);
      KParts::MainWindow::setCaption(QString());
    return;
  }

  ext = m_currentView->browserExtension();

    if (ext) {
        connectExtension(ext);
    } else {
    kDebug() << "No Browser Extension for the new part";
    // Disable all browser-extension actions

    KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
        const KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();
        for (; it != itEnd ; ++it) {
            QAction * act = actionCollection()->action(QString::fromLatin1(it.key()));
      Q_ASSERT(act);
      if (act)
        act->setEnabled( false );
    }

        if (m_paCopyFiles)
            m_paCopyFiles->setEnabled(false);
        if (m_paMoveFiles)
            m_paMoveFiles->setEnabled(false);
  }

    createGUI(part);

  // View-dependent GUI

  KParts::MainWindow::setCaption( KStringHandler::csqueeze( m_currentView->caption(), 128 ) );
    // This line causes #170470 when removing the current tab, because QTabBar
    // emits currentChanged before calling tabRemoved, so KTabWidget gets confused.
    // I don't see a need for it anyway...
    //m_currentView->frame()->setTitle(m_currentView->caption(), 0);

  updateOpenWithActions();
  updateViewActions(); // undo, lock, link and other view-dependent actions
  updateViewModeActions();

  if ( !m_currentView->showsDirectory() ) {

    m_paHome->setIcon( KIcon("user-home") );
    m_paHome->setText( i18n("Home Folder") );
    m_paHome->setStatusTip( i18n("Navigate to your 'Home Folder'") );
    m_paHome->setWhatsThis( i18n("Navigate to your local 'Home Folder'") );
  }

  m_currentView->frame()->statusbar()->updateActiveStatus();

  if ( oldView && oldView->frame() )
    oldView->frame()->statusbar()->updateActiveStatus();

  //kDebug() << "setting location bar url to"
  //         << m_currentView->locationBarURL() << "m_currentView=" << m_currentView;

  // Make sure the location bar gets updated when the view(tab) is changed.
  if (oldView != newView && m_combo) {
    m_combo->lineEdit()->setModified(false);
  }
  m_currentView->setLocationBarURL( m_currentView->locationBarURL() );

  updateToolBarActions();

  m_currentView->setActiveComponent();
}

void KonqMainWindow::insertChildView( KonqView *childView )
{
    //kDebug() << childView;
    m_mapViews.insert(childView->part(), childView);

    connect(childView, SIGNAL(viewCompleted(KonqView*)),
            this, SLOT(slotViewCompleted(KonqView*)));

    emit viewAdded(childView);
}

// Called by KonqViewManager, internal
void KonqMainWindow::removeChildView( KonqView *childView )
{
    //kDebug() << childView;

  disconnect( childView, SIGNAL(viewCompleted(KonqView*)),
              this, SLOT(slotViewCompleted(KonqView*)) );

#ifndef NDEBUG
    //dumpViewList();
#endif

  MapViews::Iterator it = m_mapViews.begin();
  const MapViews::Iterator end = m_mapViews.end();

  // find it in the map - can't use the key since childView->part() might be 0

  //kDebug() << "Searching map";

  while ( it != end && it.value() != childView )
      ++it;

  //kDebug() << "Verifying search results";

  if ( it == m_mapViews.end() )
  {
      kWarning() << "KonqMainWindow::removeChildView childView " << childView << " not in map !" ;
      return;
  }

  //kDebug() << "Removing view" << childView;

  m_mapViews.erase( it );

  emit viewRemoved( childView );

#ifndef NDEBUG
  //dumpViewList();
#endif

  // KonqViewManager takes care of m_currentView
}

void KonqMainWindow::linkableViewCountChanged()
{
    const QList<KonqView*> linkableViews = KonqLinkableViewsCollector::collect(this);
    const int lvc = linkableViews.count();
    m_paLinkView->setEnabled( lvc > 1 );
    // Only one view -> unlink it
    if (lvc == 1) {
        linkableViews.at(0)->setLinkedView( false );
    }
    m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewCountChanged()
{
    // This is called (by the view manager) when the number of views changes.
    linkableViewCountChanged();
    viewsChanged();
}

void KonqMainWindow::viewsChanged()
{
    // This is called when the number of views changes OR when
    // the type of some view changes.

    updateViewActions(); // undo, lock, link and other view-dependent actions
}

KonqView * KonqMainWindow::childView( KParts::ReadOnlyPart *view )
{
    return m_mapViews.value(view);
}

KonqView * KonqMainWindow::childView( KParts::ReadOnlyPart *callingPart, const QString &name, KParts::BrowserHostExtension *&hostExtension, KParts::ReadOnlyPart **part )
{
    //kDebug() << "this=" << this << "looking for" << name;
    QList<KonqView *> views = m_mapViews.values();
    KonqView* callingView = m_mapViews.value(callingPart);
    if (callingView) {
        // Move the callingView in front of the list, in case of duplicate frame names (#133967)
        if (views.removeAll(callingView))
            views.prepend(callingView);
    }

  Q_FOREACH(KonqView* view, views) {
    QString viewName = view->viewName();
    //kDebug() << "       - viewName=" << viewName
    //          << "frame names:" << view->frameNames();

    // First look for a hostextension containing this frame name
    KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject( view->part() );
    if ( ext )
    {
      ext = ext->findFrameParent(callingPart, name);
      kDebug() << "BrowserHostExtension found part" << ext;
      if (!ext)
         continue; // Don't use this window
    }

    if ( !viewName.isEmpty() && viewName == name )
    {
      kDebug() << "found existing view by name:" << view;
      hostExtension = 0;
      if ( part )
        *part = view->part();
      return view;
    }

//    KParts::BrowserHostExtension* ext = KonqView::hostExtension( view->part(), name );

    if ( ext )
    {
      const QList<KParts::ReadOnlyPart*> frames = ext->frames();
      QListIterator<KParts::ReadOnlyPart *> frameIt(frames);
      while (frameIt.hasNext())
      {
        KParts::ReadOnlyPart *item = frameIt.next();
        if ( item->objectName() == name )
        {
          kDebug() << "found a frame of name" << name << ":" << item;
          hostExtension = ext;
          if ( part )
            *part = item;
          return view;
        }
      }
    }
  }

  return 0;
}

int KonqMainWindow::activeViewsNotLockedCount() const
{
  int res = 0;
  MapViews::ConstIterator end = m_mapViews.constEnd();
  for (MapViews::ConstIterator it = m_mapViews.constBegin(); it != end; ++it ) {
      if ( !it.value()->isPassiveMode() && !it.value()->isLockedLocation() )
          ++res;
  }

  return res;
}

int KonqMainWindow::linkableViewsCount() const
{
    return KonqLinkableViewsCollector::collect(const_cast<KonqMainWindow*>(this)).count();
}

int KonqMainWindow::mainViewsCount() const
{
  int res = 0;
  MapViews::ConstIterator it = m_mapViews.constBegin();
  const MapViews::ConstIterator end = m_mapViews.constEnd();
  for (; it != end; ++it ) {
    if ( !it.value()->isPassiveMode() && !it.value()->isToggleView() ) {
      //kDebug() << res << it.value() << it.value()->part()->widget();
      ++res;
    }
  }

  return res;
}

void KonqMainWindow::slotURLEntered(const QString &text, Qt::KeyboardModifiers modifiers)
{
    if (m_bURLEnterLock || text.isEmpty())
        return;

    m_bURLEnterLock = true;

    if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::AltModifier)) {
        m_combo->setURL(m_currentView ? m_currentView->url().prettyUrl() : QString());
        const bool inNewTab = !m_isPopupWithProxyWindow; // do not open a new tab in popup window.
        openFilteredUrl(text.trimmed(), inNewTab);
    } else {
        openFilteredUrl(text.trimmed());
    }

    m_bURLEnterLock = false;
}

void KonqMainWindow::slotSplitViewHorizontal()
{
    if ( !m_currentView )
        return;
    KonqView* oldView = m_currentView;
    KonqView* newView = m_pViewManager->splitView(m_currentView, Qt::Horizontal);
    if (newView == 0)
        return;
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    openView(oldView->serviceType(), oldView->url(), newView, req);
}

void KonqMainWindow::slotSplitViewVertical()
{
    if ( !m_currentView )
        return;
    KonqView* oldView = m_currentView;
    KonqView* newView = m_pViewManager->splitView(m_currentView, Qt::Vertical);
    if (newView == 0)
        return;
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    openView(oldView->serviceType(), oldView->url(), newView, req);
}

void KonqMainWindow::slotAddTab()
{
    // we can hardcode text/html because this is what about:blank will use anyway
    KonqView* newView = m_pViewManager->addTab("text/html",
                                               QString(),
                                               false,
                                               KonqSettings::openAfterCurrentPage());
    if (!newView)
      return;

    openUrl( newView, KUrl("about:blank"), QString() );

    //HACK!! QTabBar likes to steal focus when changing widgets.  This can result
    //in a flicker since we don't want it to get focus we want the combo to get
    //or keep focus...
    // TODO: retest, and replace with the smaller hack from KTabWidget::moveTab
    QWidget *widget = newView->frame() && newView->frame()->part() ?
                      newView->frame()->part()->widget() : 0;
    QWidget* origFocusProxy = widget ? widget->focusProxy() : 0;
    if (widget)
        widget->setFocusProxy(m_combo);

    m_pViewManager->showTab( newView );

    if (widget)
        widget->setFocusProxy(origFocusProxy);

    m_workingTab = 0;
}

void KonqMainWindow::slotDuplicateTab()
{
    m_pViewManager->duplicateTab(m_pViewManager->tabContainer()->currentIndex(), KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotDuplicateTabPopup()
{
    m_pViewManager->duplicateTab(m_workingTab, KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotBreakOffTab()
{
    breakOffTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotBreakOffTabPopup()
{
    // Delay the call since it might delete the tabbar
    QMetaObject::invokeMethod(this, "breakOffTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::breakOffTab(int tabIndex)
{
    KonqFrameBase* tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab)
        return;
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    // TODO: Why do we warn about breaking off a modified tab, since it seems to show the unsubmitted form data just fine?
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if (KMessageBox::warningContinueCancel(
                this,
                i18n("This tab contains changes that have not been submitted.\nDetaching the tab will discard these changes."),
                i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"tab-detach"), KStandardGuiItem::cancel(), "discardchangesdetach") != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->breakOffTab(tabIndex, size());
    updateViewActions();
}

void KonqMainWindow::slotPopupNewWindow()
{
    KFileItemList::const_iterator it = m_popupItems.constBegin();
    const KFileItemList::const_iterator end = m_popupItems.constEnd();
    KonqOpenURLRequest req;
    req.args = m_popupUrlArgs;
    req.browserArgs = m_popupUrlBrowserArgs;
    for ( ; it != end; ++it ) {
      KonqMainWindow* mw = KonqMisc::createNewWindow((*it).targetUrl(), req);
      mw->show();
    }
}

void KonqMainWindow::slotPopupThisWindow()
{
    openUrl(0, m_popupItems.first().url());
}

void KonqMainWindow::slotPopupNewTab()
{
    if (m_isPopupWithProxyWindow && !m_popupProxyWindow) {
        slotPopupNewWindow();
        return;
    }
    bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    bool newTabsInFront = KonqSettings::newTabsInFront();

    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
      newTabsInFront = !newTabsInFront;

    popupNewTab(newTabsInFront, openAfterCurrentPage);
}

void KonqMainWindow::popupNewTab(bool infront, bool openAfterCurrentPage)
{
  KonqOpenURLRequest req;
  req.newTabInFront = false;
  req.forceAutoEmbed = true;
  req.openAfterCurrentPage = openAfterCurrentPage;
  req.args = m_popupUrlArgs;
  req.browserArgs = m_popupUrlBrowserArgs;
  req.browserArgs.setNewTab(true);

  KonqMainWindow* mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);

  for ( int i = 0; i < m_popupItems.count(); ++i )
  {
    if ( infront && i == m_popupItems.count()-1 )
    {
      req.newTabInFront = true;
    }
    mainWindow->openUrl( 0, m_popupItems[i].targetUrl(), QString(), req );
  }

  // Raise this window if the request to create the tab came from a popup window.
  if (m_isPopupWithProxyWindow) {
      raiseWindow(mainWindow);
  }
}

void KonqMainWindow::openMultiURL( const KUrl::List& url )
{
    KUrl::List::ConstIterator it = url.constBegin();
    const KUrl::List::ConstIterator end = url.constEnd();
    for (; it != end; ++it )
    {
        KonqView* newView = m_pViewManager->addTab("text/html");
        Q_ASSERT( newView );
        if (newView == 0) continue;
        openUrl( newView, *it, QString() );
        m_pViewManager->showTab( newView );
    }
}

void KonqMainWindow::slotRemoveView()
{
    if (!m_currentView) return;

    if (m_currentView->isModified()) {
      if ( KMessageBox::warningContinueCancel( this,
           i18n("This view contains changes that have not been submitted.\nClosing the view will discard these changes."),
           i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"view-close"), KStandardGuiItem::cancel(), "discardchangesclose") != KMessageBox::Continue )
        return;
    }

    // takes care of choosing the new active view
    m_pViewManager->removeView(m_currentView);
}

void KonqMainWindow::slotRemoveTab()
{
    removeTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveTabPopup()
{
    // Can't do immediately - may kill the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeTab(int tabIndex)
{
    KonqFrameBase* tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab)
        return;
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if ( KMessageBox::warningContinueCancel(
                 this,
                 i18n("This tab contains changes that have not been submitted.\nClosing the tab will discard these changes."),
                 i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"tab-close"), KStandardGuiItem::cancel(), "discardchangesclose") != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeTab(tab);
    updateViewActions();
}

void KonqMainWindow::slotRemoveOtherTabs()
{
    removeOtherTabs(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveOtherTabsPopup()
{
    // Can't do immediately - kills the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeOtherTabs", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeOtherTabs(int tabToKeep)
{
    if (KMessageBox::warningContinueCancel(
            this,
            i18n("Do you really want to close all other tabs?"),
            i18nc("@title:window", "Close Other Tabs Confirmation"), KGuiItem(i18n("Close &Other Tabs"),"tab-close-other"),
            KStandardGuiItem::cancel(), "CloseOtherTabConfirm") != KMessageBox::Continue)
        return;

    KonqFrameTabs* tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        if (tabIndex == tabToKeep) {
            continue;
        }
        KonqFrameBase* tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("This tab contains changes that have not been submitted.\nClosing other tabs will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"tab-close"),
                    KStandardGuiItem::cancel(), "discardchangescloseother") != KMessageBox::Continue) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeOtherTabs(tabToKeep);
    updateViewActions();
}

void KonqMainWindow::slotReloadAllTabs()
{
    KonqFrameTabs* tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        KonqFrameBase* tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(this,
                   i18n("This tab contains changes that have not been submitted.\nReloading all tabs will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"),
                    KGuiItem(i18n("&Discard Changes"), "view-refresh"),
                    KStandardGuiItem::cancel(), "discardchangesreload") != KMessageBox::Continue ) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->reloadAllTabs();
    updateViewActions();
}


void KonqMainWindow::slotActivateNextTab()
{
  m_pViewManager->activateNextTab();
}

void KonqMainWindow::slotActivatePrevTab()
{
  m_pViewManager->activatePrevTab();
}

void KonqMainWindow::slotActivateTab()
{
  m_pViewManager->activateTab( sender()->objectName().right( 2 ).toInt() -1 );
}

void KonqMainWindow::slotDumpDebugInfo()
{
#ifndef NDEBUG
  dumpViewList();
  m_pViewManager->printFullHierarchy();
#endif
}

bool KonqMainWindow::askForTarget(const KLocalizedString& text, KUrl& url)
{
   const KUrl initialUrl = (viewCount()==2) ? otherView(m_currentView)->url() : m_currentView->url();
   QString label = text.subs( m_currentView->url().pathOrUrl() ).toString();
   KUrlRequesterDialog dlg(initialUrl.pathOrUrl(), label, this);
   dlg.setCaption(i18nc("@title:window", "Enter Target"));
   dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly | KFile::Directory );
   if (dlg.exec())
   {
      url = dlg.selectedUrl();
      if ( url.isValid() )
        return true;
      else
      {
        KMessageBox::error( this, i18n("<qt><b>%1</b> is not valid</qt>", url.url()));
        return false;
      }
   }
   return false;
}

void KonqMainWindow::slotRequesterClicked( KUrlRequester *req )
{
    req->fileDialog()->setMode(KFile::Directory|KFile::ExistingOnly);
}

void KonqMainWindow::slotCopyFiles()
{
  KUrl dest;
  if (!askForTarget(ki18n("Copy selected files from %1 to:"), dest))
     return;

  KonqOperations::copy(this, KonqOperations::COPY, currentURLs(), dest);
}

void KonqMainWindow::slotMoveFiles()
{
  KUrl dest;
  if (!askForTarget(ki18n("Move selected files from %1 to:"), dest))
     return;

  KonqOperations::copy(this, KonqOperations::MOVE, currentURLs(), dest);
}

KUrl::List KonqMainWindow::currentURLs() const
{
  KUrl::List urls;
  if ( m_currentView ) {
    urls.append( m_currentView->url() );
    if (!m_currentView->selectedItems().isEmpty()) // Return list of selected items only if we have a selection
        urls = m_currentView->selectedItems().urlList();
  }
  return urls;
}

// Only valid if there are one or two views
KonqView * KonqMainWindow::otherView( KonqView * view ) const
{
  Q_ASSERT( viewCount() <= 2 );
  MapViews::ConstIterator it = m_mapViews.constBegin();
  if ( (*it) == view )
    ++it;
  if ( it != m_mapViews.constEnd() )
    return (*it);
  return 0;
}

void KonqMainWindow::slotSaveViewProfile()
{
    m_pViewManager->showProfileDlg( m_pViewManager->currentProfile() );
}

void KonqMainWindow::slotUpAboutToShow()
{
    if (!m_currentView) return;

    QMenu *popup = m_paUp->menu();
    popup->clear();

    int i = 0;

    // Use the location bar URL, because in case we display a index.html
    // we want to go up from the dir, not from the index.html
    KUrl u(m_currentView->locationBarURL());
    u = u.upUrl();
    while (u.hasPath()) {
        KAction* action = new KAction(KIcon(KonqPixmapProvider::self()->iconNameFor(u)),
                                      u.pathOrUrl(),
                                      popup);
        action->setData(u);
        popup->addAction(action);

        if (u.path() == "/" || ++i > 10)
            break;

        u = u.upUrl();
    }
}

void KonqMainWindow::slotUp(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    m_goMouseState = buttons;
    m_goKeyboardState = modifiers;
    QTimer::singleShot( 0, this, SLOT(slotUpDelayed()) );
}

void KonqMainWindow::slotUp()
{
    m_goMouseState = Qt::LeftButton;
    m_goKeyboardState = Qt::NoModifier;
    QTimer::singleShot( 0, this, SLOT(slotUpDelayed()) );
}

void KonqMainWindow::slotUpDelayed()
{
    if (!m_currentView) return;

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.forceAutoEmbed = true;

    req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    req.newTabInFront = KonqSettings::newTabsInFront();

    if (m_goKeyboardState & Qt::ShiftModifier)
        req.newTabInFront = !req.newTabInFront;

    const QString& url = m_currentView->upUrl().url();
    if(m_goKeyboardState & Qt::ControlModifier)
	openFilteredUrl(url, req );
    else if(m_goMouseState & Qt::MidButton)
    {
        if(KonqSettings::mmbOpensTab())
	    openFilteredUrl(url, req);
        else {
            KonqMainWindow* mw = KonqMisc::createNewWindow( url );
            mw->show();
        }
    }
    else
        openFilteredUrl( url, false );
    m_goMouseState = Qt::LeftButton;
}

void KonqMainWindow::slotUpActivated(QAction* action)
{
    openUrl( 0, action->data().value<KUrl>() );
}

void KonqMainWindow::slotGoHistoryActivated( int steps )
{
    slotGoHistoryActivated( steps, Qt::LeftButton, Qt::NoModifier );
}

void KonqMainWindow::slotGoHistoryActivated( int steps, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers )
{
  if (!m_goBuffer)
  {
    // Only start 1 timer.
    m_goBuffer = steps;
    m_goMouseState = buttons;
    m_goKeyboardState = modifiers;
    QTimer::singleShot( 0, this, SLOT(slotGoHistoryDelayed()));
  }
}

void KonqMainWindow::slotGoHistoryDelayed()
{
  if (!m_currentView) return;

  bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
  bool mmbOpensTab = KonqSettings::mmbOpensTab();
  bool inFront = KonqSettings::newTabsInFront();
  if(m_goKeyboardState & Qt::ShiftModifier)
      inFront = !inFront;

  if(m_goKeyboardState & Qt::ControlModifier)
  {
      KonqView * newView = m_pViewManager->addTabFromHistory( m_currentView, m_goBuffer, openAfterCurrentPage );
      if (newView && inFront)
        m_pViewManager->showTab( newView );
  }
  else if(m_goMouseState & Qt::MidButton)
  {
      if(mmbOpensTab)
      {
        KonqView * newView = m_pViewManager->addTabFromHistory( m_currentView, m_goBuffer, openAfterCurrentPage );
        if (newView && inFront)
          m_pViewManager->showTab( newView );
      }
      else
      {
        KonqMisc::newWindowFromHistory(this->currentView(), m_goBuffer);
      }
  }
  else
  {
      m_currentView->go( m_goBuffer );
      makeViewsFollow(m_currentView->url(),
                      KParts::OpenUrlArguments(),
                      KParts::BrowserArguments(),
                      m_currentView->serviceType(),
                      m_currentView);
  }

  m_goBuffer = 0;
  m_goMouseState = Qt::LeftButton;
  m_goKeyboardState = Qt::NoModifier;
}


void KonqMainWindow::slotBackAboutToShow()
{
    m_paBack->menu()->clear();
    if (m_currentView)
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paBack->menu(), true, false);
}


/**
 * Fill the closed tabs action menu before it's shown
 */
void KonqMainWindow::slotClosedItemsListAboutToShow()
{
    QMenu* popup = m_paClosedItems->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction* clearAction = popup->addAction( i18nc("This menu entry empties the closed items history", "Empty Closed Items History") );
    connect(clearAction, SIGNAL(triggered()), m_pUndoManager, SLOT(clearClosedItemsList()));
    popup->insertSeparator(static_cast<QAction*>(0));

    QList<KonqClosedItem *>::ConstIterator it =
        m_pUndoManager->closedItemsList().constBegin();
    const QList<KonqClosedItem *>::ConstIterator end =
        m_pUndoManager->closedItemsList().constEnd();
    for ( int i = 0; it != end && i < s_closedItemsListLength; ++it, ++i ) {
        const QString text = QString::number(i) + ' ' + (*it)->title();
        QAction* action = popup->addAction( (*it)->icon(), text );
        action->setActionGroup(m_closedItemsGroup);
        action->setData(i);
    }
    KAcceleratorManager::manage(popup);
}

/**
 * Fill the sessions list action menu before it's shown
 */
void KonqMainWindow::slotSessionsListAboutToShow()
{
    QMenu* popup = m_paSessions->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction* saveSessionAction = popup->addAction( KIcon("document-save"), i18n("Save As...") );
    connect(saveSessionAction, SIGNAL(triggered()), this, SLOT(saveCurrentSession()));
    QAction* manageSessionsAction = popup->addAction( KIcon("view-choose"), i18n("Manage...") );
    connect(manageSessionsAction, SIGNAL(triggered()), this, SLOT(manageSessions()));
    popup->addSeparator();

    QString dir= KStandardDirs::locateLocal("appdata", "sessions/");
    QDirIterator it(dir, QDir::Readable|QDir::NoDotAndDotDot|QDir::Dirs);

    while (it.hasNext())
    {
        QFileInfo fileInfo(it.next());

        QAction* action = popup->addAction( KIO::decodeFileName(fileInfo.baseName()) );
        action->setActionGroup(m_sessionsGroup);
        action->setData(fileInfo.filePath());
    }
    KAcceleratorManager::manage(popup);
}

void KonqMainWindow::saveCurrentSession()
{
    KonqNewSessionDlg dlg( this );
    dlg.exec();
}

void KonqMainWindow::manageSessions()
{
    KonqSessionDlg dlg( m_pViewManager, this );
    dlg.exec();
}

void KonqMainWindow::slotSessionActivated(QAction* action)
{
    QString dirpath = action->data().toString();
    KonqSessionManager::self()->restoreSessions(dirpath);
}

void KonqMainWindow::updateClosedItemsAction()
{
    bool available = m_pUndoManager->undoAvailable();
    m_paClosedItems->setEnabled(available);
    m_paUndo->setText(m_pUndoManager->undoText());
}

void KonqMainWindow::slotBack()
{
    slotGoHistoryActivated(-1);
}

void KonqMainWindow::slotBack(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    slotGoHistoryActivated( -1, buttons, modifiers );
}

void KonqMainWindow::slotBackActivated(QAction* action)
{
    KMenu* backMenu = static_cast<KMenu *>( m_paBack->menu() );
    slotGoHistoryActivated( action->data().toInt(), backMenu->mouseButtons(), backMenu->keyboardModifiers());
}

void KonqMainWindow::slotForwardAboutToShow()
{
    m_paForward->menu()->clear();
    if (m_currentView)
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paForward->menu(), false, true);
}

void KonqMainWindow::slotForward()
{
  slotGoHistoryActivated( 1 );
}

void KonqMainWindow::slotForward(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    slotGoHistoryActivated( 1, buttons, modifiers );
}

void KonqMainWindow::slotForwardActivated(QAction* action)
{
    KMenu* forwardMenu = static_cast<KMenu *>( m_paForward->menu() );
    slotGoHistoryActivated( action->data().toInt(), forwardMenu->mouseButtons(), forwardMenu->keyboardModifiers() );
}

void KonqMainWindow::checkDisableClearButton()
{
  // if the location toolbar already has the clear_location action,
  // disable the combobox's embedded clear icon.
  KToolBar* ltb = toolBar("locationToolBar");
  QAction* clearAction = action("clear_location");
  bool enable = true;
  foreach(QToolButton* atb, qFindChildren<QToolButton *>(ltb))
  {
     if (atb->defaultAction() == clearAction) {
         enable = false;
         break;
     }
  }
  KLineEdit* lineEdit = qobject_cast<KLineEdit*>(comboEdit());
  if (lineEdit)
      lineEdit->setClearButtonShown(enable);
}

void KonqMainWindow::initCombo()
{
  m_combo = new KonqCombo(0);

  m_combo->init( s_pCompletion );

  connect( m_combo, SIGNAL(activated(QString,Qt::KeyboardModifiers)),
           this, SLOT(slotURLEntered(QString,Qt::KeyboardModifiers)) );
  connect( m_combo, SIGNAL(showPageSecurity()),
           this, SLOT(showPageSecurity()) );

  m_pURLCompletion = new KUrlCompletion();
  m_pURLCompletion->setCompletionMode( s_pCompletion->completionMode() );

  // This only turns completion off. ~ is still there in the result
  // We do want completion of user names, right?
  //m_pURLCompletion->setReplaceHome( false );  // Leave ~ alone! Will be taken care of by filters!!

  connect( m_combo, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
           SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
  connect( m_combo, SIGNAL(completion(QString)),
           SLOT(slotMakeCompletion(QString)));
  connect( m_combo, SIGNAL(substringCompletion(QString)),
           SLOT(slotSubstringcompletion(QString)));
  connect( m_combo, SIGNAL(textRotation(KCompletionBase::KeyBindingType)),
           SLOT(slotRotation(KCompletionBase::KeyBindingType)));
  connect( m_combo, SIGNAL(cleared()),
           SLOT (slotClearHistory()) );
  connect( m_pURLCompletion, SIGNAL(match(QString)),
           SLOT(slotMatch(QString)));

  m_combo->installEventFilter(this);

  static bool bookmarkCompletionInitialized = false;
  if ( !bookmarkCompletionInitialized )
  {
      bookmarkCompletionInitialized = true;
      DelayedInitializer *initializer = new DelayedInitializer( QEvent::KeyPress, m_combo );
      connect( initializer, SIGNAL(initialize()), this, SLOT(bookmarksIntoCompletion()) );
  }
}

void KonqMainWindow::bookmarksIntoCompletion()
{
    // add all bookmarks to the completion list for easy access
    bookmarksIntoCompletion( s_bookmarkManager->root() );
}

// the user changed the completion mode in the combo
void KonqMainWindow::slotCompletionModeChanged( KGlobalSettings::Completion m )
{
  s_pCompletion->setCompletionMode( m );

  KonqSettings::setSettingsCompletionMode( int(m_combo->completionMode()) );
  KonqSettings::self()->writeConfig();

  // tell the other windows too (only this instance currently)
  foreach ( KonqMainWindow* window, *s_lstViews ) {
    if ( window && window->m_combo ) {
      window->m_combo->setCompletionMode( m );
      window->m_pURLCompletion->setCompletionMode( m );
    }
  }
}

// at first, try to find a completion in the current view, then use the global
// completion (history)
void KonqMainWindow::slotMakeCompletion( const QString& text )
{
  if( m_pURLCompletion )
  {
    m_urlCompletionStarted = true; // flag for slotMatch()

    // kDebug() << "Local Completion object found!";
    QString completion = m_pURLCompletion->makeCompletion( text );
    m_currentDir.clear();

    if ( completion.isNull() && !m_pURLCompletion->isRunning() )
    {
      // No match() signal will come from m_pURLCompletion
      // ask the global one
      // tell the static completion object about the current completion mode
      completion = s_pCompletion->makeCompletion( text );

      // some special handling necessary for CompletionPopup
      if ( m_combo->completionMode() == KGlobalSettings::CompletionPopup ||
           m_combo->completionMode() == KGlobalSettings::CompletionPopupAuto )
        m_combo->setCompletedItems( historyPopupCompletionItems( text ) );

      else if ( !completion.isNull() )
        m_combo->setCompletedText( completion );
    }
    else
    {
      // To be continued in slotMatch()...
      if( !m_pURLCompletion->dir().isEmpty() )
        m_currentDir = m_pURLCompletion->dir();
    }
  }
  // kDebug() << "Current dir:" << m_currentDir << "Current text:" << text;
}

void KonqMainWindow::slotSubstringcompletion( const QString& text )
{
    if (!m_currentView) return;

    QString currentURL = m_currentView->url().prettyUrl();
    bool filesFirst = currentURL.startsWith( '/' ) ||
                      currentURL.startsWith( "file:/" );
    QStringList items;
    if ( filesFirst && m_pURLCompletion )
        items = m_pURLCompletion->substringCompletion( text );

    items += s_pCompletion->substringCompletion( text );
    if ( !filesFirst && m_pURLCompletion )
        items += m_pURLCompletion->substringCompletion( text );

    m_combo->setCompletedItems( items );
}

void KonqMainWindow::slotRotation( KCompletionBase::KeyBindingType type )
{
  // Tell slotMatch() to do nothing
  m_urlCompletionStarted = false;

  bool prev = (type == KCompletionBase::PrevCompletionMatch);
  if ( prev || type == KCompletionBase::NextCompletionMatch ) {
    QString completion = prev ? m_pURLCompletion->previousMatch() :
                                m_pURLCompletion->nextMatch();

    if( completion.isNull() ) { // try the history KCompletion object
        completion = prev ? s_pCompletion->previousMatch() :
                            s_pCompletion->nextMatch();
    }
    if ( completion.isEmpty() || completion == m_combo->currentText() )
      return;

    m_combo->setCompletedText( completion );
  }
}

// Handle match() from m_pURLCompletion
void KonqMainWindow::slotMatch( const QString &match )
{
  if ( match.isEmpty() || !m_combo)
    return;

  // Check flag to avoid match() raised by rotation
  if ( m_urlCompletionStarted ) {
    m_urlCompletionStarted = false;

    // some special handling necessary for CompletionPopup
    if ( m_combo->completionMode() == KGlobalSettings::CompletionPopup ||
         m_combo->completionMode() == KGlobalSettings::CompletionPopupAuto ) {
      QStringList items = m_pURLCompletion->allMatches();
      items += historyPopupCompletionItems( m_combo->currentText() );
      items.removeDuplicates();  // when items from completion are also in history
      // items.sort(); // should we?
      m_combo->setCompletedItems( items );
    }
    else if ( !match.isNull() )
       m_combo->setCompletedText( match );
  }
}

void KonqMainWindow::slotCtrlTabPressed()
{
   KonqView * view = m_pViewManager->chooseNextView( m_currentView );
   //kDebug() << m_currentView->url() << "->" << view->url();
   if ( view ) {
      m_pViewManager->setActivePart( view->part() );
      KonqFrameTabs* tabs = m_pViewManager->tabContainer();
      m_pViewManager->showTab(tabs->tabIndexContaining(view->frame()));
   }
}

void KonqMainWindow::slotClearHistory()
{
   KonqHistoryManager::kself()->emitClear();
}

void KonqMainWindow::slotClearComboHistory()
{
   if (m_combo && m_combo->count())
      m_combo->clearHistory();
}

bool KonqMainWindow::eventFilter(QObject*obj, QEvent *ev)
{
  if ( ( ev->type()==QEvent::FocusIn || ev->type()==QEvent::FocusOut ) &&
       m_combo && m_combo->lineEdit() && m_combo == obj )
  {
    //kDebug() << obj << obj->metaObject()->className() << obj->name();

    QFocusEvent * focusEv = static_cast<QFocusEvent*>(ev);
    if (focusEv->reason() == Qt::PopupFocusReason)
    {
      return KParts::MainWindow::eventFilter( obj, ev );
    }

    KParts::BrowserExtension * ext = 0;
    if ( m_currentView )
        ext = m_currentView->browserExtension();

    if (ev->type()==QEvent::FocusIn)
    {
      //kDebug() << "ComboBox got the focus...";
      if (m_bLocationBarConnected)
      {
        //kDebug() << "Was already connected...";
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = true;

      // Workaround for Qt issue: usually, QLineEdit reacts on Ctrl-D,
      // but the duplicatecurrenttab action also has Ctrl-D as accel and
      // prevents the lineedit from getting this event. IMHO the accel
      // should be disabled in favor of the focus-widget.
      // TODO: decide if the delete-character behaviour of QLineEdit
      // really is useful enough to warrant this workaround
      QAction *duplicate = actionCollection()->action("duplicatecurrenttab");
      if ( duplicate->shortcuts().contains( QKeySequence(Qt::CTRL+Qt::Key_D) ))
          duplicate->setEnabled( false );

      connect( m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()) );
      connect( m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()) );
      connect( m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()) );
      connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      connect( m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()) );
      connect( m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()) );

      slotClipboardDataChanged();
    }
    else if ( ev->type()==QEvent::FocusOut)
    {
      //kDebug() << "ComboBox lost focus...";
      if (!m_bLocationBarConnected)
      {
        //kDebug() << "Was already disconnected...";
        return KParts::MainWindow::eventFilter( obj, ev );
      }
      m_bLocationBarConnected = false;

      // see above in FocusIn for explanation
      // action is reenabled if a view exists
      QAction *duplicate = actionCollection()->action("duplicatecurrenttab");
      if ( duplicate->shortcuts().contains( QKeySequence(Qt::CTRL+Qt::Key_D) ) )
          duplicate->setEnabled( currentView() && currentView()->frame() );

      disconnect( m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()) );
      disconnect( m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()) );
      disconnect( m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()) );
      disconnect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
      disconnect( m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()) );
      disconnect( m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()) );

      if ( ext )
      {
          m_paCut->setEnabled( ext->isActionEnabled( "cut" ) );
          m_paCopy->setEnabled( ext->isActionEnabled( "copy" ) );
          m_paPaste->setEnabled( ext->isActionEnabled( "paste" ) );
      }
      else
      {
          m_paCut->setEnabled( false );
          m_paCopy->setEnabled( false );
          m_paPaste->setEnabled( false );
      }
    }
  } else if (ev->type()==QEvent::KeyPress) {
      QKeyEvent * keyEv = static_cast<QKeyEvent*>(ev);
      if ((keyEv->key() == Qt::Key_Tab) && (keyEv->modifiers() == Qt::ControlModifier)) {
          slotCtrlTabPressed();
          return true; // don't let QTabWidget see the event
      } else if (obj == m_combo && m_currentView && keyEv->key() == Qt::Key_Escape) {
          // reset url to current view's actual url on ESC
          m_combo->setURL(m_currentView->url().pathOrUrl());
          m_combo->lineEdit()->setModified(false);
          return true;
      }
  }
  return KParts::MainWindow::eventFilter( obj, ev );
}

// Only called when m_bLocationBarConnected, i.e. when the combobox has focus.
// The rest of the time, the part handles the cut/copy/paste actions.
void KonqMainWindow::slotClipboardDataChanged()
{
  const QMimeData *data = QApplication::clipboard()->mimeData();
  m_paPaste->setEnabled( data->hasText() );
  slotCheckComboSelection();
}

void KonqMainWindow::slotCheckComboSelection()
{
    QLineEdit* edit = comboEdit();
    if (edit) {
        const bool hasSelection = edit->hasSelectedText();
        //kDebug() << "m_combo->lineEdit()->hasMarkedText():" << hasSelection;
        m_paCopy->setEnabled( hasSelection );
        m_paCut->setEnabled( hasSelection );
    }
}

void KonqMainWindow::slotClearLocationBar()
{
    slotStop();
    if (m_combo)
        m_combo->clearTemporary();
    focusLocationBar();
}

void KonqMainWindow::slotForceSaveMainWindowSettings()
{
    if ( autoSaveSettings() ) { // don't do it on e.g. JS window.open windows with no toolbars!
        saveAutoSaveSettings();
    }
}

void KonqMainWindow::slotShowMenuBar()
{
  menuBar()->setVisible(!menuBar()->isVisible());
  slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotShowStatusBar()
{
  if (m_currentView)
    m_currentView->frame()->statusbar()->setVisible( m_paShowStatusBar->isChecked() );

  // An alternative: this will change simultaneously all of the status bars on
  // all of the current views.
  //MapViews::const_iterator end = m_mapViews.constEnd();
  //for (MapViews::const_iterator it = m_mapViews.constBegin(); it != end; ++it) {
  //  KonqView* view = it.value();
  //  view->frame()->statusbar()->setVisible(on);
  //}

  slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotUpdateFullScreen( bool set )
{
  KToggleFullScreenAction::setFullScreen( this, set );
  if( set )
  {
    // Create toolbar button for exiting from full-screen mode
    // ...but only if there isn't one already...

    bool haveFullScreenButton = false;

    //Walk over the toolbars and check whether there is a show fullscreen button in any of them
    foreach (KToolBar* bar, findChildren<KToolBar*>())
    {
        //Are we plugged here, in a visible toolbar?
        if (bar->isVisible() &&
            action( "fullscreen" )->associatedWidgets().contains(bar))
        {
            haveFullScreenButton = true;
            break;
        }
    }

    if (!haveFullScreenButton)
    {
        QList<QAction*> lst;
        lst.append( m_ptaFullScreen );
        plugActionList( "fullscreen", lst );
    }


    m_prevMenuBarVisible = menuBar()->isVisible();
    menuBar()->hide();
    m_paShowMenuBar->setChecked( false );

    // Qt bug, the flags are lost. They know about it.
    // happens only with the hackish non-_NET_WM_STATE_FULLSCREEN way
    setAttribute( Qt::WA_DeleteOnClose );
    // Qt bug (see below)
//### KDE4: still relevant?
#if 0
    setAcceptDrops( false );
    topData()->dnd = 0;
    setAcceptDrops( true );
#endif
  }
  else
  {
    unplugActionList( "fullscreen" );

    if (m_prevMenuBarVisible)
    {
        menuBar()->show();
        m_paShowMenuBar->setChecked( true );
    }

    // Qt bug, the flags aren't restored. They know about it.
    //setWFlags( WType_TopLevel | WDestructiveClose );
#ifdef __GNUC__
#warning "Dunno how to port this, is the workaround still needed?"
#endif
//                (  Qt::Window );
    setAttribute( Qt::WA_DeleteOnClose );

#if 0 //### KDE4: is this still relevant?
    // Other Qt bug
    setAcceptDrops( false );
    topData()->dnd = 0;
    setAcceptDrops( true );
#endif
  }
}

void KonqMainWindow::setLocationBarURL( const KUrl &url )
{
    setLocationBarURL( url.pathOrUrl() );
}

void KonqMainWindow::setLocationBarURL( const QString &url )
{
    // Don't set the location bar URL if it hasn't changed
    // or if the user had time to edit the url since the last call to openUrl (#64868)
    QLineEdit* edit = comboEdit();
    if (edit && url != edit->text() && !edit->isModified()) {
        //kDebug() << "url=" << url;
        m_combo->setURL( url );
        updateWindowIcon();
    }
}

void KonqMainWindow::setPageSecurity( PageSecurity pageSecurity )
{
  if (m_combo)
    m_combo->setPageSecurity( pageSecurity );
}

void KonqMainWindow::showPageSecurity()
{
    if ( m_currentView && m_currentView->part() ) {
      QAction *act = m_currentView->part()->action( "security" );
      if ( act )
        act->trigger();
    }
}

// Called via DBUS from KonquerorApplication
void KonqMainWindow::comboAction( int action, const QString& url, const QString& senderId )
{
    if (!s_lstViews) // this happens in "konqueror --silent"
        return;

    KonqCombo *combo = 0;
    foreach ( KonqMainWindow* window, *s_lstViews ) {
        if ( window && window->m_combo ) {
            combo = window->m_combo;

            switch ( action ) {
            case ComboAdd:
              combo->insertPermanent( url );
              break;
            case ComboClear:
              combo->clearHistory();
              break;
            case ComboRemove:
              combo->removeURL( url );
              break;
            default:
              break;
            }
        }
    }

    // only one instance should save...
    if ( combo && senderId == QDBusConnection::sessionBus().baseService() )
      combo->saveItems();
}

QString KonqMainWindow::locationBarURL() const
{
    return (m_combo ? m_combo->currentText() : QString());
}

void KonqMainWindow::focusLocationBar()
{
  if (m_combo && (m_combo->isVisible() || !isVisible()))
    m_combo->setFocus();
}

void KonqMainWindow::startAnimation()
{
  m_paAnimatedLogo->start();
  m_paStop->setEnabled( true );
}

void KonqMainWindow::stopAnimation()
{
  m_paAnimatedLogo->stop();
  m_paStop->setEnabled( false );
}

void KonqMainWindow::setUpEnabled( const KUrl &url )
{
      bool bHasUpURL = ( (url.hasPath() && url.path() != "/" && url.path()[0] == '/')
                         || !url.query().isEmpty() /*e.g. lists.kde.org*/ );
      if ( !bHasUpURL )
          bHasUpURL = url.hasSubUrl();

      m_paUp->setEnabled(bHasUpURL);
}

void KonqMainWindow::initActions()
{
  // Note about this method : don't call setEnabled() on any of the actions.
  // They are all disabled then re-enabled with enableAllActions
  // If any one needs to be initially disabled, put that code in enableAllActions

  // For the popup menu only.
  m_pMenuNew = new KNewFileMenu(actionCollection(), "new_menu", this);

  // File menu

  KAction* action = actionCollection()->addAction("new_window");
  action->setIcon(KIcon("window-new"));
  action->setText(i18n( "New &Window" ));
  connect(action, SIGNAL(triggered()), SLOT(slotNewWindow()));
  action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::New));
  action = actionCollection()->addAction("duplicate_window");
  action->setIcon(KIcon("window-duplicate"));
  action->setText(i18n( "&Duplicate Window" ));
  connect(action, SIGNAL(triggered()), SLOT(slotDuplicateWindow()));
  action->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_D);
  action = actionCollection()->addAction("sendURL");
  action->setIcon(KIcon("mail-message-new"));
  action->setText(i18n( "Send &Link Address..." ));
  connect(action, SIGNAL(triggered()), SLOT(slotSendURL()));
  action = actionCollection()->addAction("sendPage");
  action->setIcon(KIcon("mail-message-new"));
  action->setText(i18n( "S&end File..." ));
  connect(action, SIGNAL(triggered()), SLOT(slotSendFile()));
  action = actionCollection()->addAction("open_location");
  action->setIcon(KIcon("document-open-remote"));
  action->setText(i18n( "&Open Location" ));
  action->setShortcut(Qt::ALT+Qt::Key_O);
  connect(action, SIGNAL(triggered()), SLOT(slotOpenLocation()));

  action = actionCollection()->addAction("open_file");
  action->setIcon(KIcon("document-open"));
  action->setText(i18n( "&Open File..." ));
  connect(action, SIGNAL(triggered()), SLOT(slotOpenFile()));
  action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Open));

#if 0
  m_paFindFiles = new KToggleAction(KIcon("edit-find"), i18n( "&Find File..." ), this);
  actionCollection()->addAction( "findfile", m_paFindFiles );
  connect(m_paFindFiles, SIGNAL(triggered()), SLOT(slotToolFind()));
  m_paFindFiles->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Find));
#endif

  m_paPrint = actionCollection()->addAction( KStandardAction::Print, "print", 0, 0 );
  actionCollection()->addAction( KStandardAction::Quit, "quit", this, SLOT(close()) );

  m_paLockView = new KToggleAction( i18n( "Lock to Current Location"), this );
  actionCollection()->addAction( "lock", m_paLockView );
  connect(m_paLockView, SIGNAL(triggered()), SLOT(slotLockView()));
  m_paLinkView = new KToggleAction( i18nc( "This option links konqueror views", "Lin&k View"), this );
  actionCollection()->addAction( "link", m_paLinkView );
  connect(m_paLinkView, SIGNAL(triggered()), SLOT(slotLinkView()));

  // Go menu
  m_paUp = new KToolBarPopupAction( KIcon("go-up"), i18n( "&Up" ), this );
  actionCollection()->addAction( "go_up", m_paUp );
  m_paUp->setShortcuts( KStandardShortcut::shortcut(KStandardShortcut::Up) );
  connect( m_paUp, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this,
	   SLOT(slotUp(Qt::MouseButtons,Qt::KeyboardModifiers)) );
  connect( m_paUp->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpAboutToShow()) );
  connect( m_paUp->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotUpActivated(QAction*)) );

  QPair< KGuiItem, KGuiItem > backForward = KStandardGuiItem::backAndForward();


  // Trash bin of closed tabs
  m_paClosedItems = new KToolBarPopupAction( KIcon("edit-undo-closed-tabs"),  i18n( "Closed Items" ), this );
  actionCollection()->addAction( "closeditems", m_paClosedItems );
  m_closedItemsGroup = new QActionGroup(m_paClosedItems->menu());

  // set the closed tabs list shown
  connect( m_paClosedItems, SIGNAL(triggered()), m_pUndoManager, SLOT(undoLastClosedItem()) );
  connect( m_paClosedItems->menu(), SIGNAL(aboutToShow()), this, SLOT(slotClosedItemsListAboutToShow()) );
  connect( m_closedItemsGroup, SIGNAL(triggered(QAction*)), m_pUndoManager, SLOT(slotClosedItemsActivated(QAction*)) );
  connect( m_pViewManager, SIGNAL(aboutToRemoveTab(KonqFrameBase*)), this, SLOT(slotAddClosedUrl(KonqFrameBase*)) );
  connect( m_pUndoManager, SIGNAL(openClosedTab(KonqClosedTabItem)), m_pViewManager, SLOT(openClosedTab(KonqClosedTabItem)) );
  connect( m_pUndoManager, SIGNAL(openClosedWindow(KonqClosedWindowItem)), m_pViewManager, SLOT(openClosedWindow(KonqClosedWindowItem)) );
  connect( m_pUndoManager, SIGNAL(closedItemsListChanged()), this, SLOT(updateClosedItemsAction()));


  m_paSessions = new KActionMenu( i18n( "Sessions" ), this );
  actionCollection()->addAction( "sessions", m_paSessions );
  m_sessionsGroup = new QActionGroup(m_paSessions->menu());
  connect( m_paSessions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotSessionsListAboutToShow()) );
  connect( m_sessionsGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotSessionActivated(QAction*)) );

  m_paBack = new KToolBarPopupAction( KIcon(backForward.first.iconName()), backForward.first.text(), this );
  actionCollection()->addAction( "go_back", m_paBack );
  m_paBack->setShortcuts( KStandardShortcut::shortcut(KStandardShortcut::Back) );
  connect( m_paBack, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this,
	   SLOT(slotBack(Qt::MouseButtons,Qt::KeyboardModifiers)) );
  connect( m_paBack->menu(), SIGNAL(aboutToShow()), this, SLOT(slotBackAboutToShow()) );
  connect( m_paBack->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotBackActivated(QAction*)) );

  m_paForward = new KToolBarPopupAction( KIcon(backForward.second.iconName()), backForward.second.text(), this );
  actionCollection()->addAction( "go_forward", m_paForward );
  m_paForward->setShortcuts( KStandardShortcut::shortcut(KStandardShortcut::Forward) );
  connect( m_paForward, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this,
	   SLOT(slotForward(Qt::MouseButtons,Qt::KeyboardModifiers)) );
  connect( m_paForward->menu(), SIGNAL(aboutToShow()), this, SLOT(slotForwardAboutToShow()) );
  connect( m_paForward->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotForwardActivated(QAction*)) );

  m_paHome = actionCollection()->addAction( KStandardAction::Home );
  connect( m_paHome, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this,
	   SLOT(slotHome(Qt::MouseButtons,Qt::KeyboardModifiers)) );

  KonqMostOftenURLSAction *mostOften = new KonqMostOftenURLSAction( i18nc("@action:inmenu Go", "Most Often Visited"), this );
  actionCollection()->addAction( "go_most_often", mostOften );
  connect(mostOften, SIGNAL(activated(KUrl)), SLOT(slotOpenURL(KUrl)));

  action = new KonqHistoryAction(i18nc("@action:inmenu Go", "Recently Visited"), this);
  actionCollection()->addAction("history", action);
  connect(action, SIGNAL(activated(KUrl)), SLOT(slotOpenURL(KUrl)));

  action = actionCollection()->addAction("go_history");
  action->setIcon(KIcon("view-history"));
  // Ctrl+Shift+H, shortcut from firefox
  // TODO: and Ctrl+H should open the sidebar history module
  action->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_H);
  action->setText(i18nc("@action:inmenu Go", "Show History"));
  connect(action, SIGNAL(triggered()), SLOT(slotGoHistory()));

  // Settings menu

  m_paSaveViewProfile = actionCollection()->addAction( "saveviewprofile" );
  m_paSaveViewProfile->setText( i18n( "&Save View Profile As..." ) );
  connect(m_paSaveViewProfile, SIGNAL(triggered()), SLOT(slotSaveViewProfile()));

  // This list is just for the call to authorizeControlModule; see slotConfigure for the real code
  QStringList configureModules;
  configureModules << "khtml_general" << "bookmarks" <<
      "filebehavior" << "filetypes" << "kcmtrash" <<
      "khtml_appearance" << "khtml_behavior" << "khtml_java_js" <<
      "khtml_filter" << "ebrowsing" <<
      "kcmhistory" << "cookies" <<
      "cache" << "proxy" <<
      "crypto" << "useragent" <<
      "khtml_plugins" << "kcmkonqyperformance";

  if (!KAuthorized::authorizeControlModules(configureModules).isEmpty())
      actionCollection()->addAction( KStandardAction::Preferences, this, SLOT(slotConfigure()) );

  actionCollection()->addAction( KStandardAction::KeyBindings, guiFactory(), SLOT(configureShortcuts()) );
  actionCollection()->addAction( KStandardAction::ConfigureToolbars, this, SLOT(slotConfigureToolbars()) );

  m_paConfigureExtensions = actionCollection()->addAction("options_configure_extensions");
  m_paConfigureExtensions->setText( i18n("Configure Extensions...") );
  connect(m_paConfigureExtensions, SIGNAL(triggered()), SLOT(slotConfigureExtensions()));
  m_paConfigureSpellChecking = actionCollection()->addAction("configurespellcheck");
  m_paConfigureSpellChecking->setIcon(KIcon("tools-check-spelling"));
  m_paConfigureSpellChecking->setText(i18n("Configure Spell Checking..."));
  connect(m_paConfigureSpellChecking, SIGNAL(triggered()), SLOT(slotConfigureSpellChecking()));

  // Window menu
  m_paSplitViewHor = actionCollection()->addAction("splitviewh");
  m_paSplitViewHor->setIcon( KIcon("view-split-left-right") );
  m_paSplitViewHor->setText( i18n( "Split View &Left/Right" ) );
  connect(m_paSplitViewHor, SIGNAL(triggered()), SLOT(slotSplitViewHorizontal()));
  m_paSplitViewHor->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_L);
  m_paSplitViewVer = actionCollection()->addAction("splitviewv");
  m_paSplitViewVer->setIcon( KIcon("view-split-top-bottom") );
  m_paSplitViewVer->setText( i18n( "Split View &Top/Bottom" ) );
  connect(m_paSplitViewVer, SIGNAL(triggered()), SLOT(slotSplitViewVertical()));
  m_paSplitViewVer->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_T);
  m_paAddTab = actionCollection()->addAction("newtab");
  m_paAddTab->setIcon( KIcon("tab-new") );
  m_paAddTab->setText( i18n( "&New Tab" ) );
  connect(m_paAddTab, SIGNAL(triggered()), SLOT(slotAddTab()));
  m_paAddTab->setShortcuts(KShortcut(Qt::CTRL+Qt::Key_T, Qt::CTRL+Qt::SHIFT+Qt::Key_N));

  m_paDuplicateTab = actionCollection()->addAction("duplicatecurrenttab");
  m_paDuplicateTab->setIcon( KIcon("tab-duplicate") );
  m_paDuplicateTab->setText( i18n( "&Duplicate Current Tab" ) );
  connect(m_paDuplicateTab, SIGNAL(triggered()), SLOT(slotDuplicateTab()));
  m_paDuplicateTab->setShortcut(Qt::CTRL+Qt::Key_D);
  m_paBreakOffTab = actionCollection()->addAction("breakoffcurrenttab");
  m_paBreakOffTab->setIcon( KIcon("tab-detach") );
  m_paBreakOffTab->setText( i18n( "Detach Current Tab" ) );
  connect(m_paBreakOffTab, SIGNAL(triggered()), SLOT(slotBreakOffTab()));
  m_paBreakOffTab->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_B);
  m_paRemoveView = actionCollection()->addAction("removeview");
  m_paRemoveView->setIcon( KIcon("view-close") );
  m_paRemoveView->setText( i18n( "&Close Active View" ) );
  connect(m_paRemoveView, SIGNAL(triggered()), SLOT(slotRemoveView()));
  m_paRemoveView->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_W);
  m_paRemoveTab = actionCollection()->addAction("removecurrenttab");
  m_paRemoveTab->setIcon( KIcon("tab-close") );
  m_paRemoveTab->setText( i18n( "Close Current Tab" ) );
  connect(m_paRemoveTab, SIGNAL(triggered()), SLOT(slotRemoveTab()), Qt::QueuedConnection /* exit Ctrl+W handler before deleting */);
  m_paRemoveTab->setShortcut(Qt::CTRL+Qt::Key_W);
  m_paRemoveTab->setAutoRepeat(false);
  m_paRemoveOtherTabs = actionCollection()->addAction("removeothertabs");
  m_paRemoveOtherTabs->setIcon( KIcon("tab-close-other") );
  m_paRemoveOtherTabs->setText( i18n( "Close &Other Tabs" ) );
  connect(m_paRemoveOtherTabs, SIGNAL(triggered()), SLOT(slotRemoveOtherTabs()));

  m_paActivateNextTab = actionCollection()->addAction("activatenexttab");
  m_paActivateNextTab->setText( i18n( "Activate Next Tab" ) );
  connect(m_paActivateNextTab, SIGNAL(triggered()), SLOT(slotActivateNextTab()));
  m_paActivateNextTab->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext());
  m_paActivatePrevTab = actionCollection()->addAction("activateprevtab");
  m_paActivatePrevTab->setText( i18n( "Activate Previous Tab" ) );
  connect(m_paActivatePrevTab, SIGNAL(triggered()), SLOT(slotActivatePrevTab()));
  m_paActivatePrevTab->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev());

  for (int i=1;i<13;i++) {
    QString actionname = QString::asprintf("activate_tab_%02d", i);
    QAction *action = actionCollection()->addAction( actionname );
    action->setText( i18n("Activate Tab %1", i) );
    connect(action, SIGNAL(triggered()), SLOT(slotActivateTab()));
  }

  m_paMoveTabLeft = actionCollection()->addAction("tab_move_left");
  m_paMoveTabLeft->setText( i18n("Move Tab Left") );
  m_paMoveTabLeft->setIcon( KIcon("arrow-left") );
  connect(m_paMoveTabLeft, SIGNAL(triggered()), SLOT(slotMoveTabLeft()));
  m_paMoveTabLeft->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_Left);
  m_paMoveTabRight = actionCollection()->addAction("tab_move_right");
  m_paMoveTabRight->setText( i18n("Move Tab Right") );
  m_paMoveTabRight->setIcon( KIcon("arrow-right") );
  connect(m_paMoveTabRight, SIGNAL(triggered()), SLOT(slotMoveTabRight()));
  m_paMoveTabRight->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_Right);

#ifndef NDEBUG
  action = actionCollection()->addAction("dumpdebuginfo");
  action->setIcon( KIcon("view-dump-debug-info") );
  action->setText( i18n( "Dump Debug Info" ) );
  connect(action, SIGNAL(triggered()), SLOT(slotDumpDebugInfo()));
#endif

  m_paSaveRemoveViewProfile = actionCollection()->addAction("saveremoveviewprofile" );
  m_paSaveRemoveViewProfile->setText( i18n( "C&onfigure View Profiles..." ) );
  connect(m_paSaveRemoveViewProfile, SIGNAL(triggered()), m_pViewManager, SLOT(slotProfileDlg()));
  m_pamLoadViewProfile = new KActionMenu( i18n( "Load &View Profile" ), this );
  actionCollection()->addAction( "loadviewprofile", m_pamLoadViewProfile );

  m_pViewManager->setProfiles( m_pamLoadViewProfile );

  m_ptaFullScreen = KStandardAction::fullScreen( 0, 0, this, this );
  actionCollection()->addAction( m_ptaFullScreen->objectName(), m_ptaFullScreen );
  KShortcut fullScreenShortcut = m_ptaFullScreen->shortcut();
  fullScreenShortcut.setAlternate( Qt::Key_F11 );
  m_ptaFullScreen->setShortcut( fullScreenShortcut );
  connect( m_ptaFullScreen, SIGNAL(toggled(bool)), this, SLOT(slotUpdateFullScreen(bool)));

  KShortcut reloadShortcut = KStandardShortcut::shortcut(KStandardShortcut::Reload);
  reloadShortcut.setAlternate(Qt::CTRL + Qt::Key_R);
  m_paReload = actionCollection()->addAction("reload");
  m_paReload->setIcon( KIcon("view-refresh") );
  m_paReload->setText( i18n( "&Reload" ) );
  connect(m_paReload, SIGNAL(triggered()), SLOT(slotReload()));
  m_paReload->setShortcuts(reloadShortcut);
  m_paReloadAllTabs = actionCollection()->addAction("reload_all_tabs");
  m_paReloadAllTabs->setIcon( KIcon("view-refresh-all") );
  m_paReloadAllTabs->setText( i18n( "&Reload All Tabs" ) );
  connect(m_paReloadAllTabs, SIGNAL(triggered()), SLOT(slotReloadAllTabs()));
  m_paReloadAllTabs->setShortcut(Qt::SHIFT+Qt::Key_F5);
  // "Forced"/ "Hard" reload action - re-downloads all e.g. images even if a cached
  // version already exists.
  m_paForceReload = actionCollection()->addAction("hard_reload");
  // TODO - request new icon? (view-refresh will do for the time being)
  m_paForceReload->setIcon( KIcon("view-refresh") );
  m_paForceReload->setText( i18n( "&Force Reload" ) );
  connect(m_paForceReload, SIGNAL(triggered()), SLOT(slotForceReload()));
  m_paForceReload->setShortcuts(KShortcut(Qt::CTRL+Qt::Key_F5, Qt::CTRL+Qt::SHIFT+Qt::Key_R));

  m_paUndo = KStandardAction::undo( m_pUndoManager, SLOT(undo()), this );
  actionCollection()->addAction( "undo", m_paUndo );
  connect( m_pUndoManager, SIGNAL(undoTextChanged(QString)),
           this, SLOT(slotUndoTextChanged(QString)) );

  // Those are connected to the browserextension directly
  m_paCut = KStandardAction::cut( 0, 0, this );
  actionCollection()->addAction( "cut", m_paCut );
  KShortcut cutShortCut( m_paCut->shortcut() );
  cutShortCut.remove( Qt::SHIFT + Qt::Key_Delete ); // used for deleting files
  m_paCut->setShortcuts( cutShortCut );

  m_paCopy = KStandardAction::copy( 0, 0, this );
  actionCollection()->addAction( "copy", m_paCopy );
  m_paPaste = KStandardAction::paste( 0, 0, this );
  actionCollection()->addAction( "paste", m_paPaste );
  m_paStop = actionCollection()->addAction("stop");
  m_paStop->setIcon( KIcon("process-stop") );
  m_paStop->setText( i18n( "&Stop" ) );
  connect(m_paStop, SIGNAL(triggered()), SLOT(slotStop()));
  m_paStop->setShortcut(Qt::Key_Escape);

  m_paAnimatedLogo = new KonqAnimatedLogo;
  m_paAnimatedLogo->setIcons("process-working-kde");
  KAction *logoAction = new KAction( this );
  actionCollection()->addAction( "konq_logo", logoAction );
  logoAction->setDefaultWidget( m_paAnimatedLogo );
  // Set icon and text so that it's easier to figure out what the action is in the toolbar editor
  logoAction->setText( i18n("Throbber") );
  logoAction->setIcon( KIcon("kde") );

  // Location bar
  m_locationLabel = new KonqDraggableLabel( this, i18n("L&ocation: ") );
  KAction *locationAction = new KAction( this );
  actionCollection()->addAction( "location_label", locationAction );
  locationAction->setText( i18n("L&ocation: ") );
  connect(locationAction, SIGNAL(triggered()), SLOT(slotLocationLabelActivated()));
  locationAction->setDefaultWidget(m_locationLabel);
  m_locationLabel->setBuddy( m_combo );

  KAction *comboAction = new KAction( this );
  actionCollection()->addAction( "toolbar_url_combo", comboAction );
  comboAction->setText( i18n( "Location Bar" ) );
  comboAction->setShortcut(Qt::Key_F6);
  connect(comboAction, SIGNAL(triggered()), SLOT(slotLocationLabelActivated()));
  comboAction->setDefaultWidget(m_combo);
  comboAction->setShortcutConfigurable( false );

  m_combo->setWhatsThis( i18n( "<html>Location Bar<br /><br />Enter a web address or search term.</html>" ) );

  KAction *clearLocation = actionCollection()->addAction("clear_location");
  clearLocation->setIcon( KIcon(QApplication::isRightToLeft() ? "edit-clear-locationbar-rtl" : "edit-clear-locationbar-ltr") );
  clearLocation->setText( i18n( "Clear Location Bar" ) );
  clearLocation->setShortcut(Qt::CTRL+Qt::Key_L);
  connect( clearLocation, SIGNAL(triggered()),
           SLOT(slotClearLocationBar()) );
  clearLocation->setWhatsThis( i18n( "<html>Clear Location bar<br /><br />"
                                     "Clears the contents of the location bar.</html>" ) );

  // Bookmarks menu
  m_pamBookmarks = new KBookmarkActionMenu(s_bookmarkManager->root(),
                                              i18n( "&Bookmarks" ), this);
  actionCollection()->addAction( "bookmarks", m_pamBookmarks );
  m_pamBookmarks->setDelayed(false);

  // The actual menu needs a different action collection, so that the bookmarks
  // don't appear in kedittoolbar
  m_bookmarksActionCollection = new KActionCollection( static_cast<QWidget*>( this ) );

  m_pBookmarkMenu = new KonqBookmarkMenu( s_bookmarkManager, m_pBookmarksOwner, m_pamBookmarks, m_bookmarksActionCollection );

  QAction *addBookmark = m_bookmarksActionCollection->action("add_bookmark");
  if (addBookmark) {
     // Keep the "Add bookmark" action visible though (#153835)
     // -> We should think of a way to mark actions as "not configurable in toolbars" and
     // "should not appear in shortcut dialog (!= isShortcutConfigurable)" instead, and use
     // a single actionCollection.
     actionCollection()->addAction("add_bookmark", m_bookmarksActionCollection->takeAction(addBookmark));
  } else {
     kDebug() << "Action add_bookmark not found!";
  }

  m_paShowMenuBar = KStandardAction::showMenubar( this, SLOT(slotShowMenuBar()), this );
  actionCollection()->addAction( KStandardAction::name(KStandardAction::ShowMenubar), m_paShowMenuBar );

  m_paShowStatusBar = KStandardAction::showStatusbar( this, SLOT(slotShowStatusBar()), this );
  actionCollection()->addAction( KStandardAction::name(KStandardAction::ShowStatusbar), m_paShowStatusBar );

  action = actionCollection()->addAction( "konqintro" );
  action->setText( i18n( "Kon&queror Introduction" ) );
  connect(action, SIGNAL(triggered()), SLOT(slotIntro()));

  QAction *goUrl = actionCollection()->addAction("go_url");
  goUrl->setIcon( KIcon("go-jump-locationbar") );
  goUrl->setText( i18n( "Go" ) );
  connect(goUrl, SIGNAL(triggered()), SLOT(goURL()));
  goUrl->setWhatsThis( i18n( "<html>Go<br /><br />"
			     "Goes to the page that has been entered into the location bar.</html>" ) );

  enableAllActions( false );

  // help stuff
  m_paUp->setWhatsThis( i18n( "<html>Enter the parent folder<br /><br />"
                              "For instance, if the current location is file:/home/%1 clicking this "
                              "button will take you to file:/home.</html>" ,  KUser().loginName() ) );
  m_paUp->setStatusTip( i18n( "Enter the parent folder" ) );

  m_paBack->setWhatsThis( i18n( "Move backwards one step in the browsing history" ) );
  m_paBack->setStatusTip( i18n( "Move backwards one step in the browsing history" ) );

  m_paForward->setWhatsThis( i18n( "Move forward one step in the browsing history" ) );
  m_paForward->setStatusTip( i18n( "Move forward one step in the browsing history" ) );


  m_paClosedItems->setWhatsThis( i18n( "Move backwards one step in the closed tabs history" ) );
  m_paClosedItems->setStatusTip( i18n( "Move backwards one step in the closed tabs history" ) );


  m_paReload->setWhatsThis( i18n( "<html>Reload the currently displayed document<br /><br />"
                                  "This may, for example, be needed to refresh web pages that have been "
                                  "modified since they were loaded, in order to make the changes visible.</html>" ) );
  m_paReload->setStatusTip( i18n( "Reload the currently displayed document" ) );

  m_paReloadAllTabs->setWhatsThis( i18n( "<html>Reload all currently displayed documents in tabs<br /><br />"
                                  "This may, for example, be needed to refresh web pages that have been "
                                  "modified since they were loaded, in order to make the changes visible.</html>" ) );
  m_paReloadAllTabs->setStatusTip( i18n( "Reload all currently displayed document in tabs" ) );

  m_paStop->setWhatsThis( i18n( "<html>Stop loading the document<br /><br />"
                                "All network transfers will be stopped and Konqueror will display the content "
                                "that has been received so far.</html>" ) );

  m_paForceReload->setWhatsThis( i18n( "<html>Reload the currently displayed document<br /><br />"
          "This may, for example, be needed to refresh web pages that have been "
          "modified since they were loaded, in order to make the changes visible. Any images on the page are downloaded again, even if cached copies exist.</html>" ) );

  m_paForceReload->setStatusTip( i18n( "Force a reload of the currently displayed document and any contained images" ) );


  m_paStop->setStatusTip( i18n( "Stop loading the document" ) );

  m_paCut->setWhatsThis( i18n( "<html>Cut the currently selected text or item(s) and move it "
                               "to the system clipboard<br /><br />"
                               "This makes it available to the <b>Paste</b> command in Konqueror "
                               "and other KDE applications.</html>" ) );
  m_paCut->setStatusTip( i18n( "Move the selected text or item(s) to the clipboard" ) );

  m_paCopy->setWhatsThis( i18n( "<html>Copy the currently selected text or item(s) to the "
                                "system clipboard<br /><br />"
                                "This makes it available to the <b>Paste</b> command in Konqueror "
                                "and other KDE applications.</html>" ) );
  m_paCopy->setStatusTip( i18n( "Copy the selected text or item(s) to the clipboard" ) );

  m_paPaste->setWhatsThis( i18n( "<html>Paste the previously cut or copied clipboard "
                                 "contents<br /><br />"
                                 "This also works for text copied or cut from other KDE applications.</html>" ) );
  m_paPaste->setStatusTip( i18n( "Paste the clipboard contents" ) );

  m_paPrint->setWhatsThis( i18n( "<html>Print the currently displayed document<br /><br />"
                                 "You will be presented with a dialog where you can set various "
                                 "options, such as the number of copies to print and which printer "
                                 "to use.<br /><br />"
                                 "This dialog also provides access to special KDE printing "
                                 "services such as creating a PDF file from the current document.</html>" ) );
  m_paPrint->setStatusTip( i18n( "Print the current document" ) );

  m_paLockView->setStatusTip( i18n("A locked view cannot change folders. Use in combination with 'link view' to explore many files from one folder") );
  m_paLinkView->setStatusTip( i18n("Sets the view as 'linked'. A linked view follows folder changes made in other linked views.") );
}

void KonqExtendedBookmarkOwner::openBookmark(const KBookmark & bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km)
{
    kDebug() << bm.url() << km << mb;

    const QString url = bm.url().url();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();
    req.forceAutoEmbed = true;

    if (km & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    if( km & Qt::ControlModifier ) { // Ctrl Left/MMB
        m_pKonqMainWindow->openFilteredUrl( url, req);
    } else if( mb & Qt::MidButton ) {
        if(KonqSettings::mmbOpensTab()) {
            m_pKonqMainWindow->openFilteredUrl( url, req);
        } else {
            const KUrl finalURL = KonqMisc::konqFilteredURL(m_pKonqMainWindow, url);
            KonqMainWindow* mw = KonqMisc::createNewWindow(finalURL);
            mw->show();
        }
    }
    else {
        m_pKonqMainWindow->openFilteredUrl( url, false );
    }
}

void KonqMainWindow::slotMoveTabLeft()
{
  if ( QApplication::isRightToLeft() )
    m_pViewManager->moveTabForward();
  else
    m_pViewManager->moveTabBackward();

  updateViewActions();
}

void KonqMainWindow::slotMoveTabRight()
{
  if ( QApplication::isRightToLeft() )
    m_pViewManager->moveTabBackward();
  else
    m_pViewManager->moveTabForward();

  updateViewActions();
}

void KonqMainWindow::updateHistoryActions()
{
    if(m_currentView)
    {
        m_paBack->setEnabled( m_currentView->canGoBack() );
        m_paForward->setEnabled( m_currentView->canGoForward() );
    }
}

void KonqMainWindow::updateToolBarActions( bool pendingAction /*=false*/)
{
  if (!m_currentView) return;

  // Enables/disables actions that depend on the current view & url (mostly toolbar)
  // Up, back, forward, the edit extension, stop button, wheel
  setUpEnabled( m_currentView->url() );
  m_paBack->setEnabled( m_currentView->canGoBack() );
  m_paForward->setEnabled( m_currentView->canGoForward() );

  if ( m_currentView->isLoading() )
  {
    startAnimation(); // takes care of m_paStop
  }
  else
  {
    m_paAnimatedLogo->stop();
    m_paStop->setEnabled( pendingAction );  //enable/disable based on any pending actions...
  }
}

void KonqMainWindow::updateViewActions()
{
  // Update actions that depend on the current view and its mode, or on the number of views etc.

  // Don't do things in this method that depend on m_currentView->url().
  // When going 'back' in history this will be called before opening the url.
  // Use updateToolBarActions instead.

  bool enable = false;

  if ( m_currentView && m_currentView->part() )
  {
    // Avoid qWarning from QObject::property if it doesn't exist
    if ( m_currentView->part()->metaObject()->indexOfProperty( "supportsUndo" ) != -1 )
    {
      QVariant prop = m_currentView->part()->property( "supportsUndo" );
      if ( prop.isValid() && prop.toBool() )
        enable = true;
    }
  }

  m_pUndoManager->updateSupportsFileUndo(enable);

//   slotUndoAvailable( m_pUndoManager->undoAvailable() );

  m_paLockView->setEnabled( true );
  m_paLockView->setChecked( m_currentView && m_currentView->isLockedLocation() );

  // Can remove view if we'll still have a main view after that
  m_paRemoveView->setEnabled( mainViewsCount() > 1 ||
                              ( m_currentView && m_currentView->isToggleView() ) );

    if ( !currentView() || !currentView()->frame())
    {
        m_paAddTab->setEnabled( false );
        m_paDuplicateTab->setEnabled( false );
        m_paRemoveOtherTabs->setEnabled( false );
        m_paBreakOffTab->setEnabled( false );
        m_paActivateNextTab->setEnabled( false );
        m_paActivatePrevTab->setEnabled( false );
        m_paMoveTabLeft->setEnabled( false );
        m_paMoveTabRight->setEnabled( false );
    } else {
        m_paAddTab->setEnabled( true );
        m_paDuplicateTab->setEnabled( true );
        KonqFrameTabs* tabContainer = m_pViewManager->tabContainer();
        bool state = (tabContainer->count()>1);
        m_paRemoveOtherTabs->setEnabled( state );
        m_paBreakOffTab->setEnabled( state );
        m_paActivateNextTab->setEnabled( state );
        m_paActivatePrevTab->setEnabled( state );

        QList<KonqFrameBase*> childFrameList = tabContainer->childFrameList();
        Q_ASSERT( !childFrameList.isEmpty() );
        m_paMoveTabLeft->setEnabled( currentView() ? currentView()->frame()!=
           (QApplication::isRightToLeft() ? childFrameList.last() : childFrameList.first()) : false );
        m_paMoveTabRight->setEnabled( currentView() ? currentView()->frame()!=
           (QApplication::isRightToLeft() ? childFrameList.first() : childFrameList.last()) : false );
    }

  // Can split a view if it's not a toggle view (because a toggle view can be here only once)
  bool isNotToggle = m_currentView && !m_currentView->isToggleView();
  m_paSplitViewHor->setEnabled( isNotToggle );
  m_paSplitViewVer->setEnabled( isNotToggle );

  m_paLinkView->setChecked( m_currentView && m_currentView->isLinkedView() );

#if 0
  if ( m_currentView && m_currentView->part() &&
       ::qobject_cast<KonqDirPart*>( m_currentView->part() ) )
  {
    KonqDirPart * dirPart = static_cast<KonqDirPart *>(m_currentView->part());
    m_paFindFiles->setEnabled( dirPart->findPart() == 0 );

    // Create the copy/move options if not already done
    // TODO: move that stuff to dolphin(part)
    if ( !m_paCopyFiles )
    {
      // F5 is the default key binding for Reload.... a la Windows.
      // mc users want F5 for Copy and F6 for move, but I can't make that default.


      m_paCopyFiles = actionCollection()->addAction( "copyfiles" );
      m_paCopyFiles->setText( i18n("Copy &Files...") );
      connect(m_paCopyFiles, SIGNAL(triggered()), SLOT(slotCopyFiles()));
      m_paCopyFiles->setShortcut(Qt::Key_F7);
      m_paMoveFiles = actionCollection()->addAction( "movefiles" );
      m_paMoveFiles->setText( i18n("M&ove Files...") );
      connect(m_paMoveFiles, SIGNAL(triggered()), SLOT(slotMoveFiles()));
      m_paMoveFiles->setShortcut(Qt::Key_F8);

      QList<QAction*> lst;
      lst.append( m_paCopyFiles );
      lst.append( m_paMoveFiles );
      m_paCopyFiles->setEnabled( false );
      m_paMoveFiles->setEnabled( false );
      plugActionList( "operations", lst );
    }
  }
  else
  {
      m_paFindFiles->setEnabled( false );

      if (m_paCopyFiles)
      {
          unplugActionList( "operations" );
          delete m_paCopyFiles;
          m_paCopyFiles = 0;
          delete m_paMoveFiles;
          m_paMoveFiles = 0;
      }
  }
#endif
}

QString KonqMainWindow::findIndexFile( const QString &dir )
{
  QDir d( dir );

  QString f = d.filePath( QLatin1String ( "index.html" ) );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( QLatin1String ( "index.htm" ) );
  if ( QFile::exists( f ) )
    return f;

  f = d.filePath( QLatin1String( "index.HTML" ) );
  if ( QFile::exists( f ) )
    return f;

  return QString();
}

void KonqMainWindow::connectExtension( KParts::BrowserExtension *ext )
{
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

  for ( ; it != itEnd ; ++it )
  {
    QAction * act = actionCollection()->action( it.key().data() );
    //kDebug() << it.key();
    if ( act )
    {
      // Does the extension have a slot with the name of this action ?
      if ( ext->metaObject()->indexOfSlot( it.key()+"()" ) != -1 )
      {
          connect( act, SIGNAL(triggered()), ext, it.value() /* SLOT(slot name) */ );
          act->setEnabled( ext->isActionEnabled( it.key() ) );
          const QString text = ext->actionText( it.key() );
          if ( !text.isEmpty() )
              act->setText( text );
          // TODO how to re-set the original action text, when switching to a part that didn't call setAction?
          // Can't test with Paste...
      } else
          act->setEnabled(false);

    } else kError() << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key();
  }

}

void KonqMainWindow::disconnectExtension( KParts::BrowserExtension *ext )
{
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
  KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

  for ( ; it != itEnd ; ++it )
  {
    QAction * act = actionCollection()->action( it.key().data() );
    //kDebug() << it.key();
    if ( act && ext->metaObject()->indexOfSlot( it.key()+"()" ) != -1 )
    {
        //kDebug() << act << act->name();
        act->disconnect( ext );
    }
  }
}

void KonqMainWindow::enableAction( const char * name, bool enabled )
{
  QAction * act = actionCollection()->action( name );
  if (!act)
    kWarning() << "Unknown action " << name << " - can't enable" ;
  else
  {
    if ( m_bLocationBarConnected && (
      act==m_paCopy || act==m_paCut || act==m_paPaste ) )
        // Don't change action state while the location bar has focus.
        return;
    //kDebug() << name << enabled;
    act->setEnabled( enabled );
  }

  // Update "copy files" and "move files" accordingly
  if (m_paCopyFiles && !strcmp( name, "copy" ))
  {
    m_paCopyFiles->setEnabled( enabled );
  }
  else if (m_paMoveFiles && !strcmp( name, "cut" ))
  {
    m_paMoveFiles->setEnabled( enabled );
  }
}

void KonqMainWindow::setActionText(const char * name, const QString& text)
{
    QAction * act = actionCollection()->action(name);
    if (!act) {
        kWarning() << "Unknown action " << name << "- can't enable" ;
    } else {
        //kDebug() << name << " text=" << text;
        act->setText(text);
    }
}

void KonqMainWindow::setProfileConfig(const KConfigGroup& cfg)
{
    // Read toolbar settings and window size from profile, and autosave into that profile from now on
    setAutoSaveSettings(cfg);
    currentProfileChanged();
}

void KonqMainWindow::currentProfileChanged()
{
    m_paSaveViewProfile->setEnabled(!m_pViewManager->currentProfile().isEmpty());
}

void KonqMainWindow::enableAllActions( bool enable )
{
    //kDebug() << enable;
  KParts::BrowserExtension::ActionSlotMap * actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();

  const QList<QAction *> actions = actionCollection()->actions();
  QList<QAction *>::ConstIterator it = actions.constBegin();
  QList<QAction *>::ConstIterator end = actions.constEnd();
  for (; it != end; ++it )
  {
    QAction *act = *it;
    if ( !act->objectName().startsWith("options_configure") /* do not touch the configureblah actions */
         && ( !enable || !actionSlotMap->contains( act->objectName().toLatin1() ) ) ) /* don't enable BE actions */
      act->setEnabled( enable );
  }
  // This method is called with enable=false on startup, and
  // then only once with enable=true when the first view is setup.
  // So the code below is where actions that should initially be disabled are disabled.
  if (enable)
  {
      setUpEnabled( m_currentView ? m_currentView->url() : KUrl() );
      // we surely don't have any history buffers at this time
      m_paBack->setEnabled( false );
      m_paForward->setEnabled( false );

      // Load profile submenu
      m_pViewManager->profileListDirty( false );

      currentProfileChanged();

      updateViewActions(); // undo, lock, link and other view-dependent actions
      updateClosedItemsAction();

      m_paStop->setEnabled( m_currentView && m_currentView->isLoading() );

      if (m_toggleViewGUIClient)
      {
          QList<QAction*> actions = m_toggleViewGUIClient->actions();
          for (int i = 0; i < actions.size(); ++i) {
            actions.at(i)->setEnabled( true );
          }
      }

  }
  actionCollection()->action( "quit" )->setEnabled( true );
  actionCollection()->action( "link" )->setEnabled( false );
}

void KonqMainWindow::disableActionsNoView()
{
    // No view -> there are some things we can't do
    m_paUp->setEnabled( false );
    m_paReload->setEnabled( false );
    m_paReloadAllTabs->setEnabled( false );
    m_paBack->setEnabled( false );
    m_paForward->setEnabled( false );
    m_paLockView->setEnabled( false );
    m_paLockView->setChecked( false );
    m_paSplitViewVer->setEnabled( false );
    m_paSplitViewHor->setEnabled( false );
    m_paRemoveView->setEnabled( false );
    m_paLinkView->setEnabled( false );
    if (m_toggleViewGUIClient)
    {
        QList<QAction*> actions = m_toggleViewGUIClient->actions();
        for (int i = 0; i < actions.size(); ++i) {
            actions.at(i)->setEnabled( false );
        }
    }
    // There are things we can do, though : bookmarks, view profile, location bar, new window,
    // settings, etc.
    static const char* const s_enActions[] = { "new_window", "duplicate_window", "open_location",
                                         "toolbar_url_combo", "clear_location", "animated_logo",
                                         "konqintro", "go_most_often", "go_applications",
                                         "go_trash", "go_settings", "go_network_folders", "go_autostart",
                                         "go_url", "go_media", "go_history", "options_configure_extensions", 0 };
    for ( int i = 0 ; s_enActions[i] ; ++i )
    {
        QAction * act = action(s_enActions[i]);
        if (act)
            act->setEnabled( true );
    }
    m_pamLoadViewProfile->setEnabled( true );
    m_paSaveViewProfile->setEnabled( true );
    m_paSaveRemoveViewProfile->setEnabled( true );
    m_combo->clearTemporary();
}

void KonqMainWindow::setCaption( const QString &caption )
{
    // KParts sends us empty captions when activating a brand new part
    // We can't change it there (in case of apps removing all parts altogether)
    // but here we never do that.
    if (!caption.isEmpty() && m_currentView) {
        //kDebug() << caption;

        // Keep an unmodified copy of the caption (before squeezing and KComponentData::makeStdCaption are applied)
        m_currentView->setCaption(caption);
        KParts::MainWindow::setCaption(KStringHandler::csqueeze(m_currentView->caption(), 128));
    }
}

void KonqMainWindow::showEvent(QShowEvent *event)
{
  // We need to check if our toolbars are shown/hidden here, and set
  // our menu items accordingly. We can't do it in the constructor because
  // view profiles store toolbar info, and that info is read after
  // construct time.
  m_paShowMenuBar->setChecked( !menuBar()->isHidden() );
  if ( m_currentView )
    m_paShowStatusBar->setChecked( m_currentView->frame()->statusbar()->isVisible() );
  updateBookmarkBar(); // hide if empty

  // Call parent method
  KParts::MainWindow::showEvent(event);
}

QString KonqExtendedBookmarkOwner::currentUrl() const
{
   const KonqView* view = m_pKonqMainWindow->currentView();
   return view ? view->url().url() : QString();
}

QString KonqMainWindow::currentProfile() const
{
   return m_pViewManager->currentProfile();
}

QString KonqMainWindow::currentURL() const
{
    if ( !m_currentView )
        return QString();
    QString url = m_currentView->url().prettyUrl();

#if 0 // do we want this?
    // Add the name filter (*.txt) at the end of the URL again
    if ( m_currentView->part() ) {
        const QString nameFilter = m_currentView->nameFilter();
        if (!nameFilter.isEmpty()) {
            if (!url.endsWith('/'))
                url += '/';
            url += nameFilter;
        }
    }
#endif
    return url;
}

bool KonqExtendedBookmarkOwner::supportsTabs() const
{
  return true;
}

QList<QPair<QString, QString> > KonqExtendedBookmarkOwner::currentBookmarkList() const
{
  QList<QPair<QString, QString> > list;
  KonqFrameTabs* tabContainer = m_pKonqMainWindow->viewManager()->tabContainer();

  foreach ( KonqFrameBase* frame, tabContainer->childFrameList() )
  {
    if ( !frame || !frame->activeChildView() )
      continue;
    if( frame->activeChildView()->locationBarURL().isEmpty() )
      continue;
    list << qMakePair( frame->activeChildView()->caption(),
                       frame->activeChildView()->url().url() );
  }
  return list;
}

QString KonqExtendedBookmarkOwner::currentTitle() const
{
   return m_pKonqMainWindow->currentTitle();
}

void KonqExtendedBookmarkOwner::openInNewTab(const KBookmark &bm)
{
  bool newTabsInFront = KonqSettings::newTabsInFront();
  if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    newTabsInFront = !newTabsInFront;

  KonqOpenURLRequest req;
  req.browserArgs.setNewTab(true);
  req.newTabInFront = newTabsInFront;
  req.openAfterCurrentPage = false;
  req.forceAutoEmbed = true;

  m_pKonqMainWindow->openFilteredUrl(bm.url().url(), req);
}

void KonqExtendedBookmarkOwner::openFolderinTabs(const KBookmarkGroup &grp)
{
  bool newTabsInFront = KonqSettings::newTabsInFront();
  if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    newTabsInFront = !newTabsInFront;
  KonqOpenURLRequest req;
  req.browserArgs.setNewTab(true);
  req.newTabInFront = false;
  req.openAfterCurrentPage = false;
  req.forceAutoEmbed = true;

  const QList<KUrl> list = grp.groupUrlList();
  if (list.isEmpty())
    return;

  if (list.size() > 20) {
    if(KMessageBox::questionYesNo(m_pKonqMainWindow,
				  i18n("You have requested to open more than 20 bookmarks in tabs. "
                                       "This might take a while. Continue?"),
				  i18nc("@title:window", "Open bookmarks folder in new tabs")) != KMessageBox::Yes)
      return;
  }

  QList<KUrl>::ConstIterator it = list.constBegin();
  QList<KUrl>::ConstIterator end = list.constEnd();
  --end;
  for (; it != end; ++it )
  {
    m_pKonqMainWindow->openFilteredUrl((*it).url(), req);
  }
  if ( newTabsInFront )
  {
    req.newTabInFront = true;
  }
  m_pKonqMainWindow->openFilteredUrl((*end).url(), req);
}

void KonqExtendedBookmarkOwner::openInNewWindow(const KBookmark &bm)
{
    const KUrl finalURL (KonqMisc::konqFilteredURL(m_pKonqMainWindow, bm.url().url()));
    KonqMainWindow* mw = KonqMisc::createNewWindow(finalURL);
    mw->show();
}

QString KonqMainWindow::currentTitle() const
{
  return m_currentView ? m_currentView->caption() : QString();
}

void KonqMainWindow::slotPopupMenu( const QPoint &global, const KUrl &url, mode_t mode, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& actionGroups )
{
    KFileItem item( url, args.mimeType(), mode );
    KFileItemList items;
    items.append( item );
    slotPopupMenu( global, items, args, browserArgs, flags, actionGroups );
}

void KonqMainWindow::slotPopupMenu( const QPoint &global, const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags itemFlags, const KParts::BrowserExtension::ActionGroupMap& _actionGroups )
{
    KParts::BrowserExtension::ActionGroupMap actionGroups = _actionGroups;

    KonqView * m_oldView = m_currentView;
    KonqView * currentView = childView( static_cast<KParts::ReadOnlyPart *>( sender()->parent() ) );

    //kDebug() << "m_oldView=" << m_oldView << "new currentView=" << currentView << "passive:" << currentView->isPassiveMode();

  if ( (m_oldView != currentView) && currentView->isPassiveMode() )
  {
      // Make this view active only temporarily (because it's passive)
      m_currentView = currentView;

      if ( m_oldView && m_oldView->browserExtension() )
          disconnectExtension( m_oldView->browserExtension() );
      if ( m_currentView->browserExtension() )
          connectExtension( m_currentView->browserExtension() );
  }
  // Note that if m_oldView!=currentView and currentView isn't passive,
  // then the KParts mechanism has already noticed the click in it,
  // but KonqViewManager delays the GUI-rebuilding with a single-shot timer.
  // Right after the popup shows up, currentView _will_ be m_currentView.

  //kDebug() << "current view=" << m_currentView << m_currentView->part()->metaObject()->className();

  // This action collection is used to pass actions to KonqPopupMenu.
  // It has to be a KActionCollection instead of a KActionPtrList because we need
  // the actionStatusText signal...
  KActionCollection popupMenuCollection( static_cast<QWidget*>(0) );

  // m_paBack is a submenu, for the toolbar & edit menu "back",
  // but in the RMB we want a simple one-click back instead.
  KAction* simpleBack = KStandardAction::back( this, SLOT(slotBack()), &popupMenuCollection );
  simpleBack->setEnabled( m_paBack->isEnabled() );
  popupMenuCollection.addAction( "go_back", simpleBack );

  KAction* simpleForward = KStandardAction::forward( this, SLOT(slotForward()), &popupMenuCollection );
  simpleForward->setEnabled( m_paForward->isEnabled() );
  popupMenuCollection.addAction( "go_forward", simpleForward );

  popupMenuCollection.addAction( "go_up", m_paUp );
  popupMenuCollection.addAction( "reload", m_paReload );
  popupMenuCollection.addAction( "closeditems", m_paClosedItems );

#if 0
  popupMenuCollection.addAction( "find", m_paFindFiles );
#endif

  popupMenuCollection.addAction( "undo", m_paUndo );
  popupMenuCollection.addAction( "cut", m_paCut );
  popupMenuCollection.addAction( "copy", m_paCopy );
  popupMenuCollection.addAction( "paste", m_paPaste );

  // The pasteto action is used when clicking on a dir, to paste into it.
  KAction *actPaste = KStandardAction::paste( this, SLOT(slotPopupPasteTo()), this );
  actPaste->setEnabled( m_paPaste->isEnabled() );
  popupMenuCollection.addAction( "pasteto", actPaste );

  prepareForPopupMenu(items, args, browserArgs);

  bool sReading = false;
  if (!m_popupUrl.isEmpty()) {
      sReading = KProtocolManager::supportsReading(m_popupUrl);
  }

  // Don't set the view URL for a toggle view.
  // (This is a bit of a hack for the directory tree....)
  // ## should use the new currentView->isHierarchicalView() instead?
  // Would this be correct for the konqlistview tree view?
  KUrl viewURL = currentView->isToggleView() ? KUrl() : currentView->url();
  kDebug() << "viewURL=" << viewURL;

  bool openedForViewURL = false;
  //bool dirsSelected = false;
  bool devicesFile = false;

    if ( items.count() == 1 )
    {
      const KUrl firstURL = items.first().url();
      if ( !viewURL.isEmpty() )
      {
	  //firstURL.cleanPath();
          openedForViewURL = firstURL.equals( viewURL, KUrl::CompareWithoutTrailingSlash );
      }
      devicesFile = firstURL.protocol().indexOf("device", 0, Qt::CaseInsensitive) == 0;
      //dirsSelected = S_ISDIR( items.first()->mode() );
    }
    //kDebug() << "viewURL=" << viewURL;

    KUrl url = viewURL;
    url.cleanPath();
    bool isIntoTrash = url.protocol() == "trash" || url.url().startsWith( "system:/trash" );
    const bool doTabHandling = !openedForViewURL && !isIntoTrash && sReading;
    const bool showEmbeddingServices = items.count() == 1 && !m_popupMimeType.isEmpty() &&
                                       !isIntoTrash && !devicesFile &&
                                       (itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) == 0;

    KService::List embeddingServices;
    if ( showEmbeddingServices ) {
        const QString currentServiceName = currentView->service()->desktopEntryName();

        // List of services for the "Preview In" submenu.
        embeddingServices = KMimeTypeTrader::self()->query(
            m_popupMimeType,
            "KParts/ReadOnlyPart",
            // Obey "HideFromMenus". It defaults to false so we want "absent or true"
            // (wow, testing for 'true' if absent doesn't work, so order matters)
            "(not exist [X-KDE-BrowserView-HideFromMenus] or not [X-KDE-BrowserView-HideFromMenus]) "
            "and DesktopEntryName != '"+currentServiceName+"' "
            // I had an old local dirtree.desktop without lib, no need for invalid entries
            "and exist [Library]");
    }

    PopupMenuGUIClient *konqyMenuClient = new PopupMenuGUIClient( embeddingServices,
                                                                  actionGroups,
                                                                  !menuBar()->isVisible() ? m_paShowMenuBar : 0,
                                                                  fullScreenMode() ? m_ptaFullScreen : 0
        );
    qRegisterMetaType<KService::Ptr>("KService::Ptr");
    connect(konqyMenuClient, SIGNAL(openEmbedded(KService::Ptr)),
            this, SLOT(slotOpenEmbedded(KService::Ptr)), Qt::QueuedConnection);


    // Those actions go into the PopupMenuGUIClient, since that's the one defining them.
    QList<QAction *> tabHandlingActions;
    if( doTabHandling )
    {
        if (browserArgs.forcesNewWindow()) {
            QAction* act = konqyMenuClient->actionCollection()->addAction( "sameview" );
            act->setText( i18n( "Open in T&his Window" ) );
            act->setStatusTip( i18n( "Open the document in current window" ) );
            connect(act, SIGNAL(triggered()), SLOT(slotPopupThisWindow()));
            tabHandlingActions.append(act);
        }
        KAction* actNewWindow = konqyMenuClient->actionCollection()->addAction( "newview" );
        actNewWindow->setIcon( KIcon("window-new") );
        actNewWindow->setText( i18n( "Open in New &Window" ) );
        actNewWindow->setStatusTip( i18n( "Open the document in a new window" ) );
        connect(actNewWindow, SIGNAL(triggered()), SLOT(slotPopupNewWindow()));
        tabHandlingActions.append(actNewWindow);

        KAction* actNewTab = konqyMenuClient->actionCollection()->addAction( "openintab" );
        actNewTab->setIcon( KIcon("tab-new") );
        actNewTab->setText( i18n( "Open in &New Tab" ) );
        connect(actNewTab, SIGNAL(triggered()), SLOT(slotPopupNewTab()));
        actNewTab->setStatusTip( i18n( "Open the document in a new tab" ) );
        tabHandlingActions.append(actNewTab);

        KAction* separator = new KAction(konqyMenuClient->actionCollection());
        separator->setSeparator(true);
        tabHandlingActions.append(separator);
    }

    if (currentView->isHierarchicalView())
        itemFlags |= KParts::BrowserExtension::ShowCreateDirectory;

    KonqPopupMenu::Flags kpf = 0;

    if (doTabHandling)
        actionGroups.insert("tabhandling", tabHandlingActions);
    //if (kpf & KonqPopupMenu::IsLink)
    //    actionGroups.insert("linkactions", linkActions);
    // TODO actionGroups.insert("partactions", partActions);

    QPointer<KonqPopupMenu> pPopupMenu = new KonqPopupMenu(
        items,
        viewURL,
        popupMenuCollection,
        m_pMenuNew,
        kpf,
        itemFlags,
        // This parent ensures that if the part destroys itself (e.g. KHTML redirection),
        // it will close the popupmenu
        currentView->part()->widget(),
        s_bookmarkManager,
        actionGroups );

    if ( openedForViewURL && !viewURL.isLocalFile() )
        pPopupMenu->setURLTitle( currentView->caption() );

    QPointer<KParts::BrowserExtension> be = ::qobject_cast<KParts::BrowserExtension *>(sender());

    if (be) {
        QObject::connect( this, SIGNAL(popupItemsDisturbed()), pPopupMenu, SLOT(close()) );
        QObject::connect( be, SIGNAL(itemsRemoved(KFileItemList)),
                          this, SLOT(slotItemsRemoved(KFileItemList)) );
    }


  QPointer<QObject> guard( this ); // #149736, window could be deleted inside popupmenu event loop
  pPopupMenu->exec( global );

  delete pPopupMenu;

  // We're sort of misusing KActionCollection here, but we need it for the actionStatusText signal...
  // Anyway. If the action belonged to the view, and the view got deleted, we don't want ~KActionCollection
  // to iterate over those deleted actions
  /*KActionPtrList lst = popupMenuCollection.actions();
  KActionPtrList::iterator it = lst.begin();
  for ( ; it != lst.end() ; ++it )
      popupMenuCollection.take( *it );*/

  if ( guard.isNull() ) // the placement of this test is very important, double-check #149736 if moving stuff around
      return;

  if (be) {
    QObject::disconnect( be, SIGNAL(itemsRemoved(KFileItemList)),
                         this, SLOT(slotItemsRemoved(KFileItemList)) );
  }

  delete konqyMenuClient;
  m_popupItems.clear();

  // Deleted by konqyMenuClient's actioncollection
  //delete actNewTab;
  //delete actNewWindow;

  delete actPaste;

  // Restore current view if current is passive
  if ( (m_oldView != currentView) && (currentView == m_currentView) && currentView->isPassiveMode() )
  {
    //kDebug() << "restoring active view" << m_oldView;
    if ( m_currentView->browserExtension() )
      disconnectExtension( m_currentView->browserExtension() );
    if ( m_oldView )
    {
        if ( m_oldView->browserExtension() )
        {
            connectExtension( m_oldView->browserExtension() );
            m_currentView = m_oldView;
        }
        // Special case: RMB + renaming in sidebar; setFocus would abort editing.
        QWidget* fw = focusWidget();
        if ( !fw || !::qobject_cast<QLineEdit*>( fw ) ) {
            m_oldView->part()->widget()->setFocus();
            m_pViewManager->setActivePart(m_oldView->part());
        }
    }
  }
}

void KonqMainWindow::prepareForPopupMenu(const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments& browserArgs)
{
    if (!items.isEmpty()) {
        m_popupUrl = items.first().url();
        m_popupMimeType = items.first().mimetype();
    }
    else
    {
        m_popupUrl = KUrl();
        m_popupMimeType.clear();
    }
    // We will need these if we call the newTab slot
    m_popupItems = items;
    m_popupUrlArgs = args;
    m_popupUrlArgs.setMimeType( QString() ); // Reset so that Open in New Window/Tab does mimetype detection
    m_popupUrlBrowserArgs = browserArgs;
}

void KonqMainWindow::slotItemsRemoved(const KFileItemList &items)
{
    QListIterator<KFileItem> it(items);
    while (it.hasNext()) {
        if (m_popupItems.contains(it.next())) {
            emit popupItemsDisturbed();
            return;
        }
    }
}

Q_DECLARE_METATYPE(KService::Ptr)
void KonqMainWindow::slotOpenEmbedded(KService::Ptr service)
{
    if (!m_currentView) return;

    m_currentView->stop();
    m_currentView->setLocationBarURL(m_popupUrl);
    m_currentView->setTypedURL(QString());
    if ( m_currentView->changePart( m_popupMimeType,
                                        service->desktopEntryName(), true ) )
        m_currentView->openUrl( m_popupUrl, m_popupUrl.pathOrUrl() );
}

void KonqMainWindow::slotPopupPasteTo()
{
    if ( !m_currentView || m_popupUrl.isEmpty() )
        return;
    m_currentView->callExtensionURLMethod( "pasteTo", m_popupUrl );
}

void KonqMainWindow::slotReconfigure()
{
  reparseConfiguration();
}

void KonqMainWindow::reparseConfiguration()
{
  kDebug();

  KonqSettings::self()->readConfig();
  m_pViewManager->applyConfiguration();
  KonqMouseEventFilter::self()->reparseConfiguration();

  if (m_combo)
      m_combo->setFont( KGlobalSettings::generalFont() );

  MapViews::ConstIterator it = m_mapViews.constBegin();
  MapViews::ConstIterator end = m_mapViews.constEnd();
  for (; it != end; ++it )
      (*it)->reparseConfiguration();
}

void KonqMainWindow::saveProperties( KConfigGroup& config )
{
    // Ensure no crash if the sessionmanager timer fires before the ctor is done
    // This can happen via ToggleViewGUIClient -> KServiceTypeTrader::query
    // -> KSycoca running kbuildsycoca -> nested event loop.
    if (m_fullyConstructed) {
        KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
        m_pViewManager->saveViewProfileToGroup(config, flags);
    }
}

void KonqMainWindow::readProperties( const KConfigGroup& configGroup )
{
    // ######### THIS CANNOT WORK. It's too late to change the xmlfile, the GUI has been built already!
    // We need to delay doing setXMLFile+createGUI until we know which profile we are going to use, then...
    // TODO: Big refactoring needed for this feature. On the other hand, all default profiles shipped with
    // konqueror use konqueror.rc again, so the need for this is almost zero now.
    const QString xmluiFile = configGroup.readEntry("XMLUIFile","konqueror.rc");
    setXMLFile( KonqViewManager::normalizedXMLFileName(xmluiFile) );

    m_pViewManager->loadViewProfileFromGroup( configGroup, QString() /*no profile name*/ );
    // read window settings
    applyMainWindowSettings( configGroup, true );
}

void KonqMainWindow::setInitialFrameName( const QString &name )
{
  m_initialFrameName = name;
}

void KonqMainWindow::updateOpenWithActions()
{
  unplugActionList( "openwithbase" );
  unplugActionList( "openwith" );

  qDeleteAll(m_openWithActions);
  m_openWithActions.clear();

  delete m_openWithMenu;
  m_openWithMenu = 0;

  if (!KAuthorized::authorizeKAction("openwith"))
     return;

  m_openWithMenu = new KActionMenu( i18n("&Open With"), this );

  const KService::List & services = m_currentView->appServiceOffers();
  KService::List::ConstIterator it = services.constBegin();
  const KService::List::ConstIterator end = services.constEnd();

  const int baseOpenWithItems = std::max(KonqSettings::openWithItems(), 0);

  int idxService = 0;
  for (; it != end; ++it, ++idxService)
  {
    KAction *action;

    if (idxService < baseOpenWithItems)
       action = new KAction(i18n("Open with %1", (*it)->name()), this);
    else
       action = new KAction((*it)->name(), this);
    action->setIcon( KIcon( (*it)->icon() ) );

    connect( action, SIGNAL(triggered()),
             this, SLOT(slotOpenWith()) );

    actionCollection()->addAction((*it)->desktopEntryName(), action);
    if (idxService < baseOpenWithItems)
        m_openWithActions.append(action);
    else
        m_openWithMenu->addAction(action);
  }

  if ( services.count() > 0 )
  {
      plugActionList("openwithbase", m_openWithActions);
      QList<QAction*> openWithActionsMenu;
      if (idxService > baseOpenWithItems)
      {
          openWithActionsMenu.append(m_openWithMenu);
      }
      KAction *sep = new KAction( this );
      sep->setSeparator( true );
      openWithActionsMenu.append(sep);
      plugActionList("openwith", openWithActionsMenu);
  }
}

void KonqMainWindow::updateViewModeActions()
{
    unplugViewModeActions();
    Q_FOREACH( QAction* action, m_viewModesGroup->actions() ) {
        Q_FOREACH( QWidget *w, action->associatedWidgets() )
            w->removeAction( action );
        delete action;
    }

    delete m_viewModeMenu;
    m_viewModeMenu = 0;

    const KService::List services = m_currentView->partServiceOffers();
    if ( services.count() <= 1 )
        return;

    m_viewModeMenu = new KActionMenu( i18nc("@action:inmenu View", "&View Mode"), this );
    //actionCollection()->addAction( "viewModeMenu", m_viewModeMenu );

    KService::List::ConstIterator it = services.constBegin();
    const KService::List::ConstIterator end = services.constEnd();
    for (; it != end; ++it) {
        const KService::Ptr service = *it;
        const QVariant propToggable = service->property( "X-KDE-BrowserView-Toggable" );
        const bool toggable = propToggable.isValid() && propToggable.toBool();
        const QVariant propHierarchical = service->property( "X-KDE-BrowserView-HierarchicalView" );
        const bool hierarchical = propHierarchical.isValid() && propHierarchical.toBool();
        // No hierarchical toggable views in view mode (i.e. konsole ok, but no sidebar)
        if (toggable && hierarchical)
            continue;

        const QString desktopEntryName = service->desktopEntryName();
        bool bIsCurrentView = desktopEntryName == m_currentView->service()->desktopEntryName();

        const QList<KServiceAction> actions = service->actions();
        if (!actions.isEmpty()) {

            // The service provides several view modes, like DolphinPart
            // -> create one action per view mode
            Q_FOREACH(const KServiceAction& serviceAction, actions) {
                // Create a KToggleAction for each view mode, and plug it into the menu
                KToggleAction* action = new KToggleAction(KIcon(serviceAction.icon()), serviceAction.text(), this);
                //actionCollection()->addAction(desktopEntryName /*not unique!*/, action);
                action->setObjectName(desktopEntryName + QLatin1String("-viewmode"));
                action->setData(QVariant(serviceAction.name()));
                action->setActionGroup(m_viewModesGroup);
                m_viewModeMenu->menu()->addAction(action);
                if (bIsCurrentView && m_currentView->internalViewMode() == serviceAction.name()) {
                    action->setChecked(true);
                }
            }

        } else {
            // The service only provides one view mode (common case)

            QString serviceText = service->genericName();
            if (serviceText.isEmpty())
                serviceText = service->name();

            // Create a KToggleAction for this view mode, and plug it into the menu
            KToggleAction* action = new KToggleAction(KIcon(service->icon()), serviceText, this);
            // NOTE: "-viewmode" is appended to desktopEntryName to avoid overwritting existing
            // action, e.g. konsolepart added through ToggleViewGUIClient in the ctor will be
            // overwritten by the view mode konsolepart action added here.  #266517.
            actionCollection()->addAction(desktopEntryName + QLatin1String("-viewmode"), action);
            action->setActionGroup(m_viewModesGroup);
            m_viewModeMenu->menu()->addAction(action);

            action->setChecked(bIsCurrentView);
        }
    }

    // No view mode for actions toggable views
    // (The other way would be to enforce a better servicetype for them, than Browser/View)
    if (!m_currentView->isToggleView()
        /* already tested: && services.count() > 1 */
        && m_viewModeMenu) {
        plugViewModeActions();
    }
}

void KonqMainWindow::slotInternalViewModeChanged()
{
    KParts::ReadOnlyPart *part = static_cast<KParts::ReadOnlyPart *>(sender());
    KonqView * view = m_mapViews.value(part);
    if (view) {
        const QString actionName = view->service()->desktopEntryName();
        const QString actionData = view->internalViewMode();
        Q_FOREACH(QAction* action, m_viewModesGroup->actions()) {
            if (action->objectName() == actionName + QLatin1String("-viewmode") &&
                action->data().toString() == actionData) {
                action->setChecked(true);
                break;
            }
        }
    }
}

void KonqMainWindow::plugViewModeActions()
{
    QList<QAction*> lst;

    if (m_viewModeMenu) {
        lst.append( m_viewModeMenu );
    }

    plugActionList( "viewmode", lst );
}

void KonqMainWindow::unplugViewModeActions()
{
    unplugActionList( "viewmode" );
}

void KonqMainWindow::updateBookmarkBar()
{
    KToolBar * bar = qFindChild<KToolBar *>(this, "bookmarkToolBar");
    if (!bar) return;
    if (m_paBookmarkBar && bar->actions().isEmpty() )
        bar->hide();

}

void KonqMainWindow::closeEvent( QCloseEvent *e )
{
    // This breaks session management (the window is withdrawn in kwin)
    // so let's do this only when closed by the user.

    // kapp is 0 in unit tests
    if (kapp && !kapp->sessionSaving()) {
      KonqFrameTabs* tabContainer = m_pViewManager->tabContainer();
      if ( tabContainer->count() > 1 )
      {
        KSharedConfig::Ptr config = KGlobal::config();
        KConfigGroup cs( config, QLatin1String("Notification Messages") );

        if ( !cs.hasKey( "MultipleTabConfirm" ) )
        {
          switch (
              KMessageBox::warningYesNoCancel(
                  this,
                  i18n("You have multiple tabs open in this window, "
                        "are you sure you want to quit?"),
                  i18nc("@title:window", "Confirmation"),
                  KStandardGuiItem::closeWindow(),
                  KGuiItem(i18n( "C&lose Current Tab" ), "tab-close"),
                  KStandardGuiItem::cancel(),
                  "MultipleTabConfirm"
              )
            ) {
            case KMessageBox::Yes :
            break;
            case KMessageBox::No :
            {
              e->ignore();
              slotRemoveTab();
              return;
            }
            break;
            case KMessageBox::Cancel :
            {
              e->ignore();
    	      return;
            }
          }
        }
      }

      const int originalTabIndex = tabContainer->currentIndex();
      for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
          KonqFrameBase* tab = tabContainer->tabAt(tabIndex);
          if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
              m_pViewManager->showTab(tabIndex);
              const QString question = m_pViewManager->isTabBarVisible()
                                     ? i18n("This tab contains changes that have not been submitted.\nClosing the window will discard these changes.")
                                     : i18n("This page contains changes that have not been submitted.\nClosing the window will discard these changes.");
              if ( KMessageBox::warningContinueCancel(
                       this, question,
                       i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"),"application-exit"),
                       KStandardGuiItem::cancel(), "discardchangesclose") != KMessageBox::Continue ) {
                  e->ignore();
                  m_pViewManager->showTab(originalTabIndex);
                  return;
              }
          }
      }

      if (settingsDirty() && autoSaveSettings())
          saveAutoSaveSettings();

      addClosedWindowToUndoList();

      hide();
      qApp->flush();
  }
  // We're going to close - tell the parts
  MapViews::ConstIterator it = m_mapViews.constBegin();
  MapViews::ConstIterator end = m_mapViews.constEnd();
  for (; it != end; ++it )
  {
      if ( (*it)->part() && (*it)->part()->widget() )
          QApplication::sendEvent( (*it)->part()->widget(), e );
  }
  KParts::MainWindow::closeEvent( e );
  if (kapp && !kapp->sessionSaving() && stayPreloaded()) {
      e->ignore();
      hide();
  }
}

void KonqMainWindow::addClosedWindowToUndoList()
{
    kDebug();

    // 1. We get the current title
    int numTabs = m_pViewManager->tabContainer()->childFrameList().count();
    QString title( i18n("no name") );

    if(m_currentView)
        title = m_currentView->caption();

    // 2. Create the KonqClosedWindowItem and  save its config
    KonqClosedWindowItem* closedWindowItem = new KonqClosedWindowItem(title, m_pUndoManager->newCommandSerialNumber(), numTabs);
    saveProperties( closedWindowItem->configGroup() );

    // 3. Add the KonqClosedWindowItem to the undo list
    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedWindowItem( closedWindowItem );

    kDebug() << "done";
}

void KonqMainWindow::updateWindowIcon()
{
    const QString url = m_combo->currentText();
    // TODO (kfm-devel email 13/09/2008) should we use a profile-defined icon instead of a url-dependent window icon?
    const QPixmap pix = KonqPixmapProvider::self()->pixmapFor( url );
    KParts::MainWindow::setWindowIcon( pix );

    QPixmap big = pix;
    if (!url.isEmpty()) {
       big = KonqPixmapProvider::self()->pixmapFor( url, KIconLoader::SizeMedium );
    }

    KWindowSystem::setIcons( winId(), big, pix );
}

void KonqMainWindow::slotIntro()
{
  openUrl( 0, KUrl("about:") );
}

void KonqMainWindow::goURL()
{
  QLineEdit *lineEdit = comboEdit();
  if (!lineEdit) {
    return;
  }

  QKeyEvent event( QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QChar('\n') );
  QApplication::sendEvent( lineEdit, &event );
}

/**
 * Adds the URL of a KonqView to the closed tabs list.
 * This slot gets called each time a View is closed.
 */
void KonqMainWindow::slotAddClosedUrl(KonqFrameBase *tab)
{
    kDebug();
    QString title( i18n("no name") ), url("about:blank");

    // Did the tab contain a single frame, or a splitter?
    KonqFrame* frame = dynamic_cast<KonqFrame *>(tab);
    if (!frame) {
        KonqFrameContainer* frameContainer = dynamic_cast<KonqFrameContainer *>(tab);
        if (frameContainer->activeChildView())
            frame = frameContainer->activeChildView()->frame();
    }

    KParts::ReadOnlyPart* part = frame ? frame->part() : 0;
    if (part)
        url = part->url().url();
    if (frame)
        title = frame->title().trimmed();
    if (title.isEmpty())
        title = url;
    title = KStringHandler::csqueeze( title, 50 );

    // Now we get the position of the tab
    const int index =  m_pViewManager->tabContainer()->childFrameList().indexOf(tab);

    KonqClosedTabItem* closedTabItem = new KonqClosedTabItem(url, title, index, m_pUndoManager->newCommandSerialNumber());

    QString prefix = KonqFrameBase::frameTypeToString( tab->frameType() ) + QString::number(0);
    closedTabItem->configGroup().writeEntry( "RootItem", prefix );
    prefix.append( QLatin1Char( '_' ) );
    KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
    tab->saveConfig( closedTabItem->configGroup(), prefix, flags, 0L, 0, 1);

    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedTabItem( closedTabItem );

    kDebug() << "done";
}

void KonqMainWindow::slotLocationLabelActivated()
{
  focusLocationBar();
  QLineEdit* edit = comboEdit();
  if (edit) {
    edit->selectAll();
  }
}

void KonqMainWindow::slotOpenURL( const KUrl& url )
{
    openUrl( 0, url );
}

bool KonqMainWindow::sidebarVisible() const
{
    QAction *a = m_toggleViewGUIClient->action("konq_sidebartng");
    return (a && static_cast<KToggleAction*>(a)->isChecked());
}

bool KonqMainWindow::fullScreenMode() const
{
    return m_ptaFullScreen->isChecked();
}

void KonqMainWindow::slotAddWebSideBar(const KUrl& url, const QString& name)
{
    if (url.url().isEmpty() && name.isEmpty())
        return;

    kDebug() << "Requested to add URL" << url << " [" << name << "] to the sidebar!";

    QAction *a = m_toggleViewGUIClient->action("konq_sidebartng");
    if (!a) {
        KMessageBox::sorry(0, i18n("Your sidebar is not functional or unavailable. A new entry cannot be added."), i18nc("@title:window", "Web Sidebar"));
        return;
    }

    int rc = KMessageBox::questionYesNo(0,
              i18n("Add new web extension \"%1\" to your sidebar?",
                                 name.isEmpty() ? name : url.prettyUrl()),
              i18nc("@title:window", "Web Sidebar"),KGuiItem(i18n("Add")),KGuiItem(i18n("Do Not Add")));

    if (rc == KMessageBox::Yes) {
        // Show the sidebar
        if (!static_cast<KToggleAction*>(a)->isChecked()) {
            a->trigger();
        }

        // Tell it to add a new panel
        MapViews::ConstIterator it;
        for (it = viewMap().constBegin(); it != viewMap().constEnd(); ++it) {
            KonqView *view = it.value();
            if (view) {
                KService::Ptr svc = view->service();
                if (svc->desktopEntryName() == "konq_sidebartng") {
                    emit view->browserExtension()->addWebSideBar(url, name);
                    break;
                }
            }
        }
    }
}

void KonqMainWindow::bookmarksIntoCompletion( const KBookmarkGroup& group )
{
    static const QString& http = KGlobal::staticQString( "http" );
    static const QString& ftp = KGlobal::staticQString( "ftp" );

    if ( group.isNull() )
        return;

    for ( KBookmark bm = group.first();
          !bm.isNull(); bm = group.next(bm) ) {
        if ( bm.isGroup() ) {
            bookmarksIntoCompletion( bm.toGroup() );
            continue;
        }

        KUrl url = bm.url();
        if ( !url.isValid() )
            continue;

        QString u = url.prettyUrl();
        s_pCompletion->addItem( u );

        if ( url.isLocalFile() )
            s_pCompletion->addItem( url.toLocalFile() );
        else if ( url.protocol() == http )
            s_pCompletion->addItem( u.mid( 7 ));
        else if ( url.protocol() == ftp &&
                  url.host().startsWith( ftp ) )
            s_pCompletion->addItem( u.mid( 6 ) );
    }
}

//
// the smart popup completion code , <l.lunak@kde.org>
//

// prepend http://www. or http:// if there's no protocol in 's'
// This is used only when there are no completion matches
static QString hp_tryPrepend( const QString& s )
{
    if (s.isEmpty() || s[0] == QLatin1Char('/') || s[0] == QLatin1Char('~'))
        return QString();

    bool containsSpace = false;

    for( int pos = 0;
         pos < s.length() - 2; // 4 = ://x
         ++pos ) {
        if( s[ pos ] == ':' && s[ pos + 1 ] == '/' && s[ pos + 2 ] == '/' )
            return QString();
        if( !s[ pos ].isLetter() )
            break;
        if( s[pos].isSpace() ) {
            containsSpace = true;
            break;
        }
    }

    if (containsSpace || s.at(s.length()-1).isSpace()) {
        return QString();
    }

    return ( s.startsWith( "www." ) ? "http://" : "http://www." ) + s;
}


static void hp_removeDupe( KCompletionMatches& l, const QString& dupe,
                           KCompletionMatches::Iterator it_orig )
{
    KCompletionMatches::Iterator it = it_orig + 1;
    while (it != l.end()) {
        if( (*it).value() == dupe ) {
            (*it_orig).first = std::max( (*it_orig).first, (*it).key());
            it = l.erase( it );
            continue;
        }
        ++it;
    }
}

// remove duplicates like 'http://www.kde.org' and 'http://www.kde.org/'
// (i.e. the trailing slash)
// some duplicates are also created by prepending protocols
static void hp_removeDuplicates( KCompletionMatches& l )
{
    QString http = "http://";
    QString ftp = "ftp://ftp.";
    QString file = "file:";
    QString file2 = "file://";
    l.removeDuplicates();
    for( KCompletionMatches::Iterator it = l.begin();
         it != l.end();
         ++it ) {
        QString str = (*it).value();
        if( str.startsWith( http )) {
            if( str.indexOf( '/', 7 ) < 0 ) { // http://something<noslash>
                hp_removeDupe( l, str + '/', it );
                hp_removeDupe( l, str.mid( 7 ) + '/', it );
            } else if( str[ str.length() - 1 ] == '/' ) {
                hp_removeDupe( l, str.left( str.length() - 1 ), it );
                hp_removeDupe( l, str.left( str.length() - 1 ).mid( 7 ), it );
            }
            hp_removeDupe( l, str.mid( 7 ), it );
        }
        else if( str.startsWith( ftp )) // ftp://ftp.
            hp_removeDupe( l, str.mid( 6 ), it ); // remove dupes without ftp://
        else if( str.startsWith( file2 ))
            hp_removeDupe( l, str.mid( 7 ), it ); // remove dupes without file://
        else if( str.startsWith( file ))
            hp_removeDupe( l, str.mid( 5 ), it ); // remove dupes without file:
    }
}

static void hp_removeCommonPrefix( KCompletionMatches& l, const QString& prefix )
{
    for( KCompletionMatches::Iterator it = l.begin();
         it != l.end();
         ) {
        if( (*it).value().startsWith( prefix )) {
            it = l.erase( it );
            continue;
        }
        ++it;
    }
}

// don't include common prefixes like 'http://', i.e. when s == 'h', include
// http://hotmail.com but don't include everything just starting with 'http://'
static void hp_checkCommonPrefixes( KCompletionMatches& matches, const QString& s )
{
    static const char* const prefixes[] = {
        "http://",
        "https://",
        "www.",
        "ftp://",
        "http://www.",
        "https://www.",
        "ftp://ftp.",
        "file:",
        "file://",
        NULL };
    for( const char* const *pos = prefixes;
         *pos != NULL;
         ++pos ) {
        QString prefix = *pos;
        if( prefix.startsWith( s )) {
            hp_removeCommonPrefix( matches, prefix );
        }
    }
}

QStringList KonqMainWindow::historyPopupCompletionItems( const QString& s)
{
    const QString http = "http://";
    const QString https = "https://";
    const QString www = "http://www.";
    const QString wwws = "https://www.";
    const QString ftp = "ftp://";
    const QString ftpftp = "ftp://ftp.";
    const QString file = "file:"; // without /, because people enter /usr etc.
    const QString file2 = "file://";
    if( s.isEmpty())
	    return QStringList();
    KCompletionMatches matches= s_pCompletion->allWeightedMatches( s );
    hp_checkCommonPrefixes( matches, s );
    bool checkDuplicates = false;
    if ( !s.startsWith( ftp ) ) {
        matches += s_pCompletion->allWeightedMatches( ftp + s );
	if( QString( "ftp." ).startsWith( s ))
            hp_removeCommonPrefix( matches, ftpftp );
        checkDuplicates = true;
    }
    if ( !s.startsWith( https ) ) {
        matches += s_pCompletion->allWeightedMatches( https + s );
	if( QString( "www." ).startsWith( s ))
            hp_removeCommonPrefix( matches, wwws );
        checkDuplicates = true;
    }
    if ( !s.startsWith( http )) {
        matches += s_pCompletion->allWeightedMatches( http + s );
	if( QString( "www." ).startsWith( s ))
            hp_removeCommonPrefix( matches, www );
        checkDuplicates = true;
    }
    if ( !s.startsWith( www ) ) {
        matches += s_pCompletion->allWeightedMatches( www + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( wwws ) ) {
        matches += s_pCompletion->allWeightedMatches( wwws + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( ftpftp ) ) {
        matches += s_pCompletion->allWeightedMatches( ftpftp + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( file ) ) {
        matches += s_pCompletion->allWeightedMatches( file + s );
        checkDuplicates = true;
    }
    if ( !s.startsWith( file2 ) ) {
        matches += s_pCompletion->allWeightedMatches( file2 + s );
        checkDuplicates = true;
    }
    if( checkDuplicates )
        hp_removeDuplicates( matches );
    QStringList items = matches.list();
    if( items.count() == 0
	&& !s.contains( ':' ) && !s.isEmpty() && s[ 0 ] != '/' )
        {
        QString pre = hp_tryPrepend( s );
        if( !pre.isNull())
            items += pre;
        }
    return items;
}

#ifndef NDEBUG
void KonqMainWindow::dumpViewList()
{
    kDebug() << m_mapViews.count() << "views:";

    MapViews::Iterator end = m_mapViews.end();
    for (MapViews::Iterator it = m_mapViews.begin(); it != end; ++it) {
        KonqView* view = it.value();
        kDebug() << view << view->part();
    }
}
#endif

// KonqFrameContainerBase implementation BEGIN

void KonqMainWindow::insertChildFrame( KonqFrameBase * frame, int /*index*/ )
{
  m_pChildFrame = frame;
  m_pActiveChild = frame;
  frame->setParentContainer(this);
  if ( centralWidget() && centralWidget() != frame->asQWidget() ) {
      centralWidget()->setParent( 0 ); // workaround Qt-4.1.2 crash (reported)
      setCentralWidget( 0 );
  }
  setCentralWidget( frame->asQWidget() );
}

void KonqMainWindow::childFrameRemoved(KonqFrameBase* frame)
{
    Q_ASSERT(frame == m_pChildFrame);
    Q_UNUSED(frame)
    m_pChildFrame = 0;
    m_pActiveChild = 0;
}

void KonqMainWindow::saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase* docContainer, int id, int depth ) { if( m_pChildFrame ) m_pChildFrame->saveConfig( config, prefix, options, docContainer, id, depth); }

void KonqMainWindow::copyHistory( KonqFrameBase *other ) { if( m_pChildFrame ) m_pChildFrame->copyHistory( other ); }

void KonqMainWindow::setTitle( const QString &/*title*/ , QWidget* /*sender*/) { return; }
void KonqMainWindow::setTabIcon( const KUrl &/*url*/, QWidget* /*sender*/ ) { return; }

QWidget* KonqMainWindow::asQWidget() { return this; }

KonqFrameBase::FrameType KonqMainWindow::frameType() const { return KonqFrameBase::MainWindow; }

KonqFrameBase* KonqMainWindow::childFrame()const { return m_pChildFrame; }

void KonqMainWindow::setActiveChild( KonqFrameBase* /*activeChild*/ ) { return; }

void KonqMainWindow::setWorkingTab(int index)
{
    m_workingTab = index;
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf( const QString &mimeType )
{
    return isMimeTypeAssociatedWithSelf( mimeType, KMimeTypeTrader::self()->preferredService( mimeType, "Application" ) );
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf( const QString &/*mimeType*/, const KService::Ptr &offer )
{
    // Prevention against user stupidity : if the associated app for this mimetype
    // is konqueror/kfmclient, then we'll loop forever. So we have to
    // 1) force embedding first, if that works we're ok
    // 2) check what KRun is going to do before calling it.
    return ( offer && ( offer->desktopEntryName() == "konqueror" ||
             offer->exec().trimmed().startsWith( "kfmclient" ) ) );
}

bool KonqMainWindow::refuseExecutingKonqueror(const QString& mimeType)
{
    if ( activeViewsNotLockedCount() > 0 ) { // if I lock the only view, then there's no error: open links in a new window
        KMessageBox::error( this, i18n("There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type.", mimeType));
        return true; // we refuse indeed
    }
    return false; // no error
}

// KonqFrameContainerBase implementation END

void KonqMainWindow::setPreloadedFlag( bool preloaded )
{
    if( s_preloaded == preloaded )
        return;
    s_preloaded = preloaded;
    if( s_preloaded )
    {
        kapp->disableSessionManagement(); // don't restore preloaded konqy's
        KonqSessionManager::self()->disableAutosave(); // don't save sessions
        return; // was registered before calling this
    }
    delete s_preloadedWindow; // preloaded state was abandoned without reusing the window
    s_preloadedWindow = NULL;
    kapp->enableSessionManagement(); // enable SM again
    KonqSessionManager::self()->enableAutosave(); // enable session saving again
    QDBusInterface ref( "org.kde.kded", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus() );
    ref.call( "unregisterPreloadedKonqy", QDBusConnection::sessionBus().baseService() );
}

void KonqMainWindow::setPreloadedWindow( KonqMainWindow* window )
{
    s_preloadedWindow = window;
    if( window == NULL )
        return;
    window->viewManager()->clear();
    KIO::Scheduler::unregisterWindow( window );
}

// used by preloading - this KonqMainWindow will be reused, reset everything
// that won't be reset by loading a profile
void KonqMainWindow::resetWindow()
{
#ifdef Q_WS_X11
    char data[ 1 ];
    // empty append to get current X timestamp
    QWidget tmp_widget;
    XChangeProperty( QX11Info::display(), tmp_widget.winId(), XA_WM_CLASS, XA_STRING, 8,
		    PropModeAppend, reinterpret_cast<unsigned char*>(&data), 0 );
    XEvent ev;
    XWindowEvent( QX11Info::display(), tmp_widget.winId(), PropertyChangeMask, &ev );
    long x_time = ev.xproperty.time;
    // bad hack - without updating the _KDE_NET_WM_USER_CREATION_TIME property,
    // KWin will apply don't_steal_focus to this window, and will not make it active
    // (shows mainly with 'konqueror --preload')
    static Atom atom = XInternAtom( QX11Info::display(), "_KDE_NET_WM_USER_CREATION_TIME", False );
    XChangeProperty( QX11Info::display(), winId(), atom, XA_CARDINAL, 32,
		     PropModeReplace, reinterpret_cast<unsigned char *>(&x_time), 1);
    // reset also user time, so that this window won't have _NET_WM_USER_TIME set
    QX11Info::setAppUserTime(CurrentTime);

    static Atom atom3 = XInternAtom( QX11Info::display(), "_NET_WM_USER_TIME", False );
    XDeleteProperty( QX11Info::display(), winId(), atom3 );
#endif
// Qt remembers the iconic state if the window was withdrawn while on another virtual desktop
    setWindowState( windowState() & ~Qt::WindowMinimized );
    ignoreInitialGeometry();
}

bool KonqMainWindow::event( QEvent* e )
{
    if( e->type() == QEvent::DeferredDelete )
    {
        // since the preloading code tries to reuse KonqMainWindow,
        // the last window shouldn't be really deleted, but only hidden
        // deleting WDestructiveClose windows is done using deleteLater(),
        // so catch QEvent::DeferredDelete and check if this window should stay
        if( stayPreloaded())
        {
            setAttribute(Qt::WA_DeleteOnClose); // was reset before deleteLater()
            return true; // no deleting
        }

    } else if ( e->type() == QEvent::StatusTip) {
        if (m_currentView && m_currentView->frame()->statusbar()) {
            KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();
            statusBar->message(static_cast<QStatusTipEvent*>(e)->tip());
        }
    }

    if ( KonqFileSelectionEvent::test( e ) ||
         KonqFileMouseOverEvent::test( e ) )
    {
        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it )
            QApplication::sendEvent( (*it)->part(), e );
        return true;
    }
    if ( KParts::OpenUrlEvent::test( e ) )
    {
        KParts::OpenUrlEvent * ev = static_cast<KParts::OpenUrlEvent*>(e);

        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it )
        {
            // Don't resend to sender
            if (it.key() != ev->part())
            {
                //kDebug() << "Sending event to view" << it.key()->metaObject()->className();
                QApplication::sendEvent( it.key(), e );
            }
        }
    }
    return KParts::MainWindow::event( e );
}

bool KonqMainWindow::stayPreloaded()
{
#ifdef Q_WS_X11
    // last window?
    if( mainWindowList()->count() > 1 )
        return false;
    // not running in full KDE environment?
    if( getenv( "KDE_FULL_SESSION" ) == NULL || getenv( "KDE_FULL_SESSION" )[ 0 ] == '\0' )
        return false;
    // not the same user like the one running the session (most likely we're run via sudo or something)
    if( getenv( "KDE_SESSION_UID" ) != NULL && uid_t( atoi( getenv( "KDE_SESSION_UID" ))) != getuid())
        return false;
    if( KonqSettings::maxPreloadCount() == 0 )
        return false;
    viewManager()->clear(); // reduce resource usage before checking it
    if( !checkPreloadResourceUsage())
        return false;
    QDBusInterface ref( "org.kde.kded", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus() );
    QX11Info info;
    QDBusReply<bool> retVal = ref.call( QDBus::Block, "registerPreloadedKonqy", QDBusConnection::sessionBus().baseService(), info.screen());
    if ( !retVal )
    {
        return false;
    }
    KonqMainWindow::setPreloadedFlag( true );
    kDebug() << "Konqy kept for preloading:" << QDBusConnection::sessionBus().baseService();
    KonqMainWindow::setPreloadedWindow( this );
    return true;
#else
    return false;
#endif
}

// try to avoid staying running when leaking too much memory
// this is checked by using mallinfo() and comparing
// memory usage during konqy startup and now, if the difference
// is too large -> leaks -> quit
// also, if this process is running for too long, or has been
// already reused too many times -> quit, just in case
bool KonqMainWindow::checkPreloadResourceUsage()
{
    if(
#ifndef NDEBUG
        isatty( STDIN_FILENO ) ||
#endif
        isatty( STDOUT_FILENO ) || isatty( STDERR_FILENO ))
    {
        kDebug() << "Running from tty, not keeping for preloading";
        return false;
    }
    int limit;
    qint64 usage = current_memory_usage( &limit );
    kDebug() << "Memory usage increase: " << ( usage - s_initialMemoryUsage )
        << " (" << usage << " - " << s_initialMemoryUsage << "), increase limit: " << limit;
    const qint64 max_allowed_usage = s_initialMemoryUsage + limit;
    if( usage > max_allowed_usage ) // too much memory used?
    {
	kDebug() << "Not keeping for preloading due to high memory usage";
	return false;
    }
    // working memory usage test ( usage != 0 ) makes others less strict
    if( ++s_preloadUsageCount > ( usage != 0 ? 100 : 10 )) // reused too many times?
    {
	kDebug() << "Not keeping for preloading due to high usage count";
	return false;
    }
    if( time( NULL ) > s_startupTime + 60 * 60 * ( usage != 0 ? 4 : 1 )) // running for too long?
    {
	kDebug() << "Not keeping for preloading due to long usage time";
	return false;
    }
    return true;
}

static qint64 current_memory_usage( int* limit )
{
#ifdef __linux__  //krazy:exclude=cpp
// Check whole memory usage - VmSize
    QFile f( QString::fromLatin1( "/proc/%1/statm" ).arg(getpid()) );
    if( f.open( QIODevice::ReadOnly ))
    {
        QByteArray buffer; buffer.resize( 100 );
        const auto bytes = f.readLine( buffer.data(), buffer.size()-1 );
        if ( bytes != -1 )
        {
            QString line = QString::fromLatin1( buffer ).trimmed();
            const qint64 usage = line.section( ' ', 0, 0 ).toLongLong();
            if( usage > 0 )
            {
                qint64 pagesize = sysconf (_SC_PAGE_SIZE);
                if( pagesize < 0 )
                    pagesize = 4096;
                if( limit != nullptr )
                    *limit = 16 * 1024 * 1024;
                return usage * pagesize;
            }
        }
    }
    kWarning() << "Couldn't read VmSize from /proc/*/statm." ;
#endif
// Check malloc() usage - very imprecise, but better than nothing.
    size_t usage_sum = 0;
#if defined(KDE_MALLINFO_STDLIB) || defined(KDE_MALLINFO_MALLOC)
    struct mallinfo2 m = mallinfo2();
#ifdef KDE_MALLINFO_FIELD_hblkhd
    usage_sum += m.hblkhd;
#endif
#ifdef KDE_MALLINFO_FIELD_uordblks
    usage_sum += m.uordblks;
#endif
#ifdef KDE_MALLINFO_FIELD_usmblks
    usage_sum += m.usmblks;
#endif
    // unlike /proc , this doesn't include things like size of dlopened modules,
    // and also doesn't include malloc overhead
    if( limit != nullptr )
        *limit = 6 * 1024 * 1024;
#endif
    return usage_sum;
}

void KonqMainWindow::slotUndoTextChanged( const QString & newText )
{
  m_paUndo->setText(newText);
}

// outlined because of QPointer and gcc3
KonqView * KonqMainWindow::currentView() const
{
     return m_currentView;
}

bool KonqMainWindow::accept( KonqFrameVisitor* visitor )
{
    return visitor->visit( this )
           && (!m_pChildFrame || m_pChildFrame->accept( visitor ))
           && visitor->endVisit( this );
}

void KonqMainWindow::applyWindowSizeFromProfile(const KConfigGroup& profileGroup)
{
    // KMainWindow::restoreWindowSize is protected so this logic can't move to KonqViewManager
    const QSize size = KonqViewManager::readDefaultSize(profileGroup, this); // example: "Width=80%"
    if (size.isValid())
        resize(size);
    restoreWindowSize(profileGroup); // example: "Width 1400=1120"
}

QLineEdit* KonqMainWindow::comboEdit()
{
  return m_combo ? m_combo->lineEdit() : 0;
}

#include "konqmainwindow.moc"
