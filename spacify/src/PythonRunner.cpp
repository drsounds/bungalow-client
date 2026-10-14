// Python.h must be included before any other headers to avoid conflicts
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "PythonRunner.h"
#include <string>
#include <chrono>
#include <fstream>
#include <filesystem>

PythonRunner::PythonRunner() : m_running(false), m_shouldStop(false) {}

PythonRunner::~PythonRunner() {
    stop();
}

void PythonRunner::start(const std::string& appDir) {
    if (m_running.load()) {
        return;
    }

    m_running    = true;
    m_shouldStop = false;
    m_appDir     = appDir;

    m_thread = std::thread([this, appDir]() {
        std::string pyVer = "python" +
            std::to_string(PY_MAJOR_VERSION) + "." +
            std::to_string(PY_MINOR_VERSION);
        std::string sitePackages = appDir + "/.venv/lib/" + pyVer + "/site-packages";
        std::string shutdownFile = appDir + "/.shutdown_signal";

        std::filesystem::remove(shutdownFile);

        // Py_InitializeEx(0): skip signal handler registration so we don't
        // interfere with wxWidgets' signal/Cocoa setup on the main thread.
        Py_InitializeEx(0);

        std::string setup =
            "import sys, os\n"
            "sys.path.insert(0, '" + appDir + "')\n"
            "sys.path.insert(1, '" + sitePackages + "')\n"
            "os.chdir(os.path.realpath('" + appDir + "'))\n";
        PyRun_SimpleString(setup.c_str());

        // Shutdown watcher: raises KeyboardInterrupt in the Flask (main Python)
        // thread via ctypes — Flask/Werkzeug handles KeyboardInterrupt cleanly,
        // so control returns to C++ without killing the process.
        std::string flaskCode =
            "from index import app\n"
            "import sys, os, time, threading, ctypes\n"
            "\n"
            "shutdown_file    = '" + shutdownFile + "'\n"
            "flask_thread_id  = threading.main_thread().ident\n"
            "\n"
            "def _watch_shutdown():\n"
            "    while True:\n"
            "        if os.path.exists(shutdown_file):\n"
            "            print('[spacify] Shutdown signal received', flush=True)\n"
            "            ctypes.pythonapi.PyThreadState_SetAsyncExc(\n"
            "                ctypes.c_ulong(flask_thread_id),\n"
            "                ctypes.py_object(KeyboardInterrupt)\n"
            "            )\n"
            "            return\n"
            "        time.sleep(0.1)\n"
            "\n"
            "threading.Thread(target=_watch_shutdown, daemon=True).start()\n"
            "\n"
            "try:\n"
            "    app.run(host='127.0.0.1', port=2855, use_reloader=False)\n"
            "except KeyboardInterrupt:\n"
            "    pass\n"
            "except OSError as e:\n"
            "    print(f'[spacify] Flask could not start: {e}', file=sys.stderr)\n";

        PyRun_SimpleString(flaskCode.c_str());

        std::filesystem::remove(shutdownFile);
        Py_FinalizeEx();
        m_running = false;
    });

    // Keep thread joinable — stop() will join or detach with a timeout.
}

void PythonRunner::stop() {
    if (!m_running.load() || !m_thread.joinable()) {
        return;
    }

    m_shouldStop = true;

    // Write signal file so the Python watcher thread raises KeyboardInterrupt.
    const std::string shutdownFile = m_appDir + "/.shutdown_signal";
    try {
        std::ofstream(shutdownFile) << "shutdown\n";
    } catch (...) {}

    // Wait up to 3 s for Flask to exit cleanly.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (m_running.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (m_thread.joinable()) {
        if (!m_running.load()) {
            m_thread.join();
        } else {
            // Flask didn't exit within timeout — detach so we don't block.
            m_thread.detach();
        }
    }

    std::filesystem::remove(shutdownFile);
    m_running = false;
}
