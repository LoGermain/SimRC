// simRC.cpp : Définit le point d'entrée de l'application.
//

#include "framework.h"
#include "simRC.h"

#include <thread>
#include <atomic>
#include <string>
#include <cmath> 
#include "mozaAPI.h"

#define MAX_LOADSTRING 100

std::atomic<float> g_steeringAngle(0.0f);
std::atomic<float> g_throttle(0.0f);
std::atomic<float> g_brake(0.0f);
std::atomic<bool> g_keepRunning(true);

// Variables globales :
HINSTANCE hInst;                                // instance actuelle
WCHAR szTitle[MAX_LOADSTRING];                  // Texte de la barre de titre
WCHAR szWindowClass[MAX_LOADSTRING];            // nom de la classe de fenêtre principale

// Déclarations anticipées des fonctions incluses dans ce module de code :
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

float MapToPercentage(int16_t rawValue)
{
    const float rawMin = -32768.0f;
    const float rawMax = 32767.0f;

    const float totalRange = rawMax - rawMin;

    float normalizedValue = static_cast<float>(rawValue) - rawMin;

    float percentage = (normalizedValue / totalRange) * 100.0f;

    // 3. Optional: Safety clamping to ensure it never exceeds 0% or 100% due to float rounding
    if (percentage < 0.0f)   percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;

    return percentage;
}

void MozaTelemetryWorker(HWND hWnd)
{
    // Initialize your exact Moza SDK version
    moza::installMozaSDK();

    // TODO test with lower values, to make the program quicker
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    ERRORCODE err = NORMAL;

    while (g_keepRunning)
    {
        // Query the live hardware stream data
        const HIDData* d = moza::getHIDData(err);

        if (d)
        {
            // Verify the steering wheel angle isn't a Not-A-Number (NaN) token
            if (!std::isnan(d->fSteeringWheelAngle))
            {
                // Update our live atomic values directly from your SDK variables
                g_steeringAngle = d->fSteeringWheelAngle;
                g_throttle = MapToPercentage(static_cast<float>(d->throttle));
                g_brake = MapToPercentage(static_cast<float>(d->brake));

                // Tell the Win32 window thread to trigger a fresh WM_PAINT message
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }

        // Poll at ~100Hz (10 milliseconds sleep)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Safely unhook and shut down the Moza context when the app terminates
    moza::removeMozaSDK();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialise les chaînes globales
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMRC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Effectue l'initialisation de l'application :
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // --- FIX: FETCH THE WINDOW HANDLE AND START THE MOZA THREAD ---
    HWND hWnd = FindWindowW(szWindowClass, NULL);
    std::thread telemetryThread(MozaTelemetryWorker, hWnd);
    // -------------------------------------------------------------

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMRC));

    MSG msg;

    // Boucle de messages principale :
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // --- FIX: SHUT DOWN THE THREAD CLEANLY ON EXIT ---
    g_keepRunning = false;
    if (telemetryThread.joinable()) {
        telemetryThread.join();
    }
    // -------------------------------------------------

    return (int)msg.wParam;
}

//
//  FONCTION : MyRegisterClass()
//
//  OBJECTIF : Inscrit la classe de fenêtre.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMRC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SIMRC);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FONCTION : InitInstance(HINSTANCE, int)
//
//   OBJECTIF : enregistre le handle d'instance et crée une fenêtre principale
//
//   COMMENTAIRES :
//
//        Dans cette fonction, nous enregistrons le handle de l'instance dans une variable globale, puis
//        nous créons et affichons la fenêtre principale du programme.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Stocke le handle d'instance dans la variable globale

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FONCTION : WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  OBJECTIF : Traite les messages pour la fenêtre principale.
//
//  WM_COMMAND  - traite le menu de l'application
//  WM_PAINT    - Dessine la fenêtre principale
//  WM_DESTROY  - génère un message d'arrêt et retourne
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Analyse les sélections de menu :
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Format your local atomic data caches into printable Win32 strings
        std::wstring angleStr = L"Steering Angle: " + std::to_wstring(g_steeringAngle.load()) + L" deg";
        std::wstring throttleStr = L"Throttle Input: " + std::to_wstring(g_throttle.load()) + L" %";
        std::wstring brakeStr = L"Brake Input:    " + std::to_wstring(g_brake.load()) + L" %";

        // Print them clearly line-by-line
        TextOutW(hdc, 50, 50, angleStr.c_str(), angleStr.length());
        TextOutW(hdc, 50, 90, throttleStr.c_str(), throttleStr.length());
        TextOutW(hdc, 50, 130, brakeStr.c_str(), brakeStr.length());

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Gestionnaire de messages pour la boîte de dialogue À propos de.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
