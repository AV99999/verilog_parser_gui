// File: src/gui/MainWindow.cpp

#include "MainWindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout();

    outputConsole_ = new QTextEdit();
    outputConsole_->setReadOnly(true);

    inputConsole_ = new QLineEdit();
    connect(inputConsole_, &QLineEdit::returnPressed, this, &MainWindow::onCommandEntered);

    layout->addWidget(outputConsole_);
    layout->addWidget(inputConsole_);
    central->setLayout(layout);
    setCentralWidget(central);

    interp_ = Tcl_CreateInterp();
    setupTcl();
}

MainWindow::~MainWindow() {
    if (interp_) Tcl_DeleteInterp(interp_);
}

void MainWindow::onCommandEntered() {
    QString command = inputConsole_->text();
    inputConsole_->clear();
    outputConsole_->append("% " + command);

    if (Tcl_Eval(interp_, command.toStdString().c_str()) == TCL_OK) {
    	outputConsole_->append(Tcl_GetStringResult(interp_));
    } else {
    	outputConsole_->append("[TCL ERROR] " + QString::fromUtf8(Tcl_GetStringResult(interp_)));
    }




}

void MainWindow::setupTcl() {
    Tcl_CreateCommand(interp_, "print", tcl_print, this, nullptr);
    Tcl_CreateCommand(interp_, "get_ports", tcl_get_ports, this, nullptr);
    Tcl_CreateCommand(interp_, "get_cells", tcl_get_cells, this, nullptr);
    Tcl_CreateCommand(interp_, "get_nets", tcl_get_nets, this, nullptr);
    Tcl_CreateCommand(interp_, "load_verilog", tcl_load_verilog, this, nullptr);
}

int MainWindow::tcl_print(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    QString msg;
    for (int i = 1; i < argc; ++i) msg += argv[i];
    Tcl_SetResult(interp, strdup(msg.toStdString().c_str()), TCL_DYNAMIC);
    return TCL_OK;
}

int MainWindow::tcl_get_ports(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& p : self->parser_.get_ports()) result += QString::fromStdString(p) + "\n";
    Tcl_SetResult(interp, strdup(result.toStdString().c_str()), TCL_DYNAMIC);
    return TCL_OK;
}

int MainWindow::tcl_get_cells(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& c : self->parser_.get_cells()) result += QString::fromStdString(c) + "\n";
    Tcl_SetResult(interp, strdup(result.toStdString().c_str()), TCL_DYNAMIC);
    return TCL_OK;
}

int MainWindow::tcl_get_nets(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& n : self->parser_.get_nets()) result += QString::fromStdString(n) + "\n";
    Tcl_SetResult(interp, strdup(result.toStdString().c_str()), TCL_DYNAMIC);
    return TCL_OK;
}

int MainWindow::tcl_load_verilog(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 2) {
        Tcl_SetResult(interp, (char*)"Usage: load_verilog <file>", TCL_STATIC);
        return TCL_ERROR;
    }
    bool ok = self->parser_.parseFile(argv[1]);
    Tcl_SetResult(interp, (char*)(ok ? "OK" : "FAILED"), TCL_STATIC);
    return TCL_OK;
}

