# File: src/CMakeLists.txt

# Collect shared parser source
set(VERILOG_PARSER_SRC
    verilog_parser/VerilogParser.cpp
    verilog_parser/VerilogParser.h
)

# Build Qt GUI + Terminal in one binary
option(BUILD_GUI "Build Qt GUI with Tcl shell" ON)

if (BUILD_GUI)
    find_package(Qt5 REQUIRED COMPONENTS Widgets)

    # Tcl paths
    set(TCL_INCLUDE_PATH "/usr/include/tcl" CACHE PATH "Path to Tcl headers")
    find_library(TCL_LIBRARY NAMES tcl HINTS /usr/lib /usr/lib/x86_64-linux-gnu)

    if (NOT TCL_LIBRARY)
        message(FATAL_ERROR "Tcl library not found. Please install tcl-dev or set TCL_LIBRARY.")
    endif()

    add_executable(verilog
        main.cpp                 # <== Unified main that handles -gui and -terminal
        gui/MainWindow.cpp
        gui/MainWindow.h
	gui/VisualizerWindow.h
	gui/VisualizerWindow.cpp
        gui/CommandLineEdit.h
	lic/LicenseChecker.h
	lic/LicenseChecker.cpp
        ${VERILOG_PARSER_SRC}
    )

    target_include_directories(verilog PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/verilog_parser
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
        ${TCL_INCLUDE_PATH}
    )

    target_link_libraries(verilog
        Qt5::Widgets
        ${TCL_LIBRARY}
    )
else()
    # Optional: CLI-only fallback if GUI not built
    add_executable(verilog
        main.cpp
        ${VERILOG_PARSER_SRC}
    )

    target_include_directories(verilog PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/verilog_parser
    )

    target_link_libraries(verilog ${TCL_LIBRARY})
endif()
