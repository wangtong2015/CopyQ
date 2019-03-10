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

#ifndef CLIPBOARDBROWSER_H
#define CLIPBOARDBROWSER_H

#include "common/clipboardmode.h"
#include "common/command.h"
#include "gui/clipboardbrowsershared.h"
#include "gui/configtabshortcuts.h"
#include "gui/theme.h"
#include "item/clipboardmodel.h"
#include "item/itemdelegate.h"
#include "item/itemwidget.h"

#include <QListView>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

#include <memory>

class ItemEditorWidget;
class ItemFactory;
class PersistentDisplayItem;
class QProgressBar;
class QPushButton;

/** List view of clipboard items. */
class ClipboardBrowser : public QListView
{
    Q_OBJECT

    public:
        ClipboardBrowser(
                const QString &tabName,
                const ClipboardBrowserSharedPtr &sharedData,
                QWidget *parent = nullptr);

        /** Close all external editors and save items if needed. */
        ~ClipboardBrowser();

        /**
         * Move item with given @a hash to the top of the list.
         *
         * @return true only if item exists
         */
        bool moveToTop(uint itemHash);

        /** Sort selected items. */
        void sortItems(const QModelIndexList &indexes);

        /** Reverse order of selected items. */
        void reverseItems(const QModelIndexList &indexes);

        /** Index of item in given row. */
        QModelIndex index(int i) const { return m.index(i,0); }

        /** Returns concatenation of selected items. */
        const QString selectedText() const;

        /**
         * Set tab name.
         *
         * This is ID used for saving items.
         *
         * If ID is empty saving is disabled.
         */
        bool setTabName(const QString &tabName);
        const QString &tabName() const { return m_tabName; }

        /**
         * Return true only if the internal editor widget is open.
         */
        bool isInternalEditorOpen() const;

        /**
         * Return true only if an external editor is open.
         */
        bool isExternalEditorOpen() const;

        /**
         * Close internal and external editor
         * unless user don't want to discard changed (show message box).
         *
         * @return true only if editors were closed
         */
        bool maybeCloseEditors();

        /**
         * Override to disable default QAbstractItemView search.
         */
        void keyboardSearch(const QString &) override {}

        QVariantMap copyIndex(const QModelIndex &index) const;

        QVariantMap copyIndexes(const QModelIndexList &indexes) const;

        /** Remove items and return smallest row number (new current item if selection was removed). */
        int removeIndexes(const QModelIndexList &indexes, QString *error = nullptr);

        /** Paste items. */
        void paste(const QVariantMap &data, int destinationRow);

        /** Render preview image with items. */
        QPixmap renderItemPreview(const QModelIndexList &indexes, int maxWidth, int maxHeight);

        /** Removes items from end of list without notifying plugins. */
        bool allocateSpaceForNewItems(int newItemCount);

        /** Add new item to the browser. */
        bool add(
                const QString &txt, //!< Text of new item.
                int row = 0 //!< Target row for the new item (negative to append item).
                );

        /**
         * Add new item to the browser.
         * @a item is automatically deleted after it's no longer needed.
         */
        bool add(
                const QVariantMap &data, //!< Data for new item.
                int row = 0 //!< Target row for the new item (negative to append item).
                );

        /**
         * Add item and remove duplicates.
         */
        void addUnique(const QVariantMap &data, ClipboardMode mode);

        /** Number of items in list. */
        int length() const { return m.rowCount(); }

        /** Receive key event. */
        void keyEvent(QKeyEvent *event) { keyPressEvent(event); }
        /** Move item to clipboard. */
        void moveToClipboard(const QModelIndex &ind);
        /** Move items to clipboard. */
        void moveToClipboard(const QModelIndexList &indexes);
        /** Show only items matching the regular expression. */
        void filterItems(const QRegExp &re);
        /** Open editor. */
        bool openEditor(const QByteArray &textData, bool changeClipboard = false);
        /** Open editor for an item. */
        bool openEditor(const QModelIndex &index);
        /** Add items. */
        void addItems(const QStringList &items);

        /** Set current item. */
        void setCurrent(int row, bool keepSelection = false, bool setCurrentOnly = false);

        /**
         * Save items to configuration if needed.
         */
        void saveUnsavedItems();

        /**
         * Clear all items from configuration.
         * @see setID, loadItems, saveItems
         */
        void purgeItems();

        /**
         * Create and edit new item.
         */
        void editNew(
                const QString &text = QString(), //!< Text of new item.
                bool changeClipboard = false //!< Change clipboard if item is modified.
                );

        /** Edit item in given @a row. */
        void editRow(int row);

        void otherItemLoader(bool next);

        void move(int key);

        QWidget *currentItemPreview();

        void findNext();

        void findPrevious();

        void updateItemWidget(const QModelIndex &index);

        /**
         * Load items from configuration.
         * This function does nothing if model is disabled (e.g. loading failed previously).
         * @see setID, saveItems, purgeItems
         */
        bool loadItems();

        /**
         * Return true only if row is filtered and should be hidden.
         */
        bool isFiltered(int row) const;

        QVariantMap itemData(const QModelIndex &index) const;

        bool isLoaded() const;

        /**
         * Save items to configuration.
         * @see setID, loadItems, purgeItems
         */
        bool saveItems();

        /** Move current item to clipboard. */
        void moveToClipboard();

        /** Edit selected unhidden items. */
        void editSelected();

        /** Edit notes for current item. */
        void editNotes();

        /** Open editor with text of all selected items or for single selected item. */
        bool openEditor();

        /** Remove selected unhidden items. */
        void remove();

        /** Show content of current item. */
        void showItemContent();

        void emitItemCount();

        bool eventFilter(QObject *watched, QEvent *event) override;

    signals:
        /** Show list request. */
        void requestShow(const ClipboardBrowser *self);
        /** Request clipboard change. */
        void changeClipboard(const QVariantMap &data);

        /** Emitted on error. */
        void error(const QString &errorString);

        void editingFinished();

        void itemCountChanged(const QString &tabName, int count);

        void showContextMenu(const QPoint &position);

        void itemsChanged(const ClipboardBrowser *self);

        void itemSelectionChanged(const ClipboardBrowser *self);

        void internalEditorStateChanged(const ClipboardBrowser *self);

        void searchRequest();
        void searchHideRequest();

        void closeExternalEditors();

        void itemWidgetCreated(const PersistentDisplayItem &selection);

    protected:
        void keyPressEvent(QKeyEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *) override;
        void resizeEvent(QResizeEvent *event) override;
        void showEvent(QShowEvent *event) override;
        void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
        void focusInEvent(QFocusEvent *event) override;

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragLeaveEvent(QDragLeaveEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;

        void paintEvent(QPaintEvent *e) override;

        void mousePressEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;

        void enterEvent(QEvent *event) override;

        void doItemsLayout() override;

    private slots:
        void itemModified(const QByteArray &bytes, const QString &mime, const QModelIndex &index);

        void onEditorNeedsChangeClipboard(const QByteArray &bytes, const QString &mime);

        void closeExternalEditor(QObject *editor);

    private:
        void onDataChanged(const QModelIndex &a, const QModelIndex &b);

        void onRowsInserted(const QModelIndex &parent, int first, int last);

        void onItemCountChanged();

        void onEditorSave();

        void onEditorCancel();

        void onEditorInvalidate();

        void setClipboardFromEditor();

        /**
         * Save items to configuration after an interval.
         */
        void delayedSaveItems();

        /**
         * Update item and editor sizes.
         */
        void updateSizes();

        void updateCurrent();

        /**
         * Hide row if filtered out, otherwise show.
         * @return true only if hidden
         */
        bool hideFiltered(int row);
        bool hideFiltered(const QModelIndex &index);

        /**
         * Connects signals and starts external editor.
         */
        bool startEditor(QObject *editor, bool changeClipboard = false);

        void setEditorWidget(ItemEditorWidget *editor, bool changeClipboard = false);

        void editItem(const QModelIndex &index, bool editNotes = false, bool changeClipboard = false);

        void updateEditorGeometry();

        void updateCurrentItem();

        /**
         * Get index near given @a point.
         * If space between items is at the @a point, return next item.
         */
        QModelIndex indexNear(int offset) const;

        int getDropRow(QPoint position);

        void connectModelAndDelegate();

        void disconnectModel();

        void updateItemMaximumSize();

        void processDragAndDropEvent(QDropEvent *event);

        /// Removes indexes without notifying or asking plugins.
        int dropIndexes(const QModelIndexList &indexes);

        void focusEditedIndex();

        int findNextVisibleRow(int row);
        int findPreviousVisibleRow(int row);

        void preload(int pixels, bool above, const QModelIndex &start);

        void updateCurrentIndex();

        void moveToTop(const QModelIndex &index);

        void maybeEmitEditingFinished();

        QModelIndex firstUnpinnedIndex() const;

        void dragDropScroll();

        void setCurrentIndex(const QModelIndex &index);

        ItemSaverPtr m_itemSaver;
        QString m_tabName;
        ClipboardModel m;
        ItemDelegate d;
        QTimer m_timerSave;
        QTimer m_timerEmitItemCount;
        QTimer m_timerUpdateSizes;
        QTimer m_timerUpdateCurrent;
        QTimer m_timerDragDropScroll;
        bool m_ignoreMouseMoveWithButtonPressed = false;
        bool m_resizing = false;

        QPointer<ItemEditorWidget> m_editor;
        int m_externalEditorsOpen = 0;

        ClipboardBrowserSharedPtr m_sharedData;

        int m_dragTargetRow;
        QPoint m_dragStartPosition;

        int m_filterRow = -1;
};

#endif // CLIPBOARDBROWSER_H
