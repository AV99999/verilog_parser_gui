// File: src/gui/MainWindow.cpp

#include "MainWindow.h"
#include "CommandLineEdit.h"
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QListWidget>
#include <QMenuBar>
#include <QAction>
#include <QDir>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout();

    outputConsole_ = new QTextEdit();
    outputConsole_->setReadOnly(true);
    outputConsole_->setFontPointSize(fontSize_);

    inputConsole_ = new CommandLineEdit();
    QFont f = inputConsole_->font();
    f.setPointSize(fontSize_);
    inputConsole_->setFont(f);
    connect(inputConsole_, &QLineEdit::returnPressed, this, &MainWindow::onCommandEntered);
    auto* cmdInput = static_cast<CommandLineEdit*>(inputConsole_);
    connect(cmdInput, &CommandLineEdit::escapePressed, this, &MainWindow::showAutocomplete);
    connect(cmdInput, &CommandLineEdit::tabPressed, this, &MainWindow::showAutocomplete);
    connect(cmdInput, &CommandLineEdit::ctrlPressed, this, &MainWindow::showAutocomplete);

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
    toolsMenu->addSeparator();

    // Action 1: Open blank visualizer window
    QAction* openVisAct = new QAction("Open Visualizer", this);
    connect(openVisAct, &QAction::triggered, this, [this]() {
        if (!visualizerWindow_)
            visualizerWindow_ = new VisualizerWindow(&parser_, this);
        visualizerWindow_->show();
    });
    toolsMenu->addAction(openVisAct);

    // Action 2: Show Netlist Graph
QAction* showGraphAct = new QAction("Show Netlist Graph", this);
connect(showGraphAct, &QAction::triggered, this, [this]() {
    if (!visualizerWindow_)
        visualizerWindow_ = new VisualizerWindow(&parser_, this);

    QMap<QString, QStringList> pinsByCell = parser_.getPinsByCell();
    QMap<QPair<QString, QString>, QString> netByPin = parser_.getNetByPin();

    visualizerWindow_->loadGraph(pinsByCell, netByPin);
    visualizerWindow_->show();
});
toolsMenu->addAction(showGraphAct);
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
    outputConsole_->append("> " + command);

    pendingCommand_ += command + "\n";

    if (Tcl_CommandComplete(pendingCommand_.toStdString().c_str())) {
        commandHistory_.push_back(pendingCommand_.trimmed());
        historyIndex_ = static_cast<int>(commandHistory_.size());

        if (Tcl_Eval(interp_, pendingCommand_.toStdString().c_str()) == TCL_OK) {
            outputConsole_->append(Tcl_GetStringResult(interp_));
        } else {
            outputConsole_->append("[TCL ERROR] " + QString::fromUtf8(Tcl_GetStringResult(interp_)));
        }
        pendingCommand_.clear();
    } else {
        outputConsole_->append("... ");
    }
}

void MainWindow::showAutocomplete() {
    QString currentText = inputConsole_->text().trimmed();
    qDebug() << "Autocomplete triggered with:" << currentText;

    static const QMap<QString, QString> commandMap = {
        {"load_verilog", "<filename>"},
        {"set_multi_cpu", "<int>"},
        {"get_ports", ""},
        {"get_cells", ""},
        {"get_nets", ""},
        {"get_pins", "<cell>"},
        {"get_net_for_pin", "<cell> <pin>"},
        {"print", "<message>"}
    };

    QStringList matches;
    for (auto it = commandMap.begin(); it != commandMap.end(); ++it) {
        if (it.key().startsWith(currentText)) {
            QString full = it.key();
            if (!it.value().isEmpty())
                full += " " + it.value();
            matches << full;
        }
    }

    if (matches.isEmpty()) {
        outputConsole_->append("[Auto] No match found");
        return;
    }

    if (matches.size() == 1) {
        inputConsole_->setText(matches[0]);
        outputConsole_->append("[Auto] " + matches[0]);
        return;
    }

    QString common = matches[0];
    for (const QString& match : matches) {
        int i = 0;
        while (i < common.size() && i < match.size() && common[i] == match[i])
            ++i;
        common = common.left(i);
    }

    if (!common.isEmpty() && common != currentText) {
        inputConsole_->setText(common);
        outputConsole_->append("[Auto] " + common);
    } else {
        outputConsole_->append("[Auto] Multiple matches: " + matches.join(", "));
    }

    QString lastToken = currentText.section(" ", -1);
    QDir dir;
    QFileInfo fi(lastToken);
    QString baseDir = fi.path().isEmpty() ? QDir::currentPath() : fi.path();
    QString prefix = fi.fileName();
    dir.setPath(baseDir);
    QStringList fileMatches;
    for (const QFileInfo &entry : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (entry.fileName().startsWith(prefix)) {
            fileMatches << entry.filePath();
        }
    }
    if (!fileMatches.isEmpty()) {
        QString common = fileMatches[0];
        for (const QString& match : fileMatches) {
            int i = 0;
            while (i < common.size() && i < match.size() && common[i] == match[i])
                ++i;
            common = common.left(i);
        }
        if (!common.isEmpty() && common != lastToken) {
            QString completed = currentText.left(currentText.lastIndexOf(lastToken)) + common;
            inputConsole_->setText(completed);
            outputConsole_->append("[Auto] " + completed);
        } else {
            outputConsole_->append("[Auto] Path matches: " + fileMatches.join(", "));
        }
    }
}

void MainWindow::setupTcl() {
    Tcl_CreateCommand(interp_, "print", tcl_print, this, nullptr);
    Tcl_CreateCommand(interp_, "get_ports", tcl_get_ports, this, nullptr);
    Tcl_CreateCommand(interp_, "get_cells", tcl_get_cells, this, nullptr);
    Tcl_CreateCommand(interp_, "get_nets", tcl_get_nets, this, nullptr);
    Tcl_CreateCommand(interp_, "get_pins", tcl_get_pins, this, nullptr);
    Tcl_CreateCommand(interp_, "get_net_for_pin", tcl_get_net_for_pin, this, nullptr);
    Tcl_CreateCommand(interp_, "load_verilog", tcl_load_verilog, this, nullptr);
    Tcl_CreateCommand(interp_, "set_multi_cpu", tcl_set_multi_cpu, this, nullptr);

    Tcl_Eval(interp_, R"(
        rename puts tcl_puts
        proc puts {args} {
            eval print $args
        }

        rename source tcl_source
        proc source {args} {
            set echo 0
            set verbose 0
            set filename ""
            foreach arg $args {
                if {$arg eq "-e"} {
                    set echo 1
                } elseif {$arg eq "-v"} {
                    set verbose 1
                } else {
                    set filename $arg
                }
            }
            set fp [open $filename r]
            set lines [split [read $fp] "\n"]
            close $fp
            foreach line $lines {
                if {$echo} { puts "> $line" }
                if {[string trim $line] eq ""} { continue }
                if {[catch {eval $line} result]} {
                    puts "[TCL ERROR] $result"
                } elseif {$verbose} {
                    puts "$result"
                }
            }
        }
    )");
}

int MainWindow::tcl_print(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    auto* self = static_cast<MainWindow*>(clientData);
    QString msg;
    for (int i = 1; i < argc; ++i) {
        msg += argv[i];
        if (i < argc - 1)
            msg += " ";
    }
    self->outputConsole_->append(msg);
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

int MainWindow::tcl_get_pins(ClientData clientData, Tcl_Interp* interp, int argc, const char** argv) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: get_pins <cell>", -1));
        return TCL_ERROR;
    }
    QString result;
    for (const auto& pin : self->parser_.get_pins(argv[1]))
        result += QString::fromStdString(pin) + "\n";
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.toStdString().c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_get_net_for_pin(ClientData clientData, Tcl_Interp* interp, int argc, const char** argv) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: get_net_for_pin <cell> <pin>", -1));
        return TCL_ERROR;
    }
    std::string result = self->parser_.get_net_for_pin(argv[1], argv[2]);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
    return TCL_OK;
}

int MainWindow::tcl_load_verilog(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
    auto* self = static_cast<MainWindow*>(clientData);
    if (argc < 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: load_verilog -file <filename>", -1));
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

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == inputConsole_ && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() & Qt::ShiftModifier) {
            showAutocomplete();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

