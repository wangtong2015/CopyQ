/*
    Copyright (c) 2019, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "clipboardbrowserplaceholder.h"

#include "common/common.h"
#include "common/timer.h"
#include "item/itemstore.h"
#include "gui/clipboardbrowser.h"
#include "gui/iconfactory.h"
#include "gui/icons.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

ClipboardBrowserPlaceholder::ClipboardBrowserPlaceholder(
        const QString &tabName, const ClipboardBrowserSharedPtr &shared, QWidget *parent)
    : QWidget(parent)
    , m_tabName(tabName)
    , m_sharedData(shared)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const int expireTimeoutMs = 60000 * m_sharedData->minutesToExpire;
    initSingleShotTimer( &m_timerExpire, expireTimeoutMs, this, &ClipboardBrowserPlaceholder::expire );
}

ClipboardBrowser *ClipboardBrowserPlaceholder::createBrowser()
{
    if (m_browser)
        return m_browser;

    if (m_loadButton)
        return nullptr;

    std::unique_ptr<ClipboardBrowser> c( new ClipboardBrowser(m_tabName, m_sharedData, this) );
    emit browserCreated(c.get());

    if ( !c->loadItems() ) {
        createLoadButton();
        return nullptr;
    }

    if (m_timerExpire.interval() > 0) {
        connect( c.get(), &ClipboardBrowser::itemSelectionChanged,
                 &m_timerExpire, static_cast<void (QTimer::*)()>(&QTimer::start) );
        connect( c.get(), &ClipboardBrowser::itemsChanged,
                 &m_timerExpire, static_cast<void (QTimer::*)()>(&QTimer::start) );
    }

    m_browser = c.release();
    setActiveWidget(m_browser);

    restartExpiring();

    return m_browser;
}

bool ClipboardBrowserPlaceholder::setTabName(const QString &tabName)
{
    if ( isEditorOpen() ) {
        if ( !m_browser->setTabName(tabName) )
            return false;
        reloadBrowser();
    } else {
        unloadBrowser();
        if ( !moveItems(m_tabName, tabName) ) {
            if ( isVisible() )
                createBrowser();
            return false;
        }
    }

    ::removeItems(m_tabName);
    m_tabName = tabName;

    if ( isVisible() )
        createBrowser();

    return true;
}

void ClipboardBrowserPlaceholder::removeItems()
{
    unloadBrowser();

    ::removeItems(m_tabName);
}

bool ClipboardBrowserPlaceholder::isDataLoaded() const
{
    return m_browser != nullptr;
}

ClipboardBrowser *ClipboardBrowserPlaceholder::createBrowserAgain()
{
    delete m_loadButton;
    m_loadButton = nullptr;
    return createBrowser();
}

void ClipboardBrowserPlaceholder::reloadBrowser()
{
    if ( isEditorOpen() ) {
        connect( m_browser, &ClipboardBrowser::editingFinished,
                 this, &ClipboardBrowserPlaceholder::reloadBrowser, Qt::UniqueConnection );
    } else {
        unloadBrowser();
        if ( isVisible() )
            createBrowser();
    }
}

void ClipboardBrowserPlaceholder::showEvent(QShowEvent *event)
{
    createBrowser();
    QWidget::showEvent(event);
}

void ClipboardBrowserPlaceholder::hideEvent(QHideEvent *event)
{
    restartExpiring();
    QWidget::hideEvent(event);
}

void ClipboardBrowserPlaceholder::expire()
{
    if (canExpire())
        unloadBrowser();
    else
        restartExpiring();
}

void ClipboardBrowserPlaceholder::setActiveWidget(QWidget *widget)
{
    layout()->addWidget(widget);
    setFocusProxy(widget);
    widget->show();
}

void ClipboardBrowserPlaceholder::createLoadButton()
{
    if (m_loadButton)
        return;

    m_loadButton = new QPushButton(this);
    m_loadButton->setFlat(true);

    const QIcon icon( getIcon("", IconRedo) );
    m_loadButton->setIconSize( QSize(64, 64) );
    m_loadButton->setIcon(icon);

    connect( m_loadButton, &QAbstractButton::clicked,
             this, &ClipboardBrowserPlaceholder::createBrowserAgain );

    setActiveWidget(m_loadButton);
}

void ClipboardBrowserPlaceholder::unloadBrowser()
{
    if (!m_browser)
        return;

    m_browser->saveUnsavedItems();
    m_browser->deleteLater();
    m_browser = nullptr;
}

bool ClipboardBrowserPlaceholder::canExpire() const
{
    return m_browser
            && !m_browser->isVisible()
            && !isEditorOpen();
}

void ClipboardBrowserPlaceholder::restartExpiring()
{
    if ( m_timerExpire.interval() > 0 )
        m_timerExpire.start();
}

bool ClipboardBrowserPlaceholder::isEditorOpen() const
{
    return m_browser && (
                m_browser->isInternalEditorOpen()
                || m_browser->isExternalEditorOpen() );
}
