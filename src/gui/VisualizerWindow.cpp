#include "VisualizerWindow.h"
#include "verilog_parser/VerilogParser.h"

#include <QVBoxLayout>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QDebug>

VisualizerWindow::VisualizerWindow(VerilogParser* parser, QWidget* parent)
    : QWidget(parent), parser_(parser) {
    view_ = new QGraphicsView(this);
    scene_ = new QGraphicsScene(this);
    view_->setScene(scene_);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(view_);
    setLayout(layout);
    setWindowTitle("Netlist Visualizer");
    resize(800, 600);
}

void VisualizerWindow::loadGraph(const QMap<QString, QStringList>& pinsByCell,
                                 const QMap<QPair<QString, QString>, QString>& netByPin) {
    scene_->clear();
    pinItems_.clear();
    netLines_.clear();

    int x = 50, y = 50;
    int cellSpacing = 150;
    int pinSpacing = 20;

    for (auto it = pinsByCell.begin(); it != pinsByCell.end(); ++it) {
        QString cellName = it.key();
        const QStringList& pins = it.value();

        auto* cellBox = scene_->addRect(x, y, 100, 30 + pinSpacing * pins.size(),
                                        QPen(Qt::black), QBrush(Qt::lightGray));
        scene_->addText(cellName)->setPos(x + 5, y - 20);

        int pinY = y + 10;
        for (const QString& pin : pins) {
            auto* pinRect = scene_->addRect(x + 10, pinY, 10, 10, QPen(Qt::blue), QBrush(Qt::blue));
            pinItems_[cellName + "/" + pin] = pinRect;
            connectItem(pinRect);
            pinY += pinSpacing;
        }

        x += cellSpacing;
    }

    // Draw fly-lines for nets
    for (auto it = netByPin.begin(); it != netByPin.end(); ++it) {
        QString fromPinKey = it.key().first + "/" + it.key().second;
        QString netName = it.value();

        QGraphicsRectItem* fromItem = pinItems_.value(fromPinKey, nullptr);
        if (!fromItem)
            continue;

        for (auto jt = netByPin.begin(); jt != netByPin.end(); ++jt) {
            if (jt == it || jt.value() != netName)
                continue;

            QString toPinKey = jt.key().first + "/" + jt.key().second;
            QGraphicsRectItem* toItem = pinItems_.value(toPinKey, nullptr);
            if (!toItem)
                continue;

            QPointF p1 = fromItem->sceneBoundingRect().center();
            QPointF p2 = toItem->sceneBoundingRect().center();

            auto* line = scene_->addLine(QLineF(p1, p2), QPen(Qt::red, 2));
            netLines_[netName].append(line);
        }
    }
}

void VisualizerWindow::connectItem(QGraphicsItem* item) {
    item->setFlag(QGraphicsItem::ItemIsSelectable);
}

void VisualizerWindow::highlightNet(const QString& pinName) {
    QString net;
    for (auto it = netLines_.begin(); it != netLines_.end(); ++it) {
        for (auto* line : it.value()) {
            line->setPen(QPen(Qt::gray, 1));  // Reset
        }
    }

    for (auto it = netLines_.begin(); it != netLines_.end(); ++it) {
        if (pinName.contains(it.key())) {
            for (auto* line : it.value()) {
                line->setPen(QPen(Qt::green, 3));
            }
        }
    }
}

void VisualizerWindow::wheelEvent(QWheelEvent* event) {
    const double scaleFactor = 1.2;
    if (event->angleDelta().y() > 0)
        view_->scale(scaleFactor, scaleFactor);
    else
        view_->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
}

