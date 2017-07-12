# include "EveWindow.h"

using namespace std;

int initSST(HWND hwndOverlay, EveProbeScannerWindow &windowProbeScanner)
{
    // === Find data for reading memory ===

        // Eve Online PID
    int evePID = 0;

    while(evePID == 0)
    {
        cout << endl;
        cout << "   | Find EVE ONLINE process: ";
        evePID = eveGetProcessId();

        if (evePID)
            cout << "OK" << endl;
        else
        {
            cout << "ERROR" << endl;
            cout << "   | Next attempt after 5 seconds...";
            Sleep(5000);
        }
    }

        // Eve Online HANDLE
    HANDLE eveHandle = (HANDLE)0;

    while(eveHandle == (HANDLE)0)
    {
        cout << "   | Find EVE ONLINE handle: ";

        eveHandle = eveGetHandle(evePID);

        if (eveHandle)
            cout << "OK" << endl;
        else
        {
            cout << "ERROR" << endl;
            cout << "   | ERROR CODE: " << GetLastError() << endl;
            cout << "   | Next attempt after 5 seconds..." << endl;
            Sleep(5000);
        }
    }

        // Eve Online HWND
    HWND eveHWND = (HWND)0;

    while(eveHWND == 0)
    {
        cout << "   | Find EVE ONLINE window: ";
        eveHWND = eveGetMainWindow(evePID);

        if (eveHWND)
            cout << "OK" << endl;
        else
        {
            cout << "ERROR" << endl;
            cout << "   | Next attempt after 5 seconds..." << endl;
            Sleep(5000);
        }
    }

        // Eve Online UIRoot object
    int UIRootAddress = 0;

    while(0 == UIRootAddress)
    {
        cout << "   | Find a graphical interface: ";
        UIRootAddress = eveGetUIRoot(eveHandle);

        if (UIRootAddress)
        {
            cout << "OK" << endl;
            //cout << "   | " << hex << UIRootAddress << endl;
        }
        else
        {
            cout << "ERROR" << endl;
            cout << "   | Next attempt after 5 seconds..." << endl;
            Sleep(5000);
        }
    }

        // initiate windowProbeScanner
    return windowProbeScanner.init(evePID, eveHandle, eveHWND, UIRootAddress);
}
