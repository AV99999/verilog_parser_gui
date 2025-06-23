// File: src/main.cpp

#include <QApplication>
#include <cstring>
#include <iostream>

#include "gui/MainWindow.h"
#include "lic/LicenseChecker.h"
#include <tcl.h>
#include <QMessageBox>

void runTerminal() {
    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Init(interp);

    std::cout << "AVINNOVUS Tcl Terminal Mode\n";
    std::string line;

    while (true) {
        std::cout << "innovus> ";
        if (!std::getline(std::cin, line)) break;
        if (line == ".exit" || line == "exit") break;

        if (Tcl_Eval(interp, line.c_str()) == TCL_OK) {
            std::cout << Tcl_GetStringResult(interp) << "\n";
        } else {
            std::cerr << "Error: " << Tcl_GetStringResult(interp) << "\n";
        }
    }

    Tcl_DeleteInterp(interp);
}

void runGUI(int argc, char *argv[]) {
    QApplication app(argc, argv);

    if (!LicenseChecker::isLicenseValid("license.txt")) {
        QMessageBox::critical(nullptr, "License Error", "License is expired or invalid.");
        return;
    }

    MainWindow w;
    w.show();
    app.exec();
}

int main(int argc, char *argv[]) {
    if (!LicenseChecker::isLicenseValid("license.txt")) {
        std::cerr << "[LICENSE] Invalid or expired. Exiting.\n";
        return 1;
    }

    if (argc > 1) {
        if (std::strcmp(argv[1], "-gui") == 0) {
            runGUI(argc, argv);
            return 0;
        } else if (std::strcmp(argv[1], "-terminal") == 0) {
            runTerminal();
            return 0;
        } else {
            std::cerr << "Unknown mode: " << argv[1] << "\n";
            std::cerr << "Usage: ./verilog -gui | -terminal\n";
            return 1;
        }
    }

    runGUI(argc, argv);
    return 0;
}

