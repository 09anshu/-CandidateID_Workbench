#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

    // Get the directory where Launch.exe resides
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string currentDir(exePath);
    size_t lastSlash = currentDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        currentDir = currentDir.substr(0, lastSlash);
    }

    // Set working directory to the application root
    SetCurrentDirectoryA(currentDir.c_str());

    // Path to the C++ computational backend
    std::string backendExe = currentDir + "\\workbench_backend.exe";

    // Launch backend hidden (CREATE_NO_WINDOW)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdLine = "\"" + backendExe + "\" 8080";
    BOOL success = CreateProcessA(
        NULL,
        const_cast<char*>(cmdLine.c_str()),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        currentDir.c_str(),
        &si,
        &pi
    );

    // Allow the HTTP server 600 ms to bind to port 8080
    Sleep(600);

    // Try to open in Edge App Mode (clean standalone desktop app window without browser URL bar)
    std::string edgeCmd = " --app=http://localhost:8080/";
    HINSTANCE hEdge = ShellExecuteA(NULL, "open", "msedge.exe", edgeCmd.c_str(), NULL, SW_SHOWNORMAL);

    // If msedge.exe is not found, open in default browser
    if ((INT_PTR)hEdge <= 32) {
        ShellExecuteA(NULL, "open", "http://localhost:8080/", NULL, NULL, SW_SHOWNORMAL);
    }

    if (success) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return 0;
}
