// File: src/gui/MainWindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMap>
#include <QDir>
#include "CommandLineEdit.h"
#include "verilog_parser/VerilogParser.h"
#include "VisualizerWindow.h"
#include <tcl.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onCommandEntered();
    void showAutocomplete();
    void openFile();
    void saveOutput();
    void increaseFontSize();
    void decreaseFontSize();

private:
    void setupMenu();
    void setupTcl();

    QTextEdit* outputConsole_ = nullptr;
    QLineEdit* inputConsole_ = nullptr;
    int fontSize_ = 12;
    QStringList commandHistory_;
    int historyIndex_ = 0;
    QString pendingCommand_;
    Tcl_Interp* interp_ = nullptr;
    VerilogParser parser_;
    int thread_count_ = 4;
   // VisualizerWindow* visualizer_ = nullptr;
    VisualizerWindow* visualizerWindow_ = nullptr;

    // TCL command callbacks
    static int tcl_print(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_ports(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_cells(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_nets(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_load_verilog(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_set_multi_cpu(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);

    // New TCL commands
    static int tcl_get_pins(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
    static int tcl_get_net_for_pin(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]);
};

#endif  // MAINWINDOW_H

