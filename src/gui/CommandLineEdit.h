// File: src/gui/CommandLineEdit.h
#pragma once
#include <QLineEdit>
#include <QKeyEvent>

class CommandLineEdit : public QLineEdit {
    Q_OBJECT
public:
    using QLineEdit::QLineEdit;

signals:
    void escapePressed();
    void tabPressed();
    void shiftTabPressed();
    void ctrlPressed();  // NEW SIGNAL

protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Escape) {
            emit escapePressed();
        } else if (e->key() == Qt::Key_Tab && !(e->modifiers() & Qt::ShiftModifier)) {
            emit tabPressed();
            e->accept();
        } else if (e->key() == Qt::Key_Tab && (e->modifiers() & Qt::ShiftModifier)) {
            emit shiftTabPressed();
            e->accept();
        } else if (e->key() == Qt::Key_Control) {
            emit ctrlPressed();  // NEW BEHAVIOR
            e->accept();
        } else {
            QLineEdit::keyPressEvent(e);
        }
    }
};

