// File: src/gui/MainWindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFont>
#include <QKeyEvent>
#include <deque>
#include <tcl.h>
#include "verilog_parser/VerilogParser.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onCommandEntered();
    void openFile();
    void saveOutput();
    void increaseFontSize();
    void decreaseFontSize();

private:
    QTextEdit* outputConsole_;
    QLineEdit* inputConsole_;
    VerilogParser parser_;
    Tcl_Interp* interp_;
    int thread_count_ = 1;
    int fontSize_ = 12;
    std::deque<QString> commandHistory_;
    int historyIndex_ = -1;

    void setupTcl();
    void setupMenu();
    void registerTclCommands();
    static int tcl_print(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_ports(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_nets(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_cells(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_load_verilog(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_set_multi_cpu(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
};

#endif // MAINWINDOW_H

