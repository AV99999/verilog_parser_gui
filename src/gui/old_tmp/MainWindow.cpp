
// File: src/gui/MainWindow.cpp

#include "MainWindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout();

    outputConsole_ = new QTextEdit();
    outputConsole_->setReadOnly(true);
    outputConsole_->setFontPointSize(fontSize_);

    inputConsole_ = new QLineEdit();
    QFont f = inputConsole_->font();
    f.setPointSize(fontSize_);
    inputConsole_->setFont(f);
    connect(inputConsole_, &QLineEdit::returnPressed, this, &MainWindow::onCommandEntered);

    layout->addWidget(outputConsole_);
    layout->addWidget(inputConsole_);
    central->setLayout(layout);
    setCentralWidget(central);

    setupMenu();

    interp_ = Tcl_CreateInterp();
    setupTcl();
}

MainWindow::~MainWindow() {
    if (interp_) Tcl_DeleteInterp(interp_);
}

void MainWindow::setupMenu() {
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* openAct = new QAction("&Open", this);
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

    QAction* saveAct = new QAction("&Save", this);
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveOutput);

    QAction* exitAct = new QAction("&Exit", this);
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    QAction* fontIncAct = new QAction("Increase Font", this);
    fontIncAct->setShortcut(QKeySequence("Ctrl++"));
    connect(fontIncAct, &QAction::triggered, this, &MainWindow::increaseFontSize);

    QAction* fontDecAct = new QAction("Decrease Font", this);
    fontDecAct->setShortcut(QKeySequence("Ctrl+-"));
    connect(fontDecAct, &QAction::triggered, this, &MainWindow::decreaseFontSize);

    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    toolsMenu->addAction(fontIncAct);
    toolsMenu->addAction(fontDecAct);
}

void MainWindow::increaseFontSize() {
    fontSize_ += 1;
    outputConsole_->setFontPointSize(fontSize_);
    QFont f = inputConsole_->font();
    f.setPointSize(fontSize_);
    inputConsole_->setFont(f);
}

void MainWindow::decreaseFontSize() {
    fontSize_ = std::max(6, fontSize_ - 1);
    outputConsole_->setFontPointSize(fontSize_);
    QFont f = inputConsole_->font();
    f.setPointSize(fontSize_);
    inputConsole_->setFont(f);
}

void MainWindow::openFile() {
    QString file = QFileDialog::getOpenFileName(this, "Open Verilog File", "", "Verilog (*.v *.sv)");
    if (!file.isEmpty()) {
        QString cmd = "load_verilog \"" + file + "\"";
        inputConsole_->setText(cmd);
        onCommandEntered();
    }
}

void MainWindow::saveOutput() {
    QString file = QFileDialog::getSaveFileName(this, "Save Output Log", "output.txt");
    if (!file.isEmpty()) {
        QFile f(file);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << outputConsole_->toPlainText();
            f.close();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Up && !commandHistory_.empty()) {
        historyIndex_ = std::max(0, historyIndex_ - 1);
        inputConsole_->setText(commandHistory_[historyIndex_]);
    } else if (event->key() == Qt::Key_Down && !commandHistory_.empty()) {
        historyIndex_ = std::min(static_cast<int>(commandHistory_.size()) - 1, historyIndex_ + 1);
        inputConsole_->setText(commandHistory_[historyIndex_]);
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onCommandEntered() {
    QString command = inputConsole_->text();
    inputConsole_->clear();
    outputConsole_->append("% " + command);

    commandHistory_.push_back(command);
    historyIndex_ = static_cast<int>(commandHistory_.size());

    if (Tcl_Eval(interp_, command.toStdString().c_str()) == TCL_OK) {
        outputConsole_->append(Tcl_GetStringResult(interp_));
    } else {
        outputConsole_->append("[TCL ERROR] " + QString::fromUtf8(Tcl_GetStringResult(interp_)));
    }
}

void MainWindow::setupTcl() {
    Tcl_Eval(interp_, R"(
        rename puts tcl_puts
        proc puts {args} {
            eval tcl_puts $args
            eval print $args
        }
    )");
    Tcl_CreateCommand(interp_, "print", tcl_print, this, nullptr);
    Tcl_CreateCommand(interp_, "get_ports", tcl_get_ports, this, nullptr);
    Tcl_CreateCommand(interp_, "get_cells", tcl_get_cells, this, nullptr);
    Tcl_CreateCommand(interp_, "get_nets", tcl_get_nets, this, nullptr);
    Tcl_CreateCommand(interp_, "load_verilog", tcl_load_verilog, this, nullptr);
    Tcl_CreateCommand(interp_, "set_multi_cpu", tcl_set_multi_cpu, this, nullptr);
}

int MainWindow::tcl_print(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    QString msg;
    for (int i = 1; i < argc; ++i) msg += argv[i];
    Tcl_SetObjResult(interp, Tcl_NewStringObj(msg.toStdString().c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_get_ports(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& p : self->parser_.get_ports()) result += QString::fromStdString(p) + "\n";
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.toStdString().c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_get_cells(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& c : self->parser_.get_cells()) result += QString::fromStdString(c) + "\n";
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.toStdString().c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_get_nets(ClientData clientData, Tcl_Interp* interp, int, const char**) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString result;
    for (const auto& n : self->parser_.get_nets()) result += QString::fromStdString(n) + "\n";
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.toStdString().c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_load_verilog(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: load_verilog <file>", -1));
        return TCL_ERROR;
    }
    bool ok = self->parser_.parseFileMultithreaded(argv[1], self->thread_count_);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(ok ? "OK" : "FAILED", -1));
    return TCL_OK;
}

int MainWindow::tcl_set_multi_cpu(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: set_multi_cpu <int>", -1));
        return TCL_ERROR;
    }
    self->thread_count_ = std::max(1, std::atoi(argv[1]));
    QString msg = QString("Multi-core parsing set to ") + QString::number(self->thread_count_);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(msg.toStdString().c_str(), -1));
    return TCL_OK;
}

