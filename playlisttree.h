#ifndef PLAYLISTTREE_H
#define PLAYLISTTREE_H

#include <QTreeWidget>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QDropEvent>
#include <QDebug>
#include <QPixmap>
#include <QPainter>
#include <QFontMetrics>

// PlaylistTree: QTreeWidget with a manual internal drag & drop.
//
// The Qt way (per QAbstractItemView docs):
//   - setDragDropMode(DragDrop)  -> the view ACCEPTS drop events and routes
//     them to dragEnterEvent()/dragMoveEvent()/dropEvent() (virtual). With
//     NoDragDrop the view rejects EVERY drag before our handlers run —
//     drop cursor + IgnoreAction, no matter what we override.
//   - setDragEnabled(false)      -> the view does NOT start its own drag;
//     we start a manual QDrag in mouseMoveEvent with the source row in a
//     custom mimeData format.
//   - dropEvent() reads the source row from the mimeData and the target
//     from the cursor, then emits reorderRequested() — MainWindow moves its
//     std::vector<Playlist> and rebuilds the tree (never mid-drop).

class PlaylistTree : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PlaylistTree(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {
        // DragDrop mode: view accepts drops and calls our drag handlers.
        // dragEnabled(false): the view won't start its own drag (we do).
        setAcceptDrops(true);
        setDragEnabled(false);
        setDragDropMode(QAbstractItemView::DragDrop);
        viewport()->setAcceptDrops(true);
    }

    static inline const char *kRowMime = "application/x-lararadio-playlist-row";

signals:
    // Internal reorder: move the row `fromRow` to `toRow` (both are indexes
    // into MainWindow's playlist vector, as seen by the tree).
    void reorderRequested(int fromRow, int toRow);
    // External files dropped (or dragged in from a file manager).
    void urlsDropped(const QList<QUrl> &urls, int row);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        m_dragFromRow = indexAt(event->pos()).row();
        m_dragPressPos = event->pos();
        QTreeWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if ((event->buttons() & Qt::LeftButton) && m_dragFromRow >= 0) {
            if ((event->pos() - m_dragPressPos).manhattanLength() >= 8) {
                QMimeData *mime = new QMimeData;
                mime->setData(QString::fromLatin1(kRowMime),
                              QByteArray::number(m_dragFromRow));
                QDrag *drag = new QDrag(this);
                drag->setMimeData(mime);
                // Show the dragged track in the cursor (name + duration).
                QTreeWidgetItem *it = topLevelItem(m_dragFromRow);
                if (it) {
                    QPixmap pm = makeDragPixmap(it);
                    drag->setPixmap(pm);
                    drag->setHotSpot(QPoint(pm.width() / 2, 12));
                }
                Qt::DropAction result = drag->exec(Qt::CopyAction | Qt::MoveAction);
                m_dragFromRow = -1;
                return;
            }
        }
        QTreeWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_dragFromRow = -1;
        QTreeWidget::mouseReleaseEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->mimeData()->hasFormat(QString::fromLatin1(kRowMime))
            || event->mimeData()->hasUrls())
            event->acceptProposedAction();
        else
            event->ignore();
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (event->mimeData()->hasFormat(QString::fromLatin1(kRowMime))
            || event->mimeData()->hasUrls())
            event->acceptProposedAction();
        else
            event->ignore();
    }

    void dropEvent(QDropEvent *event) override
    {
        if (event->mimeData()->hasFormat(QString::fromLatin1(kRowMime))) {
            bool ok = false;
            int fromRow = event->mimeData()->data(QString::fromLatin1(kRowMime)).toInt(&ok);
            // Target: if the cursor is on the lower half of a row, insert
            // AFTER it; upper half (or empty area) inserts BEFORE it.
            int toRow = -1;
            QModelIndex idx = indexAt(event->position().toPoint());
            if (idx.isValid()) {
                toRow = idx.row();
                QRect r = visualItemRect(topLevelItem(toRow));
                if (event->position().y() > r.center().y())
                    toRow++;
            } else {
                toRow = topLevelItemCount();
            }
            if (ok && fromRow >= 0 && fromRow != toRow)
                emit reorderRequested(fromRow, toRow);
            event->acceptProposedAction();
        } else if (event->mimeData()->hasUrls()) {
            int row = indexAt(event->position().toPoint()).row();
            emit urlsDropped(event->mimeData()->urls(), row);
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    }

private:
    int m_dragFromRow = -1;
    QPoint m_dragPressPos;

    // Renders a small "now dragging: <name> (<duration>)" badge for the QDrag
    // cursor pixmap, styled like a tooltip (dark rounded rect, white text).
    static QPixmap makeDragPixmap(QTreeWidgetItem *item)
    {
        const QString name = item->text(1);
        const QString dur = item->text(2);
        QFont f;
        f.setPointSize(9);
        QFontMetrics fm(f);
        int w = fm.horizontalAdvance(name) + 24;
        int h = 24;
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(f);
        p.setBrush(QColor(30, 30, 30, 220));
        p.setPen(QPen(QColor(120, 180, 255), 1));
        p.drawRoundedRect(1, 1, w - 2, h - 2, 5, 5);
        p.setPen(Qt::white);
        QString text = name;
        if (!dur.isEmpty() && dur != "--:--")
            text += "  [" + dur + "]";
        p.drawText(QRect(12, 3, w - 18, h - 6), Qt::AlignVCenter | Qt::AlignLeft,
                   fm.elidedText(text, Qt::ElideRight, w - 18));
        p.end();
        return pm;
    }
};

#endif // PLAYLISTTREE_H
