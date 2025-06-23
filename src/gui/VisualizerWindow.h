#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QMap>
#include <QList>
#include <QString>
#include <QPair>

class VerilogParser;

class VisualizerWindow : public QWidget {
    Q_OBJECT
public:
    explicit VisualizerWindow(VerilogParser* parser, QWidget* parent = nullptr);
    void loadGraph(const QMap<QString, QStringList>& pinsByCell,
                   const QMap<QPair<QString, QString>, QString>& netByPin);

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void connectItem(QGraphicsItem* item);
    void highlightNet(const QString& pinName);

    VerilogParser* parser_;
    QGraphicsView* view_;
    QGraphicsScene* scene_;
    QMap<QString, QGraphicsRectItem*> pinItems_;
    QMap<QString, QList<QGraphicsLineItem*>> netLines_;
};

