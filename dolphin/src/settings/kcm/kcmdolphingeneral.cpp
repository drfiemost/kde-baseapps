/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kcmdolphingeneral.h"

#include <KTabWidget>
#include <KDialog>
#include <KLocale>
#include <KPluginFactory>
#include <KPluginLoader>

#include <settings/general/behaviorsettingspage.h>
#include <settings/general/previewssettingspage.h>
#include <settings/general/confirmationssettingspage.h>

#include <QDir>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinGeneralConfigFactory, registerPlugin<DolphinGeneralConfigModule>("dolphingeneral");)
K_EXPORT_PLUGIN(KCMDolphinGeneralConfigFactory("kcmdolphingeneral"))

DolphinGeneralConfigModule::DolphinGeneralConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(KCMDolphinGeneralConfigFactory::componentData(), parent),
    m_pages()
{
    Q_UNUSED(args);

    KGlobal::locale()->insertCatalog("dolphin4");

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    KTabWidget* tabWidget = new KTabWidget(this);

    // initialize 'Behavior' tab
    BehaviorSettingsPage* behaviorPage = new BehaviorSettingsPage(QDir::homePath(), tabWidget);
    tabWidget->addTab(behaviorPage, i18nc("@title:tab Behavior settings", "Behavior"));
    connect(behaviorPage, SIGNAL(changed()), this, SLOT(changed()));

    // initialize 'Previews' tab
    PreviewsSettingsPage* previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, SIGNAL(changed()), this, SLOT(changed()));

    // initialize 'Confirmations' tab
    ConfirmationsSettingsPage* confirmationsPage = new ConfirmationsSettingsPage(tabWidget);
    tabWidget->addTab(confirmationsPage,  i18nc("@title:tab Confirmations settings", "Confirmations"));
    connect(confirmationsPage, SIGNAL(changed()), this, SLOT(changed()));

    m_pages.append(behaviorPage);
    m_pages.append(previewsPage);
    m_pages.append(confirmationsPage);

    topLayout->addWidget(tabWidget, 0, 0);
}

DolphinGeneralConfigModule::~DolphinGeneralConfigModule()
{
}

void DolphinGeneralConfigModule::save()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }
}

void DolphinGeneralConfigModule::defaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }
}

#include "kcmdolphingeneral.moc"
