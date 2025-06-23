// File: src/gui/VisualizerWindow.cpp

#include "VisualizerWindow.h"
#include "verilog_parser/VerilogParser.h"

#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>

VisualizerWindow::VisualizerWindow(VerilogParser* parser, QWidget* parent)
    : QWidget(parent), parser_(parser) {
    scene_ = new QGraphicsScene(this);
    view_ = new QGraphicsView(scene_, this);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(view_);
    setLayout(layout);
    setWindowTitle("Verilog Visualizer");
    resize(800, 600);

    populateScene();
}

void VisualizerWindow::populateScene() {
    scene_->clear();

    int x = 50, y = 50;
    int boxWidth = 140, boxHeight = 60;
    int spacing = 100;

    QFont font("Arial", 10);
    QPen pen(Qt::black);
    QBrush brush(Qt::lightGray);

    // Draw each cell as a labeled rectangle
    for (const auto& cell : parser_->get_cells()) {
        QGraphicsRectItem* rect = scene_->addRect(x, y, boxWidth, boxHeight, pen, brush);
        QGraphicsTextItem* label = scene_->addText(QString::fromStdString(cell), font);
        label->setPos(x + 10, y + 15);

        y += boxHeight + spacing;
        if (y > 500) {
            y = 50;
            x += boxWidth + spacing;
        }
    }

    // You can expand this with get_nets(), get_ports(), and draw lines to show connections
}

void VisualizerWindow::loadGraph(const QMap<QString, QStringList>& pinsByCell,
                                 const QMap<QPair<QString, QString>, QString>& netByPin) {
    scene_->clear();  // Clear previous

    QMap<QString, QGraphicsRectItem*> cellItems;

    int x = 0;
    int y = 0;

    for (auto it = pinsByCell.begin(); it != pinsByCell.end(); ++it) {
        QString cell = it.key();
        QStringList pins = it.value();

        auto* rect = scene_->addRect(x, y, 80, 40, QPen(Qt::black), QBrush(Qt::lightGray));
        auto* label = scene_->addText(cell);
        label->setPos(x + 5, y + 5);

        cellItems[cell] = rect;

        int pinX = x;
        int pinY = y + 50;
        for (const QString& pin : pins) {
            auto* pinRect = scene_->addRect(pinX, pinY, 30, 20, QPen(Qt::blue), QBrush(Qt::white));
	    pinRect->setData(0, pin);  // Tag it with pin name
	    pinRect->setFlags(QGraphicsItem::ItemIsSelectable);
	    pinItems_[pin] = pinRect;
            scene_->addText(pin)->setPos(pinX + 5, pinY + 2);
            pinX += 35;
        }

        x += 150;
        if (x > 600) {
            x = 0;
            y += 100;
        }
    }

    // Draw lines for nets
    for (auto it = netByPin.begin(); it != netByPin.end(); ++it) {
        QPair<QString, QString> pinKey = it.key();
        QString net = it.value();

        // Just draw simple lines for now
        auto fromCell = cellItems.value(pinKey.first);
        if (fromCell) {
            scene_->addLine(fromCell->rect().center().x(), fromCell->rect().center().y(),
                           fromCell->rect().center().x() + 50, fromCell->rect().center().y() + 50,
                           QPen(Qt::red, 2));
        }
    }


    connect(scene_, &QGraphicsScene::selectionChanged, this, [this]() {
    QList<QGraphicsItem*> selectedItems = scene_->selectedItems();
    if (!selectedItems.isEmpty()) {
        handleItemClicked(selectedItems.first());
    }
});

}
void VisualizerWindow::handleItemClicked(QGraphicsItem* item) {
    QString pin = item->data(0).toString();
    if (!pin.isEmpty()) {
        qDebug() << "Clicked on pin:" << pin;

        // Reset previous highlights
        for (auto& lines : netLines_) {
            for (auto* line : lines) {
                line->setPen(QPen(Qt::red, 2));  // default
            }
        }

        // Highlight net lines connected to this pin
        if (netLines_.contains(pin)) {
            for (auto* line : netLines_[pin]) {
                line->setPen(QPen(Qt::green, 3));  // Highlighted
            }
        }
    }
}

