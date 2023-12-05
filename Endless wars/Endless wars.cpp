#include "framework.h"
#include "Endless wars.h"
#include "resource.h"
#include <ctime>
#include <mmsystem.h>
#include <d2d1.h>
#include <dwrite.h>
#include "D2BMPLOADER.h"
#include "ErrH.h"
#include "FCheck.h"
#include "workapi.h"
#include <vector>
#include <fstream>

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")
#pragma comment (lib, "d2bmploader.lib")
#pragma comment (lib, "errh.lib")
#pragma comment (lib, "fcheck.lib")
#pragma comment (lib, "workapi.lib")

#define bWinClassName L"MyEndlessWar"
#define scr_width 516
#define scr_height 789  //500 x 700 + 50 for buttons

#define mNew 1001
#define mExit 1002
#define mSave 1003
#define mLoad 1004
#define mHoF 1005

#define temp_file ".\\res\\data\\temp.dat"
#define Ltemp_file L".\\res\\data\\temp.dat"
#define sound_file L".\\res\\snd\\main.wav"
#define record_file L".\\res\\data\\record.dat"
#define save_file L".\\res\\data\\save.dat"
#define help_file L".\\res\\data\\help.dat"

#define record 2001
#define no_record 2002
#define first_record 2003

WNDCLASS bWinClass;
HINSTANCE bIns = nullptr;
HWND bHwnd = nullptr;
HICON mainIcon = nullptr;
HCURSOR mainCursor = nullptr;
HCURSOR outCursor = nullptr;
POINT cur_pos = { 0,0 };
HMENU bBar = nullptr;
HMENU bMain = nullptr;
HMENU bStore = nullptr;
HDC PaintDC = nullptr;
PAINTSTRUCT bPaint;
MSG bMsg;
BOOL bRet = 0;

UINT bTimer = -1;

RECT b1Rect = { 0,0,150,50 };
RECT b2Rect = { 200,0,350,50 };
RECT b3Rect = { 400,0,500,50 };

bool pause = false;
bool sound = true;
bool show_help = false;
bool in_client = true;

bool b1_hglt = false;
bool b2_hglt = false;
bool b3_hglt = false;

bool name_set = false;

float client_width = 0;
float client_height = 0;

int level = 1;
int score = 0;
int seconds = 0;

int bad_move_wait = 5;
int good_move_wait = 5;

wchar_t current_player[16] = L"A PLAYER";
int name_size = 9;

int field_frame = 0;
int field_delay = 0;

D2D1_POINT_2F bad_army_center = { 0,0 };
D2D1_POINT_2F good_army_center = { 0,0 };

////////////////////////////////////////////////////

ID2D1Factory* iFactory = nullptr;
ID2D1HwndRenderTarget* Draw = nullptr;

ID2D1RadialGradientBrush* ButBckg = nullptr;
ID2D1SolidColorBrush* ButTxt = nullptr;
ID2D1SolidColorBrush* HgltButTxt = nullptr;
ID2D1SolidColorBrush* InactiveButTxt = nullptr;
ID2D1SolidColorBrush* ForceTxt = nullptr;

ID2D1SolidColorBrush* GreenLife = nullptr;
ID2D1SolidColorBrush* YellowLife = nullptr;
ID2D1SolidColorBrush* RedLife = nullptr;

IDWriteFactory* iWriteFactory = nullptr;
IDWriteTextFormat* bigTextFormat = nullptr;
IDWriteTextFormat* nrmTextFormat = nullptr;
IDWriteTextFormat* smallTextFormat = nullptr;
///////////////////////////////////////////////////

ID2D1Bitmap* bmpCastle = nullptr;
ID2D1Bitmap* bmpSand = nullptr;
ID2D1Bitmap* bmpEnergy1 = nullptr;
ID2D1Bitmap* bmpEnergy2 = nullptr;
ID2D1Bitmap* bmpEnergy3 = nullptr;
ID2D1Bitmap* bmpEnergy4 = nullptr;
ID2D1Bitmap* bmpKnightL = nullptr;
ID2D1Bitmap* bmpKnightR = nullptr;
ID2D1Bitmap* bmpRock1 = nullptr;
ID2D1Bitmap* bmpRock2 = nullptr;

ID2D1Bitmap* bmpOcean[20];
ID2D1Bitmap* bmpGood[8];
ID2D1Bitmap* bmpBad[40];
/////////////////////////////////////////////////////

Object Castle = nullptr;
Object Knight = nullptr;

std::vector<Warrior> vGoodArmy;
std::vector<Warrior> vBadArmy;

int good_waves = 0;
int bad_waves = 0;

int good_lifes = 480;
int bad_lifes = 480;

/////////////////////////////////////////////////////
template <typename COM> void ReleaseCOM(COM** which)
{
    if ((*which))
    {
        (*which)->Release();
        (*which) = nullptr;
    }
}
void SafeRelease()
{
    ReleaseCOM(&iFactory);
    ReleaseCOM(&Draw);
    ReleaseCOM(&ButBckg);
    ReleaseCOM(&ButTxt);
    ReleaseCOM(&HgltButTxt);
    ReleaseCOM(&InactiveButTxt);
    ReleaseCOM(&ForceTxt);

    ReleaseCOM(&GreenLife);
    ReleaseCOM(&YellowLife);
    ReleaseCOM(&RedLife);
    

    ReleaseCOM(&iWriteFactory);
    ReleaseCOM(&bigTextFormat);
    ReleaseCOM(&nrmTextFormat);
    ReleaseCOM(&smallTextFormat);

    ReleaseCOM(&bmpCastle);
    ReleaseCOM(&bmpEnergy1);
    ReleaseCOM(&bmpEnergy2);
    ReleaseCOM(&bmpEnergy3);
    ReleaseCOM(&bmpEnergy4);
    ReleaseCOM(&bmpKnightL);
    ReleaseCOM(&bmpKnightR);
    ReleaseCOM(&bmpRock1);
    ReleaseCOM(&bmpRock2);
    ReleaseCOM(&bmpSand);

    for (int i = 0; i < 20; ++i)ReleaseCOM(&bmpOcean[i]);
    for (int i = 0; i < 40; ++i)ReleaseCOM(&bmpBad[i]);
    for (int i = 0; i < 8; ++i)ReleaseCOM(&bmpGood[i]);
}
void ErrExit(int which)
{
    MessageBeep(MB_ICONERROR);
    MessageBox(NULL, ErrHandle(which), L"Критична грешка !", MB_OK | MB_APPLMODAL | MB_ICONERROR);

    SafeRelease();
    std::remove(temp_file);
    exit(1);
}

void GameOver()
{
    PlaySound(NULL, NULL, NULL);
    KillTimer(bHwnd, bTimer);


    bMsg.message = WM_QUIT;
    bMsg.wParam = 0;
}
void InitD2D1()
{
    if (!bHwnd)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Base window handle is null !" << std::endl;
        err.close();
        ErrExit(eWindow);
    }

    for (int i = 0; i < 20; ++i)bmpOcean[i] = nullptr;
    for (int i = 0; i < 8; ++i)bmpGood[i] = nullptr;
    for (int i = 0; i < 40; ++i)bmpBad[i] = nullptr;

    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &iFactory);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 iFactory !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    if (bHwnd)
        hr = iFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(bHwnd, D2D1::SizeU((UINT32)(client_width), (UINT32)(client_height))), &Draw);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 iFactory HWND Render Target !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    D2D1_GRADIENT_STOP gStop[2] = {0,0,0,0};
    gStop[0].position = 0.0f;
    gStop[0].color = D2D1::ColorF(D2D1::ColorF::DarkBlue);
    gStop[1].position = 1.0f;
    gStop[1].color = D2D1::ColorF(D2D1::ColorF::CadetBlue);

    ID2D1GradientStopCollection* gStopCol = nullptr;
    hr = Draw->CreateGradientStopCollection(gStop, 2, &gStopCol);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 GradientStopCollection - buttons background !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    if (gStopCol)
        hr = Draw->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(client_width / 2, 15.0f),
            D2D1::Point2F(0, 0), client_width, 15.0f), gStopCol, &ButBckg);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 GradientRadialBrush - buttons background !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }
    if (gStopCol)gStopCol->Release();
    gStopCol = nullptr;

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkRed), &ButTxt);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - buttons text !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGoldenrodYellow), &HgltButTxt);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - buttons highlited text !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), &InactiveButTxt);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - inactive buttons text !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkSeaGreen), &ForceTxt);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - force text !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &GreenLife);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - green life brush !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::IndianRed), &YellowLife);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - yellow life brush !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkRed), &RedLife);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 SolidColorBrush - red life brush !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    
    
    ///////////////////////////////////
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&iWriteFactory));
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 WriteFactory !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    if (iWriteFactory)
        hr = iWriteFactory->CreateTextFormat(L"Gabriola", NULL, DWRITE_FONT_WEIGHT_EXTRA_BLACK, DWRITE_FONT_STYLE_OBLIQUE,
            DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"", &nrmTextFormat);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 nrmTextFormat !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    if (iWriteFactory)
        hr = iWriteFactory->CreateTextFormat(L"Gabriola", NULL, DWRITE_FONT_WEIGHT_EXTRA_BLACK, DWRITE_FONT_STYLE_OBLIQUE,
            DWRITE_FONT_STRETCH_NORMAL, 64.0f, L"", &bigTextFormat);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 bigTextFormat !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    if (iWriteFactory)
        hr = iWriteFactory->CreateTextFormat(L"Gabriola", NULL, DWRITE_FONT_WEIGHT_EXTRA_BLACK, DWRITE_FONT_STYLE_OBLIQUE,
            DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"", &smallTextFormat);
    if (hr != S_OK)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1 smallTextFormat !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpCastle = Load(L".\\res\\img\\castle.png", Draw);
    if (!bmpCastle)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Castle !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpEnergy1 = Load(L".\\res\\img\\energyfield1.png", Draw);
    if (!bmpEnergy1)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Energy field 1 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpEnergy2 = Load(L".\\res\\img\\energyfield2.png", Draw);
    if (!bmpEnergy2)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Energy field 2 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpEnergy3 = Load(L".\\res\\img\\energyfield1.png", Draw);
    if (!bmpEnergy3)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Energy field 3 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpEnergy4 = Load(L".\\res\\img\\energyfield4.png", Draw);
    if (!bmpEnergy4)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Energy field 4 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpKnightL = Load(L".\\res\\img\\knightl.png", Draw);
    if (!bmpKnightL)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for KnightL !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpKnightR = Load(L".\\res\\img\\knightr.png", Draw);
    if (!bmpKnightR)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for KnightR !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpRock1 = Load(L".\\res\\img\\rock1.png", Draw);
    if (!bmpRock1)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Rock1 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpRock2 = Load(L".\\res\\img\\rock2.png", Draw);
    if (!bmpRock2)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Rock2 !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    bmpSand = Load(L".\\res\\img\\sand.png", Draw);
    if (!bmpSand)
    {
        std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
        err << L"Error creating D2D1Bitmap for Sand !" << std::endl;
        err.close();
        ErrExit(eD2D);
    }

    for (int i = 0; i < 20; ++i)
    {
        wchar_t name[50] = L".\\res\\img\\ocean\\";
        wchar_t add[3] = L"\0";
        wsprintf(add, L"%d", i);
        wcscat_s(name, add);
        wcscat_s(name, L".png");

        bmpOcean[i] = Load(name, Draw);

        if (!bmpOcean[i])
        {
            std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
            err << L"Error creating D2D1Bitmap for Ocean !" << std::endl;
            err.close();
            ErrExit(eD2D);
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        wchar_t name[50] = L".\\res\\img\\good\\";
        wchar_t add[3] = L"\0";
        wsprintf(add, L"%d", i);
        wcscat_s(name, add);
        wcscat_s(name, L".png");

        bmpGood[i] = Load(name, Draw);

        if (!bmpGood[i])
        {
            std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
            err << L"Error creating D2D1Bitmap for Good !" << std::endl;
            err.close();
            ErrExit(eD2D);
        }
    }

    for (int i = 0; i < 40; ++i)
    {
        wchar_t name[50] = L".\\res\\img\\bad\\";
        wchar_t add[3] = L"\0";
        wsprintf(add, L"%d", i);
        wcscat_s(name, add);
        wcscat_s(name, L".png");

        bmpBad[i] = Load(name, Draw);

        if (!bmpBad[i])
        {
            std::wofstream err(L".\\res\\data\\error.log", std::ios::app);
            err << L"Error creating D2D1Bitmap for Bad !" << std::endl;
            err.close();
            ErrExit(eD2D);
        }
    }

    wchar_t start_text[27] = L"ПРЕДЕН ПОСТ !\n\ndev. Daniel";
    wchar_t show_text[27] = L"\0";

    for (int i = 0; i < 27; i++)
    {
        mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
        Draw->BeginDraw();
        Draw->Clear(D2D1::ColorF(D2D1::ColorF::DarkCyan));
        show_text[i] = start_text[i];
        if (bigTextFormat && ButTxt)
            Draw->DrawText(show_text, i, bigTextFormat, D2D1::RectF(80.0f, 200.0f, client_width, client_height),
                ButTxt);
        Draw->EndDraw();
        Sleep(50);
    }
    Sleep(2000);
    
}
void InitGame()
{
    wcscpy_s(current_player, L"A PLAYER");
    name_size = 9;
    name_set = false;
    level = 1;
    score = 0;
    seconds = 0;

    good_waves = level + 5;
    bad_waves = level + 8;
    good_lifes = 480;
    bad_lifes = 480;


    vGoodArmy.clear();
    vBadArmy.clear();

    good_army_center = { 0,0 };
    bad_army_center = { 0,0 };

    if (Castle)Castle->Release();
    float randx = (float)(rand() % 250);
    if (randx < 100.0f)randx = 100.0f;

    Castle = new OBJECT(randx, 70.0f, 150.0f, 120.0f);

    if (Knight)Knight->Release();
    randx = (float)(rand() % 320);
    if (randx < 100.0f)randx = 100.0f;
    Knight = new OBJECT(randx, 570.0f, 80.0f, 97.0f);
    if (Knight)Knight->dir = dirs::right;
}

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT ReceivedMsg, WPARAM wParam, LPARAM lParam)
{
    switch (ReceivedMsg)
    {
    case WM_INITDIALOG:
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)mainIcon);
        return true;
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;

        case IDOK:
            name_size = GetDlgItemText(hwnd, IDC_NAME, current_player, 15);
            if (name_size < 1)
            {
                wcscpy_s(current_player, L"A PLAYER");
                name_size = 9;
                if (sound)MessageBeep(MB_ICONASTERISK);
                MessageBox(bHwnd, L"Ха, ха, ха ! Забрави си името !", L"Забраватор !", MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION);
                EndDialog(hwnd, IDCANCEL);
                break;
            }
            EndDialog(hwnd, IDOK);
        }
        break;
    }

    return (INT_PTR)FALSE;
}
LRESULT CALLBACK WinProc(HWND hwnd, UINT ReceivedMsg, WPARAM wParam, LPARAM lParam)
{
    switch (ReceivedMsg)
    {
    case WM_CREATE:
        {
            RECT clr = { 0,0,0,0 };
            GetClientRect(hwnd, &clr);
            client_width = (float)(clr.right);
            client_height = (float)(clr.bottom);
            srand((unsigned int)(time(0)));
            SetTimer(hwnd, bTimer, 1000, NULL);

            bBar = CreateMenu();
            bMain = CreateMenu();
            bStore = CreateMenu();

            AppendMenu(bBar, MF_POPUP, (UINT_PTR)bMain, L"Основно меню");
            AppendMenu(bBar, MF_POPUP, (UINT_PTR)bStore, L"Меню за данни");

            AppendMenu(bMain, MF_STRING, mNew, L"Нова игра");
            AppendMenu(bMain, MF_SEPARATOR, NULL, NULL);
            AppendMenu(bMain, MF_STRING, mExit, L"Изход");

            AppendMenu(bStore, MF_STRING, mSave, L"Запази игра");
            AppendMenu(bStore, MF_STRING, mLoad, L"Зареди игра");
            AppendMenu(bStore, MF_SEPARATOR, NULL, NULL);
            AppendMenu(bStore, MF_STRING, mHoF, L"Зала на славата");
            SetMenu(hwnd, bBar);
        
            InitGame();
        }
        break;

    case WM_CLOSE:
        pause = true;
        if (sound)MessageBeep(MB_ICONASTERISK);
        if (MessageBox(hwnd, L"Ако не си я запазил, ще загубиш тази игра !\n\nНаистина ли излизаш ?",
            L"Изход ?", MB_YESNO | MB_APPLMODAL | MB_ICONQUESTION) == IDNO)
        {
            pause = false;
            break;
        }
        GameOver();
        break;

    case WM_SETCURSOR:
        GetCursorPos(&cur_pos);
        ScreenToClient(hwnd, &cur_pos);

        if (LOWORD(lParam) == HTCLIENT)
        {
            if (!in_client)
            {
                in_client = true;
                pause = false;
            }

            if (cur_pos.y < 50)
            {
                if (cur_pos.x >= b1Rect.left && cur_pos.x <= b1Rect.right)
                {
                    if (!b1_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b1_hglt = true;
                    }
                }
                else
                {
                    if (b1_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b1_hglt = false;
                    }
                }

                if (cur_pos.x >= b2Rect.left && cur_pos.x <= b2Rect.right)
                {
                    if (!b2_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b2_hglt = true;
                    }
                }
                else
                {
                    if (b2_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b2_hglt = false;
                    }
                }

                if (cur_pos.x >= b3Rect.left && cur_pos.x <= b3Rect.right)
                {
                    if (!b3_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b3_hglt = true;
                    }
                }
                else
                {
                    if (b3_hglt)
                    {
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                        b3_hglt = false;
                    }
                }
            
                SetCursor(outCursor);
            }
            else
            {
                if (b1_hglt || b2_hglt || b3_hglt)
                {
                    if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                    b1_hglt = false;
                    b2_hglt = false;
                    b3_hglt = false;
                }
                SetCursor(mainCursor);
            }

            return true;
            break;
        }
        else
        {
            if (in_client)
            {
                in_client = false;
                pause = true;
            }
            if (b1_hglt || b2_hglt || b3_hglt)
            {
                if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                b1_hglt = false;
                b2_hglt = false;
                b3_hglt = false;
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return true;
        }
        break;

    case WM_PAINT:
        PaintDC = BeginPaint(hwnd, &bPaint);
        FillRect(PaintDC, &bPaint.rcPaint, CreateSolidBrush(RGB(50, 50, 50)));
        EndPaint(hwnd, &bPaint);
        break;

    case WM_TIMER:
        if (pause)break;
        if (seconds % 20 == 0 && bad_waves > 0)
        {
            seconds = 1;
            for (int i = 0; i < 10; i++)
            {
                if (Castle)
                    vBadArmy.push_back(iCreateWarrior(types::bad, (float)(Castle->x + rand() % 150), Castle->ey));
            }
            bad_waves--;
        }
        seconds++;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case mNew:
            pause = true;
            if (sound)MessageBeep(MB_ICONASTERISK);
            if (MessageBox(hwnd, L"Ако не си я запазил, ще загубиш тази игра !\n\nНаистина ли рестартираш ?",
                L"Рестарт ?", MB_YESNO | MB_APPLMODAL | MB_ICONQUESTION) == IDNO)
            {
                pause = false;
                break;
            }
            InitGame();
            break;

        case mExit:
            SendMessage(hwnd, WM_CLOSE, NULL, NULL);
            break;

        }
        break;

    case WM_LBUTTONDOWN:
        if (pause)break;
        if (good_waves > 0)
        {
            if (Knight)
            {
                for (int i = 0; i < 10; ++i)
                {
                    if (Knight)
                        vGoodArmy.push_back(iCreateWarrior(types::good, (float)(Knight->x + rand() %80), Knight->y));
                }
                good_waves--;
            }
        }
        else if (sound)mciSendString(L"play .\\res\\negative.wav", NULL, NULL, NULL);
        break;

    default: return DefWindowProc(hwnd, ReceivedMsg, wParam, lParam);
    }

    return (LRESULT)FALSE;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    bIns = hInstance;
    if (!bIns)ErrExit(eClass);

    int tmp_res = 0;
    CheckFile(Ltemp_file, &tmp_res);
    if (tmp_res == FILE_EXIST)ErrExit(eStarted);
    else
    {
        std::wofstream tmp(Ltemp_file);
        tmp << L"Game is started and working !";
        tmp.close();
    }

    if (GetSystemMetrics(SM_CXSCREEN) + 200 < scr_width || GetSystemMetrics(SM_CXSCREEN) + 20 < scr_height)ErrExit(eScreen);
    
    mainIcon = (HICON)LoadImage(NULL, L".\\res\\main.ico", IMAGE_ICON, 80, 97, LR_LOADFROMFILE);
    if (!mainIcon)ErrExit(eIcon);

    mainCursor = LoadCursorFromFile(L".\\res\\bcursor.ani");
    outCursor = LoadCursorFromFile(L".\\res\\out.ani");
    if (!mainCursor || !outCursor)ErrExit(eCursor);

    ZeroMemory(&bWinClass, sizeof(WNDCLASS));

    bWinClass.lpszClassName = bWinClassName;
    bWinClass.hInstance = bIns;
    bWinClass.lpfnWndProc = &WinProc;
    bWinClass.hIcon = mainIcon;
    bWinClass.hCursor = mainCursor;
    bWinClass.hbrBackground = CreateSolidBrush(RGB(50, 50, 50));
    bWinClass.style = CS_DROPSHADOW;

    if (!RegisterClass(&bWinClass))ErrExit(eClass);

    bHwnd = CreateWindow(bWinClassName, L"ПРЕДЕН ПОСТ", WS_CAPTION | WS_SYSMENU, 100, 50, scr_width, scr_height,
        NULL, NULL, bIns, NULL);
    if (!bHwnd)ErrExit(eWindow);
    else
    {
        ShowWindow(bHwnd, SW_SHOWDEFAULT);
        InitD2D1();
    }
    
    //MAIN MSG LOOP *******************************

    while (bMsg.message != WM_QUIT)
    {
        if ((bRet = PeekMessage(&bMsg, bHwnd, NULL, NULL, PM_REMOVE)) != 0)
        {
            if (bRet == -1)ErrExit(eMsg);
            TranslateMessage(&bMsg);
            DispatchMessage(&bMsg);
        }

        if (pause)
        {
            if (show_help)continue;

            Draw->BeginDraw();
            Draw->Clear(D2D1::ColorF(D2D1::ColorF::DarkCyan));
            if (bigTextFormat && ButTxt)
                Draw->DrawText(L"ПАУЗА", 6, bigTextFormat, D2D1::RectF(180.0f, client_height / 2 - 50, client_width, client_height),
                    ButTxt);
            Draw->EndDraw();
            continue;
        }
        ///////////////////////////////////////////////////////////

        // GAME ENGINE *************************************

        if (Knight)
        {
            if (Knight->dir == dirs::right)
            {
                Knight->x += 0.5f;
                Knight->SetDims();
                if (Knight->ex >= 400.0f)Knight->dir = dirs::left;
            }
            if (Knight->dir == dirs::left)
            {
                Knight->x -= 0.5f;
                Knight->SetDims();
                if (Knight->x <= 100.0f)Knight->dir = dirs::right;
            }

        }
        
        ///////////////////////////////////////////////////

        if (!vGoodArmy.empty())
        {
            if (good_move_wait < 0)
            {
                good_move_wait = 5;

                good_army_center.x = (*vGoodArmy.begin())->ex;
                good_army_center.y = (*vGoodArmy.begin())->ey;
               
                for (std::vector<Warrior>::iterator good = vGoodArmy.begin(); good < vGoodArmy.end(); ++good)
                {
                    (*good)->Move(bad_army_center.x, bad_army_center.y);
                    if ((*good)->OutOfScreen(-1, Castle->y, false, false))
                    {
                        (*good)->Release();
                        vGoodArmy.erase(good);
                        score += 10 + 2 * level;
                        bad_lifes -= 20;
                        break;
                    }
                }
            } else good_move_wait--;
        }
        else if (Knight && good_move_wait == 5)
        {
            good_army_center.x = Knight->x;
            good_army_center.y = Knight->ey;
        }

        if (!vBadArmy.empty())
        {
            if (bad_move_wait < 0)
            {
                bad_move_wait = 5;
                bad_army_center.x = (*vBadArmy.begin())->x;
                bad_army_center.y = (*vBadArmy.begin())->y;
                
                
                for (std::vector<Warrior>::iterator bad = vBadArmy.begin(); bad < vBadArmy.end(); ++bad)
                {
                    (*bad)->Move(good_army_center.x, good_army_center.y);
                    if ((*bad)->OutOfScreen(Knight->x, Knight->y, false, true))
                    {
                        good_lifes -= 20;
                        (*bad)->Release();
                        vBadArmy.erase(bad);
                        break;
                    }
                }
            }
            else bad_move_wait--;
        }
        else if (Castle && bad_move_wait == 5)
        {
            bad_army_center.x = Castle->x;
            bad_army_center.y = Castle->y;
        }


        //FIGHT ************************************

        if (!vGoodArmy.empty() && !vBadArmy.empty())
        {
            bool killed = false;
            for (std::vector<Warrior>::iterator good = vGoodArmy.begin(); good < vGoodArmy.end(); good++)
            {
                for (std::vector<Warrior>::iterator bad = vBadArmy.begin(); bad < vBadArmy.end(); bad++)
                {
                    if (!((*good)->x >= (*bad)->ex || (*good)->ex <= (*bad)->x ||
                        (*good)->y >= (*bad)->ey || (*good)->ey <= (*bad)->y))
                    {
                        switch (rand() % 2)
                        {
                        case 0:
                            score += 10 + 2 * level;
                            killed = true;
                            (*bad)->Release();
                            vBadArmy.erase(bad);
                            break;
                            
                        case 1:
                            killed = true;
                            (*good)->Release();
                            vGoodArmy.erase(good);
                            break;
                        }
                        break;
                    }
                }
                if (killed)break;
            }
        }


        //////////////////////////////////////////

        
        //////////////////////////////////////////////////////////////////////////////////
        //DRAW FIELD *********************************

        field_delay++;
        if (field_delay > 4)
        {
            field_delay = 0;
            field_frame++;
            if (field_frame > 19)field_frame = 0;
        }
        Draw->BeginDraw();
        Draw->DrawBitmap(bmpOcean[field_frame], D2D1::RectF(0, 50.0f, client_width, client_height));
        Draw->DrawBitmap(bmpSand, D2D1::RectF(100, 50.0f, client_width - 100.0f, client_height));
        Draw->FillRectangle(D2D1::RectF(0, 0, client_width, 50.0f), ButBckg);

        if (nrmTextFormat && ButTxt && HgltButTxt && InactiveButTxt)
        {
            if (name_set)
                Draw->DrawText(L"Име на рицар", 13, nrmTextFormat, D2D1::RectF(10.0f, 0, (float)b1Rect.right, 50.0f), 
                    InactiveButTxt);
            else
            {
                if (!b1_hglt)
                    Draw->DrawText(L"Име на рицар", 13, nrmTextFormat, D2D1::RectF(10.0f, 0, (float)b1Rect.right, 50.0f),
                        ButTxt);
                else
                    Draw->DrawText(L"Име на рицар", 13, nrmTextFormat, D2D1::RectF(10.0f, 0, (float)b1Rect.right, 50.0f),
                        HgltButTxt);
            }

            if (!b2_hglt)
                Draw->DrawText(L"Звуци ON / OFF", 15, nrmTextFormat, D2D1::RectF(210.0f, 0, (float)b2Rect.right, 50.0f),
                    ButTxt);
            else
                Draw->DrawText(L"Звуци ON / OFF", 15, nrmTextFormat, D2D1::RectF(210.0f, 0, (float)b2Rect.right, 50.0f),
                    HgltButTxt);

            if (!b3_hglt)
                Draw->DrawText(L"Помощ", 6, nrmTextFormat, D2D1::RectF(410.0f, 0, (float)b3Rect.right, 50.0f),
                    ButTxt);
            else
                Draw->DrawText(L"Помощ", 6, nrmTextFormat, D2D1::RectF(410.0f, 0, (float)b3Rect.right, 50.0f),
                    HgltButTxt);
        }

        if (Castle)Draw->DrawBitmap(bmpCastle, D2D1::RectF(Castle->x, Castle->y, Castle->ex, Castle->ey));
        if (Knight)
        {
            if (Knight->dir == dirs::right)
                Draw->DrawBitmap(bmpKnightR, D2D1::RectF(Knight->x, Knight->y, Knight->ex, Knight->ey));
            else if (Knight->dir == dirs::left)
                Draw->DrawBitmap(bmpKnightL, D2D1::RectF(Knight->x, Knight->y, Knight->ex, Knight->ey));

            if (GreenLife && YellowLife && RedLife)
            {
                if (good_lifes > 400)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 680.0f), D2D1::Point2F((float)(good_lifes / 2), 680.0f), GreenLife, 10.0f);
                else if (good_lifes <= 400 && good_lifes >= 100)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 680.0f), D2D1::Point2F((float)(good_lifes / 2), 680.0f), YellowLife, 10.0f);
                else if (good_lifes < 100)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 680.0f), D2D1::Point2F((float)(good_lifes / 2), 680.0f), RedLife, 10.0f);
            
                if (bad_lifes > 400)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 60.0f), D2D1::Point2F((float)(bad_lifes / 2), 60.0f), GreenLife, 10.0f);
                else if (bad_lifes <= 400 && bad_lifes >= 100)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 60.0f), D2D1::Point2F((float)(bad_lifes / 2), 60.0f), YellowLife, 10.0f);
                else if (bad_lifes < 100)
                    Draw->DrawLine(D2D1::Point2F(120.0f, 60.0f), D2D1::Point2F((float)(bad_lifes / 2), 60.0f), RedLife, 10.0f);
            }

        }

        /////////////////////////////////////////////////////////////////////////////////

        //DRAW TROOPS *********************************

        if (!vGoodArmy.empty())
        {
            for (int i = 0; i < vGoodArmy.size(); ++i)
            {
                Draw->DrawBitmap(bmpGood[vGoodArmy[i]->GetFrame()],
                    D2D1::RectF(vGoodArmy[i]->x, vGoodArmy[i]->y, vGoodArmy[i]->ex, vGoodArmy[i]->ey));
            }
        }

        if (!vBadArmy.empty())
        {
            for (int i = 0; i < vBadArmy.size(); ++i)
            {
                Draw->DrawBitmap(bmpBad[vBadArmy[i]->GetFrame()],
                    D2D1::RectF(vBadArmy[i]->x, vBadArmy[i]->y, vBadArmy[i]->ex, vBadArmy[i]->ey));
            }
        }

        ///////////////////////////////////////////
        Draw->EndDraw();
    }

    /////////////////////////////////////////////////
    SafeRelease();
    std::remove(temp_file);
    return (int) bMsg.wParam;
}