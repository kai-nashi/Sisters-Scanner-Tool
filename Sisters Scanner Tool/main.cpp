#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

# include <Windows.h>
# include <gdiplus.h>

# include <iostream>
# include <string>
# include <thread>
# include <tchar.h>
# include <map>

# include "InitSST.h"

using namespace std;
using namespace Gdiplus;

#define OVERLAY_REDRAW_RATE 5

#define WM_DRAWTEXT 10000000
#define WM_DRAWTYPE 10000001
#define WM_MOVE_TO  10000002
#define WM_SET_SIZE 10000003

vector<EveScanResultStruct> vectorOfScanResultsStructs;
#define RESULTS_TO_DRAW vectorOfScanResultsStructs

RECT overlayRect = { 0, 0, 0, 0 }, overlayResultsRect = { 0, 0, 0, 0 };

# define OVERLAY_RECT overlayRect
# define OVERLAY_RESULTS_RECT overlayResultsRect

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("Sisters Scanner Tools");

void winPaintRect(HWND hwnd, HDC hdc, RECT rect, int color)
// draw filled rect
{
    HBRUSH myBrush = CreateSolidBrush(color);
    FillRect(hdc, &rect, myBrush);
    DeleteObject(myBrush);
}

void winPaintText(HWND hwnd, HDC hdc, string text, int x, int y, int width, int height, int textColor, int bgColor)
// draw text
{
        // text region
    RECT textRegion = {x,y,x+width,y+height};

        // background of text
    winPaintRect(hwnd, hdc, textRegion, bgColor);

        // draw text
    SetBkColor(hdc, bgColor);
    SetTextColor(hdc, textColor);

    DrawText(hdc, text.c_str(), text.size(), &textRegion, DT_LEFT);
}

void winPaintOverlay(HWND hwnd, HDC hdc, int BG_COLOR)
// draw overlay
{
        // draw overlay text
    for (int i=0; i<RESULTS_TO_DRAW.size(); i++)
    {

        EveScanResultStruct resultStruct = RESULTS_TO_DRAW.at(i);

            // if name is not empty. draw it
        if (resultStruct.name.size())
        {
            winPaintText(hwnd, hdc, resultStruct.name,
                        resultStruct.nameX, resultStruct.nameY,
                        resultStruct.nameWidth, resultStruct.nameHeight,
                        resultStruct.textColor, resultStruct.bgColor);
        }

            // if group is not empty, draw it
        if (resultStruct.group.size())
        {
            winPaintText(hwnd, hdc, resultStruct.group,
                        resultStruct.groupX, resultStruct.groupY,
                        resultStruct.groupWidth, resultStruct.groupHeight,
                        resultStruct.textColor, resultStruct.bgColor);
        }
    }

        // clear top and bottom region
    RECT headerRect = {0}, bottomRect = {0};

    headerRect.left = 0;
    headerRect.top = 0;
    headerRect.right = OVERLAY_RECT.right;
    headerRect.bottom = OVERLAY_RESULTS_RECT.top;

    bottomRect.left = 0;
    bottomRect.top = OVERLAY_RESULTS_RECT.bottom;
    bottomRect.right = OVERLAY_RECT.right;
    bottomRect.bottom = OVERLAY_RESULTS_RECT.bottom + (OVERLAY_RECT.bottom - OVERLAY_RESULTS_RECT.bottom);

    winPaintRect(hwnd, hdc, headerRect, BG_COLOR);
    winPaintRect(hwnd, hdc, bottomRect, BG_COLOR);
}

void parserThreadUpdateOverlay(EveProbeScannerWindow *probeScannerParser, int scanResultX, int scanResultY,int scrollY)
// update RESULT_TO_DRAW by adding scrollY
// and cut text by scanResultContainer geometry
{
    vector<EveScanResultStruct> tempScanResultsStructs(probeScannerParser->scanResultOverlay);

        // for each result
    for (int i=0; i<tempScanResultsStructs.size(); i++)
    {
        EveScanResultStruct resultStruct;
        resultStruct = tempScanResultsStructs.at(i);

            // update x
        resultStruct.nameX+= scanResultX;
        resultStruct.groupX+= scanResultX;

            // update y
        resultStruct.nameY += scanResultY - scrollY;
        resultStruct.groupY += scanResultY - scrollY;

        tempScanResultsStructs.at(i) = resultStruct;
    }

    RESULTS_TO_DRAW.clear();
    RESULTS_TO_DRAW.swap(tempScanResultsStructs);
}

int parserThreadDrawOverlay(HWND hwnd, EveProbeScannerWindow *probeScannerParser)
// overlay thread function (created by parser thread)
{
        // rect
    RECT eveWinRect = { 0 }, eveClientRect = { 0 };
    POINT overlayPoint;

    int overlayHeight = 100, overlayWidth = 100;
    int borderSize = 0, headerSize = 0;

    int scanResultX = 0, scanResultY = 0,
        scanResultWidth = 0, scanResultHeight = 0,
        scanResultTotalHeight = 0,
        scrollYAddress = 0, scrollY = 0;

        // get window and client rect for eve online window
    GetWindowRect(probeScannerParser->eveHWND, &eveWinRect);
    GetClientRect(probeScannerParser->eveHWND, &eveClientRect) - borderSize ;

        // calc border size and header size (bottom border = left/right)
    borderSize = ((eveWinRect.right - eveWinRect.left) - (eveClientRect.right - eveClientRect.left))/2;
    headerSize = ((eveWinRect.bottom - eveWinRect.top) - (eveClientRect.bottom - eveClientRect.top)) - borderSize;

        // overlay position
    overlayPoint.x = eveWinRect.left + borderSize + probeScannerParser->getSavedPropetyValue("x");
    overlayPoint.y = eveWinRect.top + headerSize + probeScannerParser->getSavedPropetyValue("y");

        // check overlay position AND inflight state AND probescanner is find AND probeScanner is show
    if (WindowFromPoint(overlayPoint) == probeScannerParser->eveHWND &&
        probeScannerParser->inflight == 1 && probeScannerParser->windowNode.address &&
        probeScannerParser->getSavedPropetyValue("show"))
    {
            // move overlay to current eveonline window position
        WindowProcedure(hwnd, WM_MOVE_TO, overlayPoint.x, overlayPoint.y);

            // update overlay geometry
        overlayWidth = probeScannerParser->getSavedPropetyValue("width");
        overlayHeight = probeScannerParser->getSavedPropetyValue("height");

        OVERLAY_RECT.right = overlayWidth;
        OVERLAY_RECT.bottom = overlayHeight;

        WindowProcedure(hwnd, WM_SET_SIZE, overlayWidth, overlayHeight);

            // scanResult position
        scanResultX = 4;
        scanResultY = 5+probeScannerParser->getSavedPropetyValue("scanResultPosY") + probeScannerParser->getSavedPropetyValue("scanResultsHeaderPosY");

        OVERLAY_RESULTS_RECT.left = 0;
        OVERLAY_RESULTS_RECT.top = scanResultY;

            // scanResult geometry
        scanResultWidth = overlayWidth;
        scanResultHeight = probeScannerParser->getSavedPropetyValue("scanResultHeight") - probeScannerParser->getSavedPropetyValue("scanResultsHeaderHeight");

        OVERLAY_RESULTS_RECT.right = scanResultWidth;
        OVERLAY_RESULTS_RECT.bottom = OVERLAY_RESULTS_RECT.top + scanResultHeight;

            // update scrollY
            // DANGEROUS IF USER CLOSE PROBESCANNER WE TRY READ NONREADEBLE REGION
        scrollYAddress = probeScannerParser->windowNode.readFromPath(probeScannerParser->windowNode.name + "/scannerTools/resultScroll/_position");

        if (scrollYAddress)
            scrollY = readPyInt(probeScannerParser->eveHandle, scrollYAddress);

            // update overlayText with new overlay settings
        parserThreadUpdateOverlay(probeScannerParser, scanResultX, scanResultY, scrollY);

            // repaint overlay
        WindowProcedure(hwnd, WM_PAINT, NULL, NULL);
    }

    // if overlay position is buzy of other window
    // or not fly, or not found probascanner
    else
    {
        WindowProcedure(hwnd, WM_MOVE_TO, -32000, -32000);
    }
}

void parserThreadDelay(int delay)
// delay
{
        // get console handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        // do delay
    for (int i=delay; i>0; i--)
    {
        cout << "\rStarting in [";
        SetConsoleTextAttribute(hConsole, 10);
        cout << i;
        SetConsoleTextAttribute(hConsole, 15);
        cout << "] seconds";

        Sleep(1000);
    }
}

void parserThreadFunction(HWND hwnd, EveProbeScannerWindow probeScannerParser)
{
        // created
    cout << "Parser thread created." << endl;

        // delay 5 seconds before start
    parserThreadDelay(5);

        // update parser data while not paused
    while (0 == probeScannerParser.paused)
    {
            // update and print scanResults
        probeScannerParser.update();
        probeScannerParser.print();

            // redraw overlay rate
        for (int i=0; i<OVERLAY_REDRAW_RATE; i++)
        {
            parserThreadDrawOverlay(hwnd, &probeScannerParser);
            Sleep(1000/OVERLAY_REDRAW_RATE);
        }
    }

        // print closing
    system("cls");
    cout << "Parser thread is stopped." << endl;
    cout << "Press any key to exit" << endl;
    cin.get();
    WindowProcedure(hwnd, WM_MOVE_TO, -1000, -1000);
    WindowProcedure(hwnd, WM_DESTROY, 0, 0);
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
        // console color
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 15);

    cout << "Create class for overlay: ";
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */

    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
    {
        cout << "ERROR" << endl << "Can't register the window class" << endl;
        return 0;
    }
    else
        cout << "OK" << endl;

    cout << "Create overlay window: ";
    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,/* Extended possibilites for variation */
           szClassName,                                                         /* Classname */
           _T("SST:Overlay"),                                                   /* Title Text */
           WS_POPUP,                                                            /* default window */
           -1000,                                                               /* Windows decides the position */
           -1000,                                                               /* where the window ends up on the screen */
           400,                                                                 /* The programs width */
           375,                                                                 /* and height in pixels */
           HWND_DESKTOP,                                                        /* The window is a child-window to desktop */
           NULL,                                                                /* No menu */
           hThisInstance,                                                       /* Program Instance handler */
           NULL                                                                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    DWORD WinVersion = (DWORD)(LOBYTE(LOWORD(GetVersion())));
    LONG BG_COLOR;
    if (WinVersion > 5)
        BG_COLOR = RGB(200,200,200);
    else
        BG_COLOR = RGB(214,211,206);

    ShowWindow (hwnd, nCmdShow);
    SetLayeredWindowAttributes(hwnd, BG_COLOR, 0, LWA_COLORKEY);

    if (hwnd)
    {
        cout << "OK" << endl;
        cout << "Create parser thread: ";

        EveProbeScannerWindow eveProbeScannerWindowParser;

            // if probeScannerWindowParser wasn't initiate
        if (initSST(hwnd, eveProbeScannerWindowParser) == 0)
        {
            cout << "Error while create parser. Application will close" << endl;
            return 0;
        }
            // scaning thread
        std::thread parserThread(parserThreadFunction, hwnd, eveProbeScannerWindowParser);
        parserThread.detach();
    }
    else
    {
        cout << "ERROR" << endl << "Can't create overlay. Application will close" << endl;
        return 0;
    }

    //WindowProcedure(hwnd, WM_PAINT, -5, NULL);
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);

        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    GdiplusShutdown(gdiplusToken);
    return messages.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
// This function is called by the Windows function DispatchMessage()
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY:
            PostQuitMessage(0);       /* send a WM_QUIT to the message queue */
            break;

        case WM_PAINT:
            {
                InvalidateRect(hwnd, NULL, TRUE);

                PAINTSTRUCT ps;
                HDC hdc;
                hdc=BeginPaint(hwnd,&ps);

                DWORD WinVersion = (DWORD)(LOBYTE(LOWORD(GetVersion())));
                LONG BG_COLOR;
                if (WinVersion > 5)
                    BG_COLOR = RGB(200,200,200);
                else
                    BG_COLOR = RGB(214,211,206);

                winPaintOverlay(hwnd, hdc, BG_COLOR);

                EndPaint(hwnd,&ps);
            }

            break;

        case WM_SET_SIZE:
            {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, wParam, lParam, SWP_NOMOVE | SWP_SHOWWINDOW);
            }

            break;

        case WM_MOVE_TO:
            {
                SetWindowPos(hwnd, HWND_TOPMOST, wParam, lParam, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
            }

            break;

        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
