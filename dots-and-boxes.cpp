////////////////////////////////////////////////////////////////////////////////
//
//  dots-and-boxes.cpp
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ヘッダファイル
//
#include <Windows.h>
#include <WinSock.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
//
// 使用ライブラリ
//
#pragma comment(lib, "wsock32.lib")

////////////////////////////////////////////////////////////////////////////////
//
//  定数定義
//
#define WM_SOCKET (WM_USER + 1) // ソケット用メッセージ
#define PORT 20000				// 通信ポート番号

//コントロールID////////////
// Screen10
#define IDB_CONNECT 1000	  // [対戦する]ボタン
#define IDB_CONNECTCANCEL 1003
#define IDB_ACCEPT 1001		  // [対戦待ち]ボタン
#define IDB_ACCEPTCANCEL 1002 // [切断]ボタン
#define IDB_BUTTON 1006
#define IDB_EXPLANATION 1007
#define IDB_RECORD 1008
#define IDF_HOSTNAME 1009 // ホスト名入力エディットボックス
#define IDF_PLAYERNAME 1010 //プレイヤー名入力エディットボックス

// Screen20
#define IDB_REJECT 2000
#define IDL_VER 2001
#define IDL_HOL 2002
#define IDR_Player1 2003
#define IDR_Player2 2004
#define IDR_Round1 2005
#define IDR_Round3 2006
#define IDR_Round5 2007
#define IDB_PLAYSTART 2008
#define IDB_REFERENCE  2009

// Screen21
#define IDB_COLOR 2101
#define WM_INVALIDATE 2102

// Screen30

#define WM_SETUP 3002      //メッセージ

// Screen40, 41
#define IDB_AGAIN 4000 // [もう一度]ボタン
#define IDB_TITLE 4001 // [タイトルへ]ボタン
#define IDB_NEXT 4002  // [次の試合へ]ボタン

#define IDE_RECVMSG 3000 // メッセージ受信イベント

//ゲーム画面関連/////////////////////////////////////
#define WINDOW_W 700 // ウィンドウの幅
#define WINDOW_H 800 // ウィンドウの高さ

#define MAX_MESSAGE 64 // 配列の最大要素数

#define MAX_BOARD_W_NUM 8						  // ボードの横のマス目の最大の数
#define MAX_BOARD_H_NUM 8						  // ボードの縦のマス目の最大の数
#define MAX_BOARD_DOT_W_NUM (MAX_BOARD_W_NUM + 1) // ボードの横のマス目の最大の数
#define MAX_BOARD_DOT_H_NUM (MAX_BOARD_H_NUM + 1) // ボードの縦のマス目の最大の数


#define RATIO_OF_DOT_CIRCLE_RADIUS 0.1		  // 1辺の長さのRATIO_OF_DOT_CIRCLE_RADIUS倍が、ボードの点を表す丸（円）の半径
#define RATIO_OF_JUDGEMENT_CIRCLE_RADIUS 0.33 // 1辺の長さのRATIO_OF_JUDGE_CIRCLE_RADIUS倍が、ボードの点の判定範囲となる円の半径
#define RATIO_OF_SQUARE_MARGIN 0.1			  // 1辺の長さのRATIO_OF_SQUARE_MARGIN倍が、マスの中の角丸長方形とマスを表す正方形の間の余白の長さ

////////////////////////////////////////////////////////////////////////////////
//
//  グローバル変数
//

// ウインドウクラス
LPCTSTR lpWindowName = "ドット＆ボックス"; // タイトルバーにつく名前
LPCTSTR lpClassName10 = "Screen10";		 // ウィンドウクラス名
LPCTSTR lpClassName20 = "Screen20";
LPCTSTR lpClassName21 = "Screen21";
LPCTSTR lpClassName30 = "Screen30";
LPCTSTR lpClassName40 = "Screen40";
LPCTSTR lpClassName41 = "Screen41";

//通信関係
SOCKET sock = INVALID_SOCKET;	 // ソケット
SOCKET sv_sock = INVALID_SOCKET; // サーバ用ソケット
HOSTENT* phe;					 // HOSTENT構造体]

// gc資源（ペン、ブラシ、フォント等）//////
HPEN hPen, hPenPrev;
HBRUSH hBrush, hBrushPrev;
HFONT hfon, hfonPrev;
//個別のペン、ブラシ、フォント
HPEN hPenGray;
HPEN hPenDef_Col_2;
HBRUSH hBrush_Def_Col_2;
HBRUSH hBrushGray;
HPEN hPenBk2px;
HPEN hPenBlackDot;
HFONT hfon1, hfon2;
HFONT hfon100px;

//名前
char Player1_name[64];
char Player2_name[64];
char host[64];

//色
COLORREF Player1_Default_Color = 0x77FF23;
COLORREF Player2_Default_Color = 0xB221FF;
COLORREF Player1_Color = Player1_Default_Color;
COLORREF Player2_Color = Player2_Default_Color;

//点数
int point_1 = 0, point_2 = 0; // プレイヤー1の得点、プレイヤー2の得点
int Points_1[5] = { 0, 0, 0, 0, 0 };
int Points_2[5] = { 0, 0, 0, 0, 0 };
int phase = 1;      // 今のフェイズ(1：プレイヤー1のフェイズ、2：プレイヤー2のフェイズ)
int round_mode = 1;     //ゲームのモード設定
int round_num = 1; //現在のround数
int score_1 = 0;
int score_2 = 0;

//リストボックスの要素（縦×横の数）
LPCTSTR strText[] = {
	TEXT("2"),
	TEXT("3"),
	TEXT("4"),
	TEXT("5"),
	TEXT("6"),
	TEXT("7"),
	TEXT("8") };

// 点を表す構造体（メンバ変数(x,y)はint型の点の座標）
typedef struct tagDot
{
	int x;
	int y;

} DOT;

//各ウインドウ（親ウインドウ・コントロール）/////////////////////////////////
// Screen10
HWND hWnd10;
static HWND hWndPlayer;				 // プレイヤー名入力用エディットボックス
static HWND hWndHost;				 // ホスト名入力用エディットボックス
static HWND hWndConnect, hWndAccept; // [対戦する]ボタンと[対戦待ち]ボタン
static HWND hWndConnectCancel, hWndAcceptCancel;
static HWND hWndRequest;
static HWND hWndSend; // [送信]ボタン
static HWND hbutton;
static HWND hWndExplanation;
static HWND hWndRecord;
static HWND label10;

// Screen20
HWND hWnd20;
static HWND hWndPlayStart;
static HWND hWndReject; // [切断]ボタン
static HWND hWndReference;
static HWND hListVer;
static HWND hListHol;
static HWND hRadPlayer1;
static HWND hRadPlayer2;
static HWND hRadRound1;
static HWND hRadRound3;
static HWND hRadRound5;
static HWND label;


// Screen21
HWND hWnd21;
static HWND hWndSelectColor;

// Screen30
HWND hWnd30;
//static HWND hWndReject; // [切断]ボタン

// Screen40
HWND hWnd40;
static HWND hWndAgain40; // [もう一度]ボタン
static HWND hWndTitle40; // [タイトルへ]ボタン

//Screen41
HWND hWnd41;
static HWND hWndAgain41; // [もう一度]ボタン
static HWND hWndTitle41; // [タイトルへ]ボタン
static HWND hWndNext;  // [次の試合へ]ボタン


///////////////////////////////////////////////////////////////////////////

//ゲーム関連///////////////////////////////////////////////////////////////
HPEN hPenPlayer1;	// プレイヤー1のペン
HPEN hPenPlayer2;	// プレイヤー2のペン
HPEN hPenDot;		// 灰色の破線のペン（ボードの線の色）
HBRUSH hBrushWhite; // 白ブラシ（背景）

HBRUSH hBrushPlayer1; // プレイヤー1のブラシ
HBRUSH hBrushPlayer2; // プレイヤー2のブラシ

int board_w_num = 3;  // ボードの横のマス目の数（線の個数も同様）
int board_h_num = 3;  // ボードの縦のマス目の数（線の個数も同様）

int board_dot_w_num = board_w_num + 1; // ボードの点の横の個数
int board_dot_h_num = board_h_num + 1; // ボードの点の縦の個数

const RECT d = { 0, 0, WINDOW_W, WINDOW_H }; // 描画領域(左上隅のx座標, 左上隅のy座標, 右下隅のx座標, 右下隅のy座標)

DOT top_left, top_right, bottom_left, bottom_right; // ボードの4隅の点

DOT board_dot[MAX_BOARD_DOT_H_NUM][MAX_BOARD_DOT_W_NUM] = { {0} }; // ボードの各点

int vir_line[MAX_BOARD_H_NUM][MAX_BOARD_W_NUM + 1] = { {0} }; // ボードの縦線の状態
int hor_line[MAX_BOARD_H_NUM + 1][MAX_BOARD_W_NUM] = { {0} }; // ボードの横線の状態
// （0：線が引かれていない状態、1：プレイヤー1の線、2：プレイヤー2の線）

int square[MAX_BOARD_H_NUM][MAX_BOARD_W_NUM] = { {0} }; // ボードのマスの状態
// （0：マスがまだ塗られていない状態、1：プレイヤー1のマス、2：プレイヤー2のマス）

DOT mid_board_pos; // ボードの中心の点
int len_square;	   // ボードのマスの1辺の長さ

int my_id = 0;		// ID(1：サーバー側、2：クライアント側、0：通信接続前)

int next1 = 0; //次の試合へのフラグ
int next2 = 0;

int again1 = 0; //もう一度　のフラグ
int again2 = 0;

int PlayStart1 = 0; //ゲームを始める　のフラグ
int PlayStart2 = 0;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  プロトタイプ宣言
//

//ウインドウ関数
LRESULT CALLBACK WindowProc10(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowProc20(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowProc21(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowProc30(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowProc40(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WindowProc41(HWND, UINT, WPARAM, LPARAM);

//通信関係の関数
BOOL SockInit(HWND hWnd);				  // ソケット初期化
BOOL SockAccept(HWND hWnd);				  // ソケット対戦待ち
BOOL SockConnect(HWND hWnd, LPCSTR host); // ソケット対戦する
void WindowInit(HWND hWnd);				  // ウィンドウ初期化

// OwnerDraw関数
LRESULT OnDrawItem(HWND, UINT, WPARAM, LPARAM);
void ButtonDraw(HDC, RECT*, BOOL);

//ゲーム関係
LRESULT CALLBACK OnPaint(HWND, UINT, WPARAM, LPARAM);			 // 描画関数
BOOL judgeDotInCircle(DOT, int, int*, int*);					 // 引数の点がボードの各点のいずれかの周り（引数の半径の円の内部）にあるか判定する
int getDist(DOT, DOT);											 // 2点間ABの距離を整数値で返す
void switchPhase(int* phase);									 // フェイズを切り替える
BOOL updateSquare(BOOL is_vir_line, int line, int row, int val); // 引数の線[line][row]に関わるマスのみを判定し、マスが囲まれていた場合は、そのマスに値valをセットして更新する
																 // 返り値はhas_updated（更新したらTRUE、更新しなかったらFALSE）
BOOL is_board_full();											 // ボードのすべてのマスが塗られていたら、TRUEを返す。そうでなかったらFALSEを返す
int count_square(int val);										 // ボードのマスのうち、引数の値をもつマスの個数を数える

////////////////////////////////////////////////////////////////////////////////
//
//  WinMain関数 (Windowsプログラム起動時に呼ばれる関数)
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;									   // メッセージ
	WNDCLASSEX wc10, wc20, wc21, wc30, wc40, wc41; // ウィンドウクラス

	hBrushWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);

	//ウィンドウクラス定義
	wc10.hInstance = hInstance;					  // インスタンス
	wc10.lpszClassName = lpClassName10;			  // クラス名
	wc10.lpfnWndProc = WindowProc10;			  // ウィンドウ関数名
	wc10.style = 0;								  // クラススタイル
	wc10.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc10.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc10.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc10.hCursor = LoadCursor(NULL, IDC_HAND);	  // マウスポインタ
	wc10.lpszMenuName = NULL;					  // メニュー(なし)
	wc10.cbClsExtra = 0;						  // クラス拡張情報
	wc10.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc10.hbrBackground = hBrushWhite;			  // ウィンドウの背景色
	if (!RegisterClassEx(&wc10))
		return 0; // ウィンドウクラス登録

	wc20.hInstance = hInstance;					  // インスタンス
	wc20.lpszClassName = lpClassName20;			  // クラス名
	wc20.lpfnWndProc = WindowProc20;			  // ウィンドウ関数名
	wc20.style = 0;								  // クラススタイル
	wc20.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc20.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc20.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc20.hCursor = LoadCursor(NULL, IDC_HAND);	  // マウスポインタ
	wc20.lpszMenuName = NULL;					  // メニュー(なし)
	wc20.cbClsExtra = 0;						  // クラス拡張情報
	wc20.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc20.hbrBackground = hBrushWhite;			  // ウィンドウの背景色
	if (!RegisterClassEx(&wc20))
		return 0; // ウィンドウクラス登録

	wc21.hInstance = hInstance;					  // インスタンス
	wc21.lpszClassName = lpClassName21;			  // クラス名
	wc21.lpfnWndProc = WindowProc21;			  // ウィンドウ関数名
	wc21.style = 0;								  // クラススタイル
	wc21.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc21.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc21.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc21.hCursor = LoadCursor(NULL, IDC_HAND);	  // マウスポインタ
	wc21.lpszMenuName = NULL;					  // メニュー(なし)
	wc21.cbClsExtra = 0;						  // クラス拡張情報
	wc21.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc21.hbrBackground = hBrushWhite;			  // ウィンドウの背景色
	if (!RegisterClassEx(&wc21))
		return 0; // ウィンドウクラス登録

	wc30.hInstance = hInstance;					  // インスタンス
	wc30.lpszClassName = lpClassName30;			  // クラス名
	wc30.lpfnWndProc = WindowProc30;			  // ウィンドウ関数名
	wc30.style = 0;								  // クラススタイル
	wc30.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc30.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc30.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc30.hCursor = LoadCursor(NULL, IDC_ARROW);	  // マウスポインタ
	wc30.lpszMenuName = NULL;					  // メニュー(なし)
	wc30.cbClsExtra = 0;						  // クラス拡張情報
	wc30.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc30.hbrBackground = (HBRUSH)hBrushWhite;	  // ウィンドウの背景色
	if (!RegisterClassEx(&wc30))
		return 0; // ウィンドウクラス登録

	wc40.hInstance = hInstance;					  // インスタンス
	wc40.lpszClassName = lpClassName40;			  // クラス名
	wc40.lpfnWndProc = WindowProc40;			  // ウィンドウ関数名
	wc40.style = 0;								  // クラススタイル
	wc40.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc40.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc40.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc40.hCursor = LoadCursor(NULL, IDC_HAND);	  // マウスポインタ
	wc40.lpszMenuName = NULL;					  // メニュー(なし)
	wc40.cbClsExtra = 0;						  // クラス拡張情報
	wc40.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc40.hbrBackground = hBrushWhite;			  // ウィンドウの背景色
	if (!RegisterClassEx(&wc40))
		return 0; // ウィンドウクラス登録

	wc41.hInstance = hInstance;					  // インスタンス
	wc41.lpszClassName = lpClassName41;			  // クラス名
	wc41.lpfnWndProc = WindowProc41;			  // ウィンドウ関数名
	wc41.style = 0;								  // クラススタイル
	wc41.cbSize = sizeof(WNDCLASSEX);			  // 構造体サイズ
	wc41.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc41.hIconSm = LoadIcon(NULL, IDI_WINLOGO);	  // スモールアイコン
	wc41.hCursor = LoadCursor(NULL, IDC_HAND);	  // マウスポインタ
	wc41.lpszMenuName = NULL;					  // メニュー(なし)
	wc41.cbClsExtra = 0;						  // クラス拡張情報
	wc41.cbWndExtra = 0;						  // ウィンドウ拡張情報
	wc41.hbrBackground = hBrushWhite;			  // ウィンドウの背景色
	if (!RegisterClassEx(&wc41))
		return 0; // ウィンドウクラス登録

	// ウィンドウ生成
	hWnd10 = CreateWindow(
		lpClassName10,			  // ウィンドウクラス名
		lpWindowName,			  // ウィンドウ名
		WS_DLGFRAME | WS_SYSMENU| WS_MINIMIZEBOX, // ウィンドウ属性
		CW_USEDEFAULT,			  // ウィンドウ表示位置(X)
		CW_USEDEFAULT,			  // ウィンドウ表示位置(Y)
		WINDOW_W,				  // ウィンドウサイズ(X)
		WINDOW_H,				  // ウィンドウサイズ(Y)
		HWND_DESKTOP,			  // 親ウィンドウハンドル
		NULL,
		hInstance, // インスタンスハンドル
		NULL);

	hWnd20 = CreateWindow(
		lpClassName20,			  // ウィンドウクラス名
		lpWindowName,			  // ウィンドウ名
		WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX, // ウィンドウ属性
		CW_USEDEFAULT,			  // ウィンドウ表示位置(X)
		CW_USEDEFAULT,			  // ウィンドウ表示位置(Y)
		WINDOW_W,				  // ウィンドウサイズ(X)
		WINDOW_H,				  // ウィンドウサイズ(Y)
		HWND_DESKTOP,			  // 親ウィンドウハンドル
		NULL,
		hInstance, // インスタンスハンドル
		NULL);

	hWnd21 = CreateWindow(
		lpClassName21,
		lpWindowName,
		WS_OVERLAPPEDWINDOW | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500,
		700,
		hWnd20,
		NULL,
		hInstance,
		NULL);

	hWnd30 = CreateWindow(
		lpClassName30,			  // ウィンドウクラス名
		lpWindowName,			  // ウィンドウ名
		WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX, // ウィンドウ属性
		CW_USEDEFAULT,			  // ウィンドウ表示位置(X)
		CW_USEDEFAULT,			  // ウィンドウ表示位置(Y)
		WINDOW_W,				  // ウィンドウサイズ(X)
		WINDOW_H,				  // ウィンドウサイズ(Y)
		HWND_DESKTOP,			  // 親ウィンドウハンドル
		NULL,
		hInstance, // インスタンスハンドル
		NULL);

	hWnd40 = CreateWindow(
		lpClassName40,			  // ウィンドウクラス名
		lpWindowName,			  // ウィンドウ名
		WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX, // ウィンドウ属性
		CW_USEDEFAULT,			  // ウィンドウ表示位置(X)
		CW_USEDEFAULT,			  // ウィンドウ表示位置(Y)
		WINDOW_W,				  // ウィンドウサイズ(X)
		WINDOW_H,				  // ウィンドウサイズ(Y)
		HWND_DESKTOP,			  // 親ウィンドウハンドル
		NULL,
		hInstance, // インスタンスハンドル
		NULL);

	hWnd41 = CreateWindow(
		lpClassName41,			  // ウィンドウクラス名
		lpWindowName,			  // ウィンドウ名
		WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX, // ウィンドウ属性
		CW_USEDEFAULT,			  // ウィンドウ表示位置(X)
		CW_USEDEFAULT,			  // ウィンドウ表示位置(Y)
		WINDOW_W,				  // ウィンドウサイズ(X)
		WINDOW_H,				  // ウィンドウサイズ(Y)
		HWND_DESKTOP,			  // 親ウィンドウハンドル
		NULL,
		hInstance, // インスタンスハンドル
		NULL);

	// ウィンドウ表示
	ShowWindow(hWnd10, nCmdShow); // ウィンドウ表示モード
	UpdateWindow(hWnd10);		  // ウインドウ更新

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0))
	{ // メッセージを取得
		TranslateMessage(&msg);
		DispatchMessage(&msg); // メッセージ送る
	}
	return (int)msg.wParam; // プログラム終了
}

////////////////////////////////////////////////////////////////////////////////
//
//  ウィンドウ関数(イベント処理を記述)
//
LRESULT CALLBACK WindowProc10(HWND hWnd10, UINT uMsg, WPARAM wP, LPARAM lP)
{
	//定義//////////////////////////////////////////////////////////////////////////////

	static int dx = 0;
	static int dy = 0;
	static int time_counter = 0;
	int rad = 9;
	//HRGN box = CreateRectRgn(250, 210, 450, 410);

	//処理////////////////////////////////////////////////////////////////////////////
	switch (uMsg)
	{

	case WM_CREATE: // ウィンドウが生成された

		// エディットボックス　プレイヤー名入力フォーム
		hWndPlayer = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
			WS_CHILD | WS_VISIBLE, 50, 50, 150, 25,
			hWnd10, (HMENU)IDF_PLAYERNAME, NULL, NULL);
		// エディットボックス  ホスト名入力フォーム
		hWndHost = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
			WS_CHILD | WS_VISIBLE, 100, 505, 200, 25,
			hWnd10, (HMENU)IDF_HOSTNAME, NULL, NULL);
		
		// ボタン　対戦する
		hWndConnect = CreateWindow("button", "対戦する",
			WS_CHILD | WS_VISIBLE, 100, 540, 200, 90,
			hWnd10, (HMENU)IDB_CONNECT, NULL, NULL);
		//ボタン　対戦するのをやめる
		hWndConnectCancel = CreateWindow("button", "やめる",
			WS_CHILD | WS_VISIBLE, 225, 640, 75, 40,
		hWnd10, (HMENU)IDB_CONNECTCANCEL, NULL, NULL);

		// ボタン　対戦を待つ
		hWndAccept = CreateWindow("button", "対戦相手を待つ",
			WS_CHILD | WS_VISIBLE, 400, 505, 200, 125,
			hWnd10, (HMENU)IDB_ACCEPT, NULL, NULL);
		// ボタン　待つのをやめる
		hWndAcceptCancel = CreateWindow("button", "やめる",
			WS_CHILD | WS_VISIBLE, 525, 640, 75, 40,
			hWnd10, (HMENU)IDB_ACCEPTCANCEL, NULL, NULL);

		//ボタン　遊び方
		hWndExplanation = CreateWindowEx(
			WS_EX_LEFT, TEXT("BUTTON"), "ルール",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			550, 40, 75, 40,
			hWnd10, (HMENU)IDB_EXPLANATION, NULL, NULL);
		/*
		//ボタン　これまでの記録
		hWndRecord = CreateWindowEx(
			WS_EX_LEFT, TEXT("BUTTON"), TEXT("これまでの記録"),
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			250, 700, 200, 50,
			hWnd10, (HMENU)IDB_RECORD, NULL, NULL);
		*/

		/*label10 = CreateWindow(
			TEXT("STATIC"), NULL,
			WS_CHILD | WS_VISIBLE,
			100, 300, 150, 80, hWnd10, (HMENU)2,
			((LPCREATESTRUCT)(lP))->hInstance, NULL);
		*/

		SetFocus(hWndHost);					   //フォーカス指定
		SetWindowText(hWndPlayer, "Player");
		EnableWindow(hWndConnectCancel, FALSE);
		EnableWindow(hWndAcceptCancel, FALSE); // [接続待ちキャンセル]無効

		SockInit(hWnd10); // ソケット初期化

		return 0L;

	case WM_ACTIVATE:

		SetTimer(hWnd10, 1, 1, NULL);

		return 0L;


		//再描画/////////////////////////////////////////////////////////////////////////////
	case WM_PAINT: // 再描画

		HDC hdc;
		PAINTSTRUCT ps;

		hdc = BeginPaint(hWnd10, &ps);

		// gc資源////////////

		hPenGray = CreatePen(PS_SOLID, 1, 0xABABAF);
		hPenDef_Col_2 = CreatePen(PS_SOLID, 1, Player2_Default_Color);
		hBrush_Def_Col_2 = CreateSolidBrush(Player2_Default_Color);

		// 50pxのフォント
		hfon1 = CreateFont(50, 0,								 //高さ, 幅
			0, 0, FW_BOLD,						 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

	   /////////////////////

		hBrushPrev = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);

		//タイトル
		TextOut(hdc, 185, 125, "ドット＆ボックス", lstrlen("ドット＆ボックス"));

		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

		TextOut(hdc, 50, 25, "Your Name", lstrlen("Your Name"));
		TextOut(hdc, 100, 480, "対戦相手", lstrlen("対戦相手"));

		TextOut(hdc, 225, 725, "2021年度 ソフトウェア実現 14班", lstrlen("2021年度 ソフトウェア実現 14班"));

		RoundRect(hdc, 75, 465, 325, 700, 30, 30);
		RoundRect(hdc, 375, 465, 625, 700, 30, 30);

		if (time_counter >= 50)
		{
			hPenPrev = (HPEN)SelectObject(hdc, hPenGray);
			Ellipse(hdc, 275 - rad, 235 - rad, 275 + rad, 235 + rad);
			Ellipse(hdc, 425 - rad, 235 - rad, 425 + rad, 235 + rad);
			Ellipse(hdc, 275 - rad, 385 - rad, 275 + rad, 385 + rad);
			Ellipse(hdc, 425 - rad, 385 - rad, 425 + rad, 385 + rad);
			SelectObject(hdc, GetStockObject(BLACK_PEN));
		}

		if (time_counter >= 50 + 3 && time_counter < 100 - 3)
		{
			for (int i = -1; i <= 1; i++)
			{
				SetPixel(hdc, dx, dy + i, Player1_Default_Color);
				SetPixel(hdc, dx + 1, dy + i, Player1_Default_Color);
				SetPixel(hdc, dx + 2, dy + i, Player1_Default_Color);
			}
		}

		if (time_counter >= 100 + 3 && time_counter < 150 - 3)
		{
			for (int i = -1; i <= 1; i++)
			{
				SetPixel(hdc, dx + i, dy, Player2_Default_Color);
				SetPixel(hdc, dx + i, dy + 1, Player2_Default_Color);
				SetPixel(hdc, dx + i, dy + 2, Player2_Default_Color);
			}
		}

		if (time_counter >= 150 + 3 && time_counter < 200 - 3)
		{
			for (int i = -1; i <= 1; i++)
			{
				SetPixel(hdc, dx, dy + i, Player1_Default_Color);
				SetPixel(hdc, dx - 1, dy + i, Player1_Default_Color);
				SetPixel(hdc, dx - 2, dy + i, Player1_Default_Color);
			}
		}

		if (time_counter >= 200 + 3 && time_counter < 250 - 3)
		{
			for (int i = -1; i <= 1; i++)
			{
				SetPixel(hdc, dx + i, dy, Player2_Default_Color);
				SetPixel(hdc, dx + i, dy - 1, Player2_Default_Color);
				SetPixel(hdc, dx + i, dy - 2, Player2_Default_Color);
			}
		}

		if (time_counter >= 275)
		{
			SelectObject(hdc, hPenDef_Col_2);
			SelectObject(hdc, hBrush_Def_Col_2);
			RoundRect(hdc, 285, 245, 416, 376, 10, 10);
		}

		SelectObject(hdc, hPenPrev);
		SelectObject(hdc, hBrushPrev);
		SelectObject(hdc, hfonPrev);

		DeleteObject(hPenGray);
		DeleteObject(hPenDef_Col_2);
		DeleteObject(hBrush_Def_Col_2);
		DeleteObject(hfon1);

		EndPaint(hWnd10, &ps);

		return 0L;

	case WM_TIMER:

		time_counter++;

		if (time_counter >= 0 && time_counter < 50)
		{
		}

		if (time_counter >= 50 && time_counter < 100)
		{
			if (time_counter == 50)
			{
				dx = 275;
				dy = 235;
			}
			else
				dx += 3;
		}

		if (time_counter >= 100 && time_counter < 150)
		{
			if (time_counter == 100)
			{
				dx = 425;
				dy = 235;
			}
			else
				dy += 3;
		}

		if (time_counter >= 150 && time_counter < 200)
		{
			if (time_counter == 150)
			{
				dx = 425;
				dy = 385;
			}
			else
				dx -= 3;
		}

		if (time_counter >= 200 && time_counter < 250)
		{
			if (time_counter == 200)
			{
				dx = 275;
				dy = 385;
			}
			else
				dy -= 3;
		}

		if (time_counter >= 275)
			KillTimer(hWnd10, 1);
		//DeleteObject(box);

		InvalidateRgn(hWnd10, NULL, FALSE);

		return 0;

		//ボタン関連//////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND: // ボタンが押された
		switch (LOWORD(wP))
		{
		case IDB_CONNECT: // [対戦する]ボタン押下(クライアント)

			GetWindowText(hWndHost, host, sizeof(host));

			if (SockConnect(hWnd10, host)) //対戦する要求
			{   //対戦要求が失敗した場合			
				SetFocus(hWndHost);
				EnableWindow(hWndConnect, TRUE);

				return 0L;
			}

			EnableWindow(hWndConnect, FALSE);
			EnableWindow(hWndConnectCancel, TRUE);
			EnableWindow(hWndAccept, FALSE);
			EnableWindow(hWndAcceptCancel, FALSE);

			return 0L;

		case IDB_CONNECTCANCEL:

			closesocket(sock);
			sock = INVALID_SOCKET;

			EnableWindow(hWndConnect, TRUE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndAcceptCancel, FALSE);

			return 0L;

		case IDB_ACCEPT: // [対戦待ち]ボタン押下(サーバー)

			if (SockAccept(hWnd10)) // 対戦待ち要求
			{   //対戦待ち要求が失敗した場合   
				EnableWindow(hWndAccept, TRUE);	       // [接続待ち]有効
				EnableWindow(hWndAcceptCancel, FALSE);

				return 0L;
			}

			EnableWindow(hWndConnect, FALSE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, FALSE);
			EnableWindow(hWndAcceptCancel, TRUE);

			return 0L;

		case IDB_ACCEPTCANCEL:

			closesocket(sv_sock);
			sv_sock = INVALID_SOCKET;

			EnableWindow(hWndConnect, TRUE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndAcceptCancel, FALSE);

			return 0L;

		case IDB_EXPLANATION:
			MessageBox(hWnd10, TEXT("ドット＆ボックス ～ルール～\n\n【進行】\nm×nの正方形のマス状に、点と点を結ぶ線（斜めは禁止）をお互いに一本ずつ引いていく。\n【自分の陣地となる条件】\n４辺目を囲えたマスが陣地になる。マスを囲えた場合、もう一度 自分のターンとなる。\n【勝利条件】\n全てのマスが埋まったら、ゲーム終了となる。その時点で、陣地の数が多いプレイヤーを勝ちとする。\n\nみんななかよくプレイしましょう"), TEXT("ルール"), MB_OK | MB_ICONINFORMATION);

			return 0L;

		case IDB_RECORD:

			//SetWindowText(label10, "RECORD");

			//送信処理
			if (send(sock, "RECORD", strlen("RECORD") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd10, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			return 0L;

		} /* end of switch (LOWORD(wP)) */
		return 0L;

		//通信関連 //////////////////////////////////////////////////////////////////
	case WM_SOCKET: // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0)
		{
			return 0L;
		}

		switch (WSAGETSELECTEVENT(lP))
		{

		case FD_ACCEPT: // 接続待ち完了通知, サーバー側が受け取る
		{
			SOCKADDR_IN cl_sin;
			int len = sizeof(cl_sin);
			sock = accept(sv_sock, (LPSOCKADDR)&cl_sin, &len);

			if (sock == INVALID_SOCKET)
			{
				MessageBox(hWnd10, "Accepting connection failed",
					"Error", MB_OK | MB_ICONEXCLAMATION);
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;

				SetFocus(hWndHost);				 // フォーカス指定
				return 0L;
			}

#ifndef NO_DNS
			// ホスト名取得
			phe = gethostbyaddr((char*)&cl_sin.sin_addr, 4, AF_INET);
			if (phe)
			{
				SetWindowText(hWndHost, phe->h_name);
			}
#endif NO_DNS

			// 非同期モード (受信＆切断）
			if (WSAAsyncSelect(sock, hWnd10, WM_SOCKET, FD_CONNECT | FD_ACCEPT) == SOCKET_ERROR)
			{
				// 接続に失敗したら初期状態に戻す
				MessageBox(hWnd10, "WSAAsyncSelect() failed",
					"Error", MB_OK | MB_ICONEXCLAMATION);

				SetFocus(hWndHost);				 // フォーカス指定
				return 0L;
			}

			//idと自分の名前を設定する
			my_id = 1; //サーバー

			char buf_myname1[64];
			GetWindowText(hWndPlayer, buf_myname1, sizeof(buf_myname1));

			if (strlen(buf_myname1) == 0) {
				sprintf_s(Player1_name, "Player1");
			}
			else
				sprintf_s(Player1_name, "%s", buf_myname1);

			//sprintf_s(Player1_name, "%s", buf_myname1);			

			//画面遷移　Screen10→Screen20/////////////////////

			// Screen10を非表示にする
			ShowWindow(hWnd10, SW_HIDE);

			//WM_SOCKETがhWnd20に届くように切り替える
			WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

			// Screen20を表示する
			ShowWindow(hWnd20, SW_NORMAL);
			ShowWindow(hWndPlayStart, SW_NORMAL);
			ShowWindow(hWndReject, SW_NORMAL);
			ShowWindow(hListVer, SW_NORMAL);
			ShowWindow(hListHol, SW_NORMAL);
			ShowWindow(hRadPlayer1, SW_NORMAL);
			ShowWindow(hRadPlayer2, SW_NORMAL);
			ShowWindow(hRadRound1, SW_NORMAL);
			ShowWindow(hRadRound5, SW_NORMAL);
			InvalidateRect(hWnd20, NULL, TRUE); //再描画

			///////////////////////////////////////////////////

			return 0L;

		} /* end of case FD_ACCEPT: */

		case FD_CONNECT: // 接続完了通知, クライアント側が受け取る
			// 非同期モード (受信＆切断)
			if (WSAAsyncSelect(sock, hWnd10, WM_SOCKET, FD_CONNECT | FD_ACCEPT) == SOCKET_ERROR)
			{
				// 接続に失敗したら初期状態に戻す
				MessageBox(hWnd10, "WSAAsyncSelect() failed",
					"Error", MB_OK | MB_ICONEXCLAMATION);
				EnableWindow(hWndPlayer, TRUE);
				EnableWindow(hWndHost, TRUE);	 // [HostName]有効
				EnableWindow(hWndConnect, TRUE); // [接続]    有効
				EnableWindow(hWndAccept, TRUE);	 // [接続待ち]有効
				SetFocus(hWndHost);				 // フォーカス指定
				return 0L;
			}

			//idと自分の名前を設定する
			my_id = 2; //クライアント

			char buf_myname2[64];
			GetWindowText(hWndPlayer, buf_myname2, sizeof(buf_myname2));

			if (strlen(buf_myname2) == 0) {
				sprintf_s(Player2_name, "Player2");
			}
			else
				sprintf_s(Player2_name, "%s", buf_myname2);

			//sprintf_s(Player2_name, "%s", buf_myname2);


			//画面遷移　Screen10→Screen20/////////////////////

			// Screen10を非表示にする
			ShowWindow(hWnd10, SW_HIDE);

			//WM_SOCKETがhWnd20に届くように切り替える
			WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

			// Screen20を表示する
			ShowWindow(hWnd20, SW_NORMAL);
			ShowWindow(hWndPlayStart, SW_NORMAL);
			ShowWindow(hWndReject, SW_NORMAL);
			ShowWindow(hListVer, SW_NORMAL);
			ShowWindow(hListHol, SW_NORMAL);
			ShowWindow(hRadPlayer1, SW_NORMAL);
			ShowWindow(hRadPlayer2, SW_NORMAL);
			ShowWindow(hRadRound1, SW_NORMAL);
			ShowWindow(hRadRound5, SW_NORMAL);
			InvalidateRect(hWnd20, NULL, TRUE); //再描画

			///////////////////////////////////////////////////

			return 0L;

		} /* end of switch (WSAGETSELECTEVENT(lP)) */
		return 0L;

		//デフォルトな処理/////////////////////////////////////////////////////////
	case WM_SETFOCUS: // ウィンドウにフォーカスが来たら
		SetFocus(hWndHost);
		return 0L;

	case WM_DESTROY: // ウィンドウが破棄された
		closesocket(sock);
		PostQuitMessage(0);
		return 0L;
	default:
		return DefWindowProc(hWnd10, uMsg, wP, lP); // 標準メッセージ処理
	}												/* end of switch (uMsg) */
	return 0L;
}

////////////////////////////////////////////////////////////////////////////////
//
//  ウィンドウ関数(Screen20 ゲーム選択画面)
//
LRESULT CALLBACK WindowProc20(HWND hWnd20, UINT uMsg, WPARAM wP, LPARAM lP)
{
	//定義//////////////////////////////////////////////////////////////////////////////

	HRGN rgn1, rgn2;

	//処理////////////////////////////////////////////////////////////////////////////

	switch (uMsg)
	{

	case WM_CREATE: // ウィンドウが生成された

		//ボタン
		hWndReject = CreateWindow("button", "EXIT",
			WS_CHILD | WS_VISIBLE, 50, 30, 80, 40,
			hWnd20, (HMENU)IDB_REJECT, NULL, NULL);

		hWndPlayStart = CreateWindow("button", "ゲームを始める",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 250, 675, 200, 50,
			hWnd20, (HMENU)IDB_PLAYSTART, NULL, NULL);
		/*
		hWndReference = CreateWindow("button", "情報共有",
			WS_CHILD | WS_VISIBLE, 560, 185, 80, 30,
			hWnd20, (HMENU)IDB_REFERENCE, NULL, NULL);
		*/

		//リストボックス
		hListVer = CreateWindow(
			TEXT("LISTBOX"), NULL,
			WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_VSCROLL,
			100, 335, 50, 75, hWnd20, (HMENU)IDL_VER,
			((LPCREATESTRUCT)(lP))->hInstance, NULL);

		for (int i = 0; i < 7; i++)
			SendMessage(hListVer, LB_ADDSTRING, 0, (LPARAM)strText[i]);

		hListHol = CreateWindow(
			TEXT("LISTBOX"), NULL,
			WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_VSCROLL,
			240, 335, 50, 75, hWnd20, (HMENU)IDL_HOL,
			((LPCREATESTRUCT)(lP))->hInstance, NULL);

		for (int i = 0; i < 7; i++)
			SendMessage(hListHol, LB_ADDSTRING, 0, (LPARAM)strText[i]);

		//ラジオボタン
		hRadPlayer1 =
			CreateWindow(
				TEXT("BUTTON"), TEXT("Player1"),
				WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
				125, 515, 125, 50,
				hWnd20, (HMENU)IDR_Player1, ((LPCREATESTRUCT)(lP))->hInstance, NULL);

		hRadPlayer2 =
			CreateWindow(
				TEXT("BUTTON"), TEXT("Player2"),
				WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
				125, 575, 125, 50,
				hWnd20, (HMENU)IDR_Player2, ((LPCREATESTRUCT)(lP))->hInstance, NULL);

		hRadRound1 =
			CreateWindow(
				TEXT("BUTTON"), TEXT("ROUND1"),
				WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
				450, 515, 125, 30,
				hWnd20, (HMENU)IDR_Round1, ((LPCREATESTRUCT)(lP))->hInstance, NULL);

		hRadRound3 =
			CreateWindow(
				TEXT("BUTTON"), TEXT("ROUND3"),
				WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
				450, 555, 125, 30,
				hWnd20, (HMENU)IDR_Round3, ((LPCREATESTRUCT)(lP))->hInstance, NULL);

		hRadRound5 =
			CreateWindow(
				TEXT("BUTTON"), TEXT("ROUND5"),
				WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
				450, 595, 125, 30,
				hWnd20, (HMENU)IDR_Round5, ((LPCREATESTRUCT)(lP))->hInstance, NULL);

		/*
		label = CreateWindow(   //送られてきたメッセージを確認するためのSTATICウインドウ
			TEXT("STATIC"), NULL,
			WS_CHILD | WS_VISIBLE,
			50, 650, 150, 80, hWnd20, (HMENU)2,
			((LPCREATESTRUCT)(lP))->hInstance, NULL);
		*/

		SendMessage(hRadPlayer1, BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(hRadRound1, BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(hListVer, LB_SETCURSEL, 1, 0);
		SendMessage(hListHol, LB_SETCURSEL, 1, 0);

		return 0L;

		//再描画/////////////////////////////////////////////////////////////////////////////
	case WM_PAINT: // 再描画

		HDC hdc;
		PAINTSTRUCT ps;

		hdc = BeginPaint(hWnd20, &ps);

		// gc資源（ペン、ブラシ、領域）////////////

		hPenBk2px = (HPEN)CreatePen(PS_SOLID, 2, RGB(0, 0, 0));

		// 25px, 太字
		hfon1 = CreateFont(25, 0,								 //高さ, 幅
			0, 0, FW_BOLD,						 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

		// 25px, 普通の太さ
		hfon2 = CreateFont(25, 0,								 //高さ, 幅
			0, 0, 0,								 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

		rgn1 = CreateRoundRectRgn(535, 300, 585, 350, 15, 15);
		rgn2 = CreateRoundRectRgn(535, 360, 585, 410, 15, 15);

		////////////////////////////////////////////

		hfonPrev = (HFONT)SelectObject(hdc, hfon1); // フォントを選択
		TextOut(hdc, 340, 40, "対戦相手の情報", lstrlen("対戦相手の情報"));


		//対戦相手の情報//////////////////////////////////////////////////////////////////////
		if (my_id == 1)
		{																 //サーバー
			TextOut(hdc, 75, 150, Player1_name, lstrlen(Player1_name));	 //自分の名前

			if (strlen(Player2_name) == 0) {
				TextOut(hdc, 355 - lstrlen("Player2") / 2 * 25, 150, "Player2", lstrlen("Player2"));
			}
			else {
				TextOut(hdc, 355 - lstrlen(Player2_name) / 2 * 25, 150, Player2_name, lstrlen(Player2_name)); //相手の名前
			}

			TextOut(hdc, 570 - lstrlen(phe->h_name) / 2 * 25, 150, phe->h_name, lstrlen(phe->h_name));					 //相手のIP or "localhost"
		}
		else if (my_id == 2)
		{																 //クライアント
			TextOut(hdc, 75, 150, Player2_name, lstrlen(Player2_name));	 //自分の名前

			if (strlen(Player1_name) == 0) {
				TextOut(hdc, 355 - lstrlen("Player1") / 2 * 25, 150, "Player1", lstrlen("Player1"));
			}
			else {
				TextOut(hdc, 355 - lstrlen(Player1_name) / 2 * 25, 150, Player1_name, lstrlen(Player1_name)); //相手の名前
			}

			TextOut(hdc, 570 - lstrlen(host) / 2 * 25, 150, host, lstrlen(host));				 //相手のIP or "localhost"

		}


		//太文字、見出し
		TextOut(hdc, 135, 250, "盤面の形", lstrlen("盤面の形"));
		TextOut(hdc, 450, 250, "色の選択", lstrlen("色の選択"));
		TextOut(hdc, 120, 475, "最初の手番", lstrlen("最初の手番"));
		TextOut(hdc, 450, 475, "対戦形式", lstrlen("対戦形式"));

		SelectObject(hdc, hfon2);								  // フォントを選択
		TextOut(hdc, 75, 100, "Your Name", lstrlen("Your Name")); //自分の名前
		TextOut(hdc, 290, 100, "Name", lstrlen("Name"));		  //相手の名前
		TextOut(hdc, 510, 100, "IP", lstrlen("IP"));			  //相手のIP
		TextOut(hdc, 145, 290, "四角形", lstrlen("四角形"));
		TextOut(hdc, 65, 340, "縦", lstrlen("縦"));
		TextOut(hdc, 205, 340, "横", lstrlen("横"));
		TextOut(hdc, 170, 370, TEXT("×"), lstrlen("×"));


		//色の選択のPlayer名//////////////////////////////////////////////////////////////
		if (my_id == 1)
		{
			TextOut(hdc, 420, 310, Player1_name, lstrlen(Player1_name));

			if (strlen(Player2_name) == 0) {
				TextOut(hdc, 420, 370, "Player2", lstrlen("Player2"));
			}
			else {
				TextOut(hdc, 420, 370, Player2_name, lstrlen(Player2_name)); //相手の名前
			}

		}
		else if (my_id == 2)
		{
			if (strlen(Player1_name) == 0) {
				TextOut(hdc, 420, 310, "Player1", lstrlen("Player1"));
			}
			else {
				TextOut(hdc, 420, 310, Player1_name, lstrlen(Player1_name)); //相手の名前
			}

			TextOut(hdc, 420, 370, Player2_name, lstrlen(Player2_name));

		}


		//最初の手番の部分の名前を表示する////////////////////////////////////////////////////
		if (my_id == 1)
		{										          //サーバー
			SetWindowText(hRadPlayer1, Player1_name);	 //自分の名前

			if (strlen(Player2_name) == 0) {
				SetWindowText(hRadPlayer2, "Player2");
			}
			else {
				SetWindowText(hRadPlayer2, Player2_name); //相手の名前
			}
		}
		else if (my_id == 2)
		{					                             //クライアント		
			if (strlen(Player1_name) == 0) {
				SetWindowText(hRadPlayer1, "Player1");
			}
			else {
				SetWindowText(hRadPlayer1, Player1_name); //相手の名前
			}

			SetWindowText(hRadPlayer2, Player2_name);	 //自分の名前
		}



		SelectObject(hdc, GetStockObject(SYSTEM_FONT));

		//開始フラグの表示
		if (PlayStart1 == 1 && PlayStart2 == 1)
		{
			TextOut(hdc, 465, 700, TEXT("(clicked PlayStart Button(2))"), strlen("(clicked PlayStart Button(2))"));
		}
		else if (PlayStart1 == 1 || PlayStart2 == 1) {
			TextOut(hdc, 465, 700, TEXT("(clicked PlayStart Button(1))"), strlen("(clicked PlayStart Button(1))"));
		}

		hBrushPrev = (HBRUSH)SelectObject(hdc, CreateSolidBrush(Player1_Color));
		PaintRgn(hdc, rgn1);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(Player2_Color)));
		PaintRgn(hdc, rgn2);
		DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

		hPenPrev = (HPEN)SelectObject(hdc, hPenBk2px);
		MoveToEx(hdc, 50, 100, NULL);
		LineTo(hdc, 50, 175);

		SelectObject(hdc, GetStockObject(BLACK_PEN));
		RoundRect(hdc, 230, 25, 650, 75, 30, 30);	//対戦相手の情報の枠
		RoundRect(hdc, 50, 225, 335, 425, 30, 30);	//盤面の形
		RoundRect(hdc, 365, 225, 650, 425, 30, 30); //色の選択の枠
		RoundRect(hdc, 50, 450, 335, 650, 30, 30);	//最初の手番の枠
		RoundRect(hdc, 365, 450, 650, 650, 30, 30); //対戦形式の枠

		//元のペン・ブラシ・フォントに戻す
		SelectObject(hdc, hPenPrev);
		SelectObject(hdc, hBrushPrev);
		SelectObject(hdc, hfonPrev);

		// gc資源をDeleteする
		DeleteObject(hPenBk2px);
		DeleteObject(hfon1);
		DeleteObject(hfon2);
		DeleteObject(rgn1);
		DeleteObject(rgn2);

		EndPaint(hWnd20, &ps);

		return 0L;

	case WM_DRAWITEM:
	{
		return OnDrawItem(hWnd20, uMsg, wP, lP);
	}

	case WM_INVALIDATE:

		InvalidateRect(hWnd20, NULL, TRUE);

		char buf_col[64];
		sprintf_s(buf_col, "COLOR%8x%8x", Player1_Color, Player2_Color);

		if (send(sock, buf_col, strlen(buf_col) + 1, 0) == SOCKET_ERROR)
		{
			// 送信に失敗したらエラーを表示
			MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
				MB_OK | MB_ICONEXCLAMATION);
		}

		return 0L;


	case WM_SETUP:




		return 0L;

		//ボタン関連//////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND: // コントロールが押された
		switch (LOWORD(wP))
		{
		case IDB_REJECT: // [切断]ボタン押下

			if (sock != INVALID_SOCKET)
			{ // 自分がクライアント側なら
				// ソケットを閉じる
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			if (sv_sock != INVALID_SOCKET)
			{ // 自分がサーバ側なら
				// サーバ用ソケットを閉じる
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
			}

			phe = NULL;


			//画面遷移 Screen20→Screen10////////////////////////////


			//初期化する
			point_1 = 0;
			point_2 = 0;


			for (int i = 0; i < 5; i++) {  // 対戦内容初期化
				Points_1[i] = 0;
				Points_2[i] = 0;
			}
			for (int i = 0; i <= board_h_num; i++) {
				for (int j = 0; j <= board_w_num + 1; j++) {
					vir_line[i][j] = 0;
				}
			}
			for (int i = 0; i <= board_h_num + 1; i++) {
				for (int j = 0; j <= board_w_num; j++) {
					hor_line[i][j] = 0;
				}
			}
			for (int i = 0; i <= board_h_num; i++) {
				for (int j = 0; j <= board_w_num; j++) {
					square[i][j] = 0;
				}
			}

			board_w_num = 3;
			board_h_num = 3;
			Player1_Color = Player1_Default_Color;
			Player2_Color = Player2_Default_Color;
			phase = 1;
			round_mode = 1;
			round_num = 1;

			EnableWindow(hWndConnect, TRUE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndAcceptCancel, FALSE);

			// Screen20を非表示にする
			ShowWindow(hWndReject, SW_HIDE);
			ShowWindow(hWnd20, SW_HIDE);

			//SocketメッセージをhWnd10に送るようにする
			WSAAsyncSelect(sock, hWnd10, WM_SOCKET, FD_ACCEPT | FD_CONNECT);

			// Screen10を表示する
			ShowWindow(hWndPlayer, SW_NORMAL);
			ShowWindow(hWndHost, SW_NORMAL);
			ShowWindow(hWndConnect, SW_NORMAL);
			ShowWindow(hWndAccept, SW_NORMAL);
			ShowWindow(hWndExplanation, SW_NORMAL);
			ShowWindow(hWndRecord, SW_NORMAL);
			ShowWindow(hWnd10, SW_NORMAL);
			InvalidateRect(hWnd10, NULL, FALSE); //再描画

			/////////////////////////////////////////////////////////

			return 0L;

		case IDB_REFERENCE:

			char buf_name_ref[64];

			if (my_id == 1) {
				sprintf_s(buf_name_ref, "NAME%s", Player1_name);
			}
			else if (my_id == 2) {
				sprintf_s(buf_name_ref, "NAME%s", Player2_name);
			}

			if (send(sock, buf_name_ref, strlen(buf_name_ref) + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			return 0L;

		case IDL_VER:

			int ver;

			ver = SendMessage(hListVer, LB_GETCURSEL, 0, 0);

			char buf_ver[64];
			sprintf_s(buf_ver, "IDL_VER%1d", ver);
			// wsprintf(strText1, TEXT("IDL_VER%1d"), ver);

			if (send(sock, buf_ver, strlen(buf_ver) + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			SetWindowText(label, buf_ver);

			board_h_num = ver + 2;
			board_dot_h_num = board_h_num + 1;
			char str[9];
			sprintf_s(str, 8, "w%d", board_h_num);
			SetWindowText(label, str);

			FILE* f;
			fopen_s(&f, "output.txt", "a");
			fprintf(f, "送信、縦%d\n", board_h_num);
			fclose(f);

			return 0L;

		case IDL_HOL:
			int hol;

			hol = SendMessage(hListHol, LB_GETCURSEL, 0, 0);

			char buf_hol[64];
			sprintf_s(buf_hol, "IDL_HOL%1d", hol);

			if (send(sock, buf_hol, strlen(buf_hol) + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			SetWindowText(label, buf_hol);

			board_w_num = hol + 2;
			board_dot_w_num = board_w_num + 1;
			char str1[9];
			sprintf_s(str1, 8, "h%d", board_w_num);
			SetWindowText(label, str1);


			FILE* fp;
			fopen_s(&fp, "output.txt", "a");
			fprintf(fp, "送信 横%d\n", board_w_num);
			fclose(fp);

			return 0L;

		case IDR_Player1:
			if (SendMessage(hRadPlayer1, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				phase = 1;

				if (send(sock, "IDR_Player1", strlen("IDR_Player1") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}

			}
			return 0L;

		case IDR_Player2:
			if (SendMessage(hRadPlayer2, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				phase = 2;

				if (send(sock, "IDR_Player2", strlen("IDR_Player2") + 1, 0) == SOCKET_ERROR)
				{

					// 送信に失敗したらエラーを表示
					MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0L;

		case IDR_Round1:
			if (SendMessage(hRadRound1, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				round_mode = 1;

				if (send(sock, "IDR_Round1", strlen("IDR_Round1") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0L;

		case IDR_Round3:
			if (SendMessage(hRadRound3, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				round_mode = 3;

				if (send(sock, "IDR_Round3", strlen("IDR_Round3") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0L;

		case IDR_Round5:
			if (SendMessage(hRadRound5, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				round_mode = 5;

				if (send(sock, "IDR_Round5", strlen("IDR_Round5") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0L;

		case IDB_PLAYSTART:

			char buf_playstart[MAX_MESSAGE];
			PlayStart1 = 1;

			InvalidateRect(hWnd20, NULL, TRUE);

			sprintf_s(buf_playstart, sizeof(buf_playstart), "PLAYSTART");
			if (send(sock, buf_playstart, strlen(buf_playstart) + 1, 0) == SOCKET_ERROR) {    // 送信処理
			 // 送信に失敗したらエラーを表示
				MessageBox(hWnd41, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}
			else if (PlayStart2 == 1) {      // 相手も押していたら

				//MessageBox(hWnd20, "試合を開始します", "通知", MB_OK | MB_ICONINFORMATION);


				//画面遷移　Screen20→Screen30//////////////////

				PlayStart1 = 0;  PlayStart2 = 0;

				// Screen20を非表示にする
				ShowWindow(hWndReject, SW_HIDE);
				ShowWindow(hWndPlayStart, SW_HIDE);
				ShowWindow(hWnd20, SW_HIDE);

				//ゲーム画面を構成するメッセージを送る
				SendMessage(hWnd30, WM_SETUP, 0, 0);

				// WM_SOCKETがScreen30に送られるように切り替える
				WSAAsyncSelect(sock, hWnd30, WM_SOCKET, FD_READ | FD_CLOSE);

				// Screen30を表示する
				ShowWindow(hWnd30, SW_NORMAL);
				InvalidateRect(hWnd30, NULL, TRUE); //再描画

				/////////////////////////////////////////////////

				return 0L;
			}

			return 0L;

		} /* end of switch (LOWORD(wP)) */
		return 0L;

		//マウス操作関連/////////////////////////////////////////////////////////////////////
	case WM_LBUTTONDOWN: // マウス左ボタンが押された

		rgn1 = CreateRoundRectRgn(535, 300, 585, 350, 15, 15);
		rgn2 = CreateRoundRectRgn(535, 360, 585, 410, 15, 15);

		if (PtInRegion(rgn1, LOWORD(lP), HIWORD(lP)) | PtInRegion(rgn2, LOWORD(lP), HIWORD(lP)))
		{
			ShowWindow(hWnd21, SW_NORMAL);
		}

		/*
		char buf[MAX_MESSAGE];
		setData(0, LOWORD(lP), HIWORD(lP));       // 線の始点として座標を記録
		sprintf_s(buf, sizeof(buf), "%1d%03d%03d", 0, LOWORD(lP), HIWORD(lP));

		send(sock, buf, strlen(buf) + 1, 0);
		*/

		DeleteObject(rgn1);
		DeleteObject(rgn2);

		return 0L;

		//通信関連 //////////////////////////////////////////////////////////////////
	case WM_SOCKET: // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0)
		{
			return 0L;
		}

		switch (WSAGETSELECTEVENT(lP))
		{

		case FD_READ: //メッセージ受信

			char buf[MAX_MESSAGE]; // 受信内容を一時的に格納する

			if (recv(sock, buf, sizeof(buf) - 1, 0) != SOCKET_ERROR)
			{ // 受信できたなら

				//SetWindowText(label, buf);

				if (strcmp(buf, "PLAYSTART") == 0)
				{

					PlayStart2 = 1;

					InvalidateRect(hWnd20, NULL, TRUE);

					if (PlayStart1 == 1) {     // 自分も押していたら

						//MessageBox(hWnd20, "試合を開始します", "通知", MB_OK | MB_ICONINFORMATION);

						//画面遷移　Screen20→Screen30//////////////////

						PlayStart1 = 0;  PlayStart2 = 0;

						// Screen20を非表示にする
						ShowWindow(hWndReject, SW_HIDE);
						ShowWindow(hWndPlayStart, SW_HIDE);
						ShowWindow(hWnd20, SW_HIDE);

						//ゲーム画面を構成するメッセージを送る
						SendMessage(hWnd30, WM_SETUP, 0, 0);

						// WM_SOCKETがScreen30に送られるように切り替える
						WSAAsyncSelect(sock, hWnd30, WM_SOCKET, FD_READ | FD_CLOSE);

						// Screen30を表示する
						ShowWindow(hWnd30, SW_NORMAL);
						InvalidateRect(hWnd30, NULL, TRUE); //再描画

						/////////////////////////////////////////////////

						return 0L;
					}

				}

				if (strcmp(buf, "IDR_Player1") == 0)
				{

					SendMessage(hRadPlayer1, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(hRadPlayer2, BM_SETCHECK, BST_UNCHECKED, 0);

					phase = 1;

					return 0L;
				}

				if (strcmp(buf, "IDR_Player2") == 0)
				{

					SendMessage(hRadPlayer1, BM_SETCHECK, BST_UNCHECKED, 0);
					SendMessage(hRadPlayer2, BM_SETCHECK, BST_CHECKED, 0);

					phase = 2;

					return 0L;
				}
				if (strcmp(buf, "IDR_Round1") == 0)
				{

					SendMessage(hRadRound1, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(hRadRound3, BM_SETCHECK, BST_UNCHECKED, 0);
					SendMessage(hRadRound5, BM_SETCHECK, BST_UNCHECKED, 0);

					round_mode = 1;

					return 0L;
				}

				if (strcmp(buf, "IDR_Round3") == 0)
				{

					SendMessage(hRadRound1, BM_SETCHECK, BST_UNCHECKED, 0);
					SendMessage(hRadRound3, BM_SETCHECK, BST_CHECKED, 0);
					SendMessage(hRadRound5, BM_SETCHECK, BST_UNCHECKED, 0);

					round_mode = 3;

					return 0L;
				}

				if (strcmp(buf, "IDR_Round5") == 0)
				{

					SendMessage(hRadRound1, BM_SETCHECK, BST_UNCHECKED, 0);
					SendMessage(hRadRound3, BM_SETCHECK, BST_UNCHECKED, 0);
					SendMessage(hRadRound5, BM_SETCHECK, BST_CHECKED, 0);

					round_mode = 5;

					return 0L;
				}

				char* search1;
				search1 = strstr(buf, "IDL_VER");

				if (search1 != NULL)
				{

					int ver = 0;
					sscanf_s(search1, "IDL_VER%1d", &ver);
					SendMessage(hListVer, LB_SETCURSEL, ver, 0);

					board_h_num = ver + 2;
					board_dot_h_num = board_h_num + 1;
					return 0L;
				}

				char* search2;
				search2 = strstr(buf, "IDL_HOL");

				if (search2 != NULL)
				{

					int hol = 0;
					sscanf_s(search2, "IDL_HOL%1d", &hol);
					SendMessage(hListHol, LB_SETCURSEL, hol, 0);

					board_w_num = hol + 2;
					board_dot_w_num = board_w_num + 1;
					return 0L;
				}

				char* search3;
				search3 = strstr(buf, "COLOR");

				if (search3 != NULL)
				{

					sscanf_s(search3, "COLOR%8x%8x", &Player1_Color, &Player2_Color);

					InvalidateRect(hWnd20, NULL, TRUE);

					return 0L;
				}

				char* search4;
				search4 = strstr(buf, "NAME");

				if (search4 != NULL)
				{
					SetWindowText(label, search4);

					if (my_id == 1) {
						sscanf_s(search4, "NAME%s", Player2_name);
						/*FILE* fp3;
						fopen_s(&fp3, "output.txt", "a");
						fprintf(fp3, "search4:%s", search4);
						fprintf(fp3, "player2_name: %s", Player2_name);
						fclose(fp3);*/
					}
					else if (my_id == 2) {
						sscanf_s(search4, "NAME%s", Player1_name);
						/*FILE* fp4;
						fopen_s(&fp4, "output.txt", "a");
						fprintf(fp4, "search4:%s", search4);
						fprintf(fp4, "player1_name: %s", Player1_name);
						fclose(fp4);*/

					}

					InvalidateRect(hWnd20, NULL, TRUE);

					return 0L;
				}
			}

			return 0L;

		} /* end of switch (WSAGETSELECTEVENT(lP)) */
		return 0L;

		//デフォルトな処理/////////////////////////////////////////////////////////
	case WM_DESTROY: // ウィンドウが破棄された
		closesocket(sock);
		PostQuitMessage(0);
		return 0L;
	default:

		return DefWindowProc(hWnd20, uMsg, wP, lP); // 標準メッセージ処理
	}												/* end of switch (uMsg) */
	return 0L;
}

LRESULT CALLBACK WindowProc21(HWND hwnd21, UINT msg, WPARAM wP, LPARAM lP)
{

	HDC hdc;

	int x, y;
	HRGN rgn1, rgn2, rgn3, rgn4, rgn5, rgn6, rgn7;
	HRGN rgn8, rgn9, rgn10, rgn11, rgn12, rgn13, rgn14;
	HRGN rgn15, rgn16, rgn17, rgn18, rgn19, rgn20, rgn21, rgn22;

	PAINTSTRUCT ps;

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:

		hWndSelectColor = CreateWindow("button", "この色にする",
			WS_CHILD | WS_VISIBLE, 175, 600, 200, 50,
			hWnd21, (HMENU)IDB_COLOR, NULL, NULL);

		return 0;

	case WM_PAINT:

		hWndSelectColor = CreateWindow("button", "この色にする",
			WS_CHILD | WS_VISIBLE, 175, 575, 150, 50,
			hWnd21, (HMENU)IDB_COLOR, NULL, NULL);

		// Playerの色の部分の枠
		rgn1 = CreateRoundRectRgn(175, 120, 225, 170, 15, 15);
		rgn2 = CreateRoundRectRgn(275, 120, 325, 170, 15, 15);

		//左側部分
		rgn3 = CreateRoundRectRgn(50, 200, 100, 250, 15, 15);
		rgn4 = CreateRoundRectRgn(150, 200, 200, 250, 15, 15);
		rgn5 = CreateRoundRectRgn(50, 275, 100, 325, 15, 15);
		rgn6 = CreateRoundRectRgn(150, 275, 200, 325, 15, 15);
		rgn7 = CreateRoundRectRgn(50, 350, 100, 400, 15, 15);
		rgn8 = CreateRoundRectRgn(150, 350, 200, 400, 15, 15);
		rgn9 = CreateRoundRectRgn(50, 425, 100, 475, 15, 15);
		rgn10 = CreateRoundRectRgn(150, 425, 200, 475, 15, 15);
		rgn11 = CreateRoundRectRgn(50, 500, 100, 550, 15, 15);
		rgn12 = CreateRoundRectRgn(150, 500, 200, 550, 15, 15);

		//右側部分
		rgn13 = CreateRoundRectRgn(300, 200, 350, 250, 15, 15);
		rgn14 = CreateRoundRectRgn(400, 200, 450, 250, 15, 15);
		rgn15 = CreateRoundRectRgn(300, 275, 350, 325, 15, 15);
		rgn16 = CreateRoundRectRgn(400, 275, 450, 325, 15, 15);
		rgn17 = CreateRoundRectRgn(300, 350, 350, 400, 15, 15);
		rgn18 = CreateRoundRectRgn(400, 350, 450, 400, 15, 15);
		rgn19 = CreateRoundRectRgn(300, 425, 350, 475, 15, 15);
		rgn20 = CreateRoundRectRgn(400, 425, 450, 475, 15, 15);
		rgn21 = CreateRoundRectRgn(300, 500, 350, 550, 15, 15);
		rgn22 = CreateRoundRectRgn(400, 500, 450, 550, 15, 15);

		hdc = BeginPaint(hwnd21, &ps);
		RoundRect(hdc, 140, 70, 360, 185, 25, 25);

		hfon1 = CreateFont(20, 10,								 //高さ, 幅
			0, 0, 0,								 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

		hfonPrev = (HFONT)SelectObject(hdc, hfon1); // フォントを選択

		TextOut(hdc, 160, 90, Player1_name, lstrlen(Player1_name));
		TextOut(hdc, 260, 90, Player2_name, lstrlen(Player2_name));

		hfon1 = CreateFont(35, 10,								 //高さ, 幅
			0, 0, FW_HEAVY,						 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

		hfonPrev = (HFONT)SelectObject(hdc, hfon1); // フォントを選択

		TextOut(hdc, 50, 25, TEXT("色の選択"), lstrlen(TEXT("色の選択")));

		//×の描画
		hfon1 = CreateFont(30, 0,								 //高さ, 幅
			0, 0, 0,								 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

		hfonPrev = (HFONT)SelectObject(hdc, hfon1); // フォントを選択
		TextOut(hdc, 235, 130, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 110, 210, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 110, 285, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 110, 360, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 110, 435, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 110, 510, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 360, 210, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 360, 285, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 360, 360, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 360, 435, TEXT("×"), lstrlen("×"));
		TextOut(hdc, 360, 510, TEXT("×"), lstrlen("×"));

		//選択された色
		SelectObject(hdc, CreateSolidBrush(Player1_Color));
		PaintRgn(hdc, rgn1);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(Player2_Color)));
		PaintRgn(hdc, rgn2);

		//色選択の欄
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF15F2)));
		PaintRgn(hdc, rgn3);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x26FFF8)));
		PaintRgn(hdc, rgn4);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF2244)));
		PaintRgn(hdc, rgn5);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xB221FF)));
		PaintRgn(hdc, rgn6);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xB5FF23)));
		PaintRgn(hdc, rgn7);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF1BAA)));
		PaintRgn(hdc, rgn8);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFFFA1B)));
		PaintRgn(hdc, rgn9);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x911FFF)));
		PaintRgn(hdc, rgn10);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x30B5FF)));
		PaintRgn(hdc, rgn11);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF1B6E)));
		PaintRgn(hdc, rgn12);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x2CAFFF)));
		PaintRgn(hdc, rgn13);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xA9FF17)));
		PaintRgn(hdc, rgn14);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x19FF95)));
		PaintRgn(hdc, rgn15);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF2D37)));
		PaintRgn(hdc, rgn16);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x21F0FF)));
		PaintRgn(hdc, rgn17);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFF1A76)));
		PaintRgn(hdc, rgn18);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xFFE71F)));
		PaintRgn(hdc, rgn19);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x32FEFF)));
		PaintRgn(hdc, rgn20);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0x17FF23)));
		PaintRgn(hdc, rgn21);
		DeleteObject(SelectObject(hdc, CreateSolidBrush(0xB221FF)));
		PaintRgn(hdc, rgn22);
		DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

		DeleteObject(rgn1);
		DeleteObject(rgn2);
		DeleteObject(rgn3);
		DeleteObject(rgn4);
		DeleteObject(rgn5);
		DeleteObject(rgn6);
		DeleteObject(rgn7);
		DeleteObject(rgn8);
		DeleteObject(rgn9);
		DeleteObject(rgn10);
		DeleteObject(rgn11);
		DeleteObject(rgn12);
		DeleteObject(rgn13);
		DeleteObject(rgn14);
		DeleteObject(rgn15);
		DeleteObject(rgn16);
		DeleteObject(rgn17);
		DeleteObject(rgn18);
		DeleteObject(rgn19);
		DeleteObject(rgn20);
		DeleteObject(rgn21);
		DeleteObject(rgn22);

		//ボタン関連//////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND: // ボタンが押された
		switch (LOWORD(wP))
		{

		case IDB_COLOR: // [この色にする]ボタン押下

			PostMessage(hWnd20, WM_INVALIDATE, 0, 0);

			ShowWindow(hWndSelectColor, SW_HIDE);
			ShowWindow(hWnd21, SW_HIDE);
			return 0L;
		}

	case WM_LBUTTONDOWN:

		x = LOWORD(lP);
		y = HIWORD(lP);

		rgn1 = CreateRoundRectRgn(175, 120, 225, 170, 15, 15);
		rgn2 = CreateRoundRectRgn(275, 120, 325, 170, 15, 15);

		rgn3 = CreateRoundRectRgn(50, 200, 100, 250, 15, 15);
		rgn4 = CreateRoundRectRgn(150, 200, 200, 250, 15, 15);
		rgn5 = CreateRoundRectRgn(50, 275, 100, 325, 15, 15);
		rgn6 = CreateRoundRectRgn(150, 275, 200, 325, 15, 15);
		rgn7 = CreateRoundRectRgn(50, 350, 100, 400, 15, 15);
		rgn8 = CreateRoundRectRgn(150, 350, 200, 400, 15, 15);
		rgn9 = CreateRoundRectRgn(50, 425, 100, 475, 15, 15);
		rgn10 = CreateRoundRectRgn(150, 425, 200, 475, 15, 15);
		rgn11 = CreateRoundRectRgn(50, 500, 100, 550, 15, 15);
		rgn12 = CreateRoundRectRgn(150, 500, 200, 550, 15, 15);

		rgn13 = CreateRoundRectRgn(300, 200, 350, 250, 15, 15);
		rgn14 = CreateRoundRectRgn(400, 200, 450, 250, 15, 15);
		rgn15 = CreateRoundRectRgn(300, 275, 350, 325, 15, 15);
		rgn16 = CreateRoundRectRgn(400, 275, 450, 325, 15, 15);
		rgn17 = CreateRoundRectRgn(300, 350, 350, 400, 15, 15);
		rgn18 = CreateRoundRectRgn(400, 350, 450, 400, 15, 15);
		rgn19 = CreateRoundRectRgn(300, 425, 350, 475, 15, 15);
		rgn20 = CreateRoundRectRgn(400, 425, 450, 475, 15, 15);
		rgn21 = CreateRoundRectRgn(300, 500, 350, 550, 15, 15);
		rgn22 = CreateRoundRectRgn(400, 500, 450, 550, 15, 15);

		if (PtInRegion(rgn3, x, y))
		{

			Player1_Color = 0xFF15F2; // color3
			Player2_Color = 0x26FFF8; // color4

			//送信処理
			if (send(sock, "0xFF15F2", strlen("0xFF15F2") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn4, x, y))
		{

			Player1_Color = 0x26FFF8; // color4
			Player2_Color = 0xFF15F2; // color3

			//送信処理
			if (send(sock, "0x26FFF8", strlen("0x26FFF8") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn5, x, y))
		{

			Player1_Color = 0xFF2244; // color5
			Player2_Color = 0xB221FF; // color6

			//送信処理
			if (send(sock, "0xFF2244", strlen("0xFF2244") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn6, x, y))
		{

			Player1_Color = 0xB221FF; // color6
			Player2_Color = 0xFF2244; // color5

			//送信処理
			if (send(sock, "0xB221FF", strlen("0xB221FF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn7, x, y))
		{

			Player1_Color = 0xB5FF23; // color7
			Player2_Color = 0xFF1BAA; // color8

			//送信処理
			if (send(sock, "0xB5FF23", strlen("0xB5FF23") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn8, x, y))
		{

			Player1_Color = 0xFF1BAA; // color8
			Player2_Color = 0xB5FF23; // color7

			//送信処理
			if (send(sock, "0xFF1BAA", strlen("0xFF1BAA") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn9, x, y))
		{

			Player1_Color = 0xFFFA1B; // color9
			Player2_Color = 0x911FFF; // color10

			//送信処理
			if (send(sock, "0xFFFA1B", strlen("0xFFFA1B") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn10, x, y))
		{

			Player1_Color = 0x911FFF; // color10
			Player2_Color = 0xFFFA1B; // color9

			//送信処理
			if (send(sock, "0x911FFF", strlen("0x911FFF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn11, x, y))
		{

			Player1_Color = 0x30B5FF; // color11
			Player2_Color = 0xFF1B6E; // color12

			//送信処理
			if (send(sock, "0x30B5FF", strlen("0x30B5FF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn12, x, y))
		{

			Player1_Color = 0xFF1B6E; // color12
			Player2_Color = 0x30B5FF; // color11

			//送信処理
			if (send(sock, "0xFF1B6E", strlen("0xFF1B6E") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn13, x, y))
		{

			Player1_Color = 0x2CAFFF; // color13
			Player2_Color = 0xA9FF17; // color14

			//送信処理
			if (send(sock, "0x2CAFFF", strlen("0x2CAFFF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);

		}
		if (PtInRegion(rgn14, x, y))
		{

			Player1_Color = 0xA9FF17; // color14
			Player2_Color = 0x2CAFFF; // color13

			//送信処理
			if (send(sock, "0xA9FF17", strlen("0xA9FF17") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn15, x, y))
		{

			Player1_Color = 0x19FF95; // color15
			Player2_Color = 0xFF2D37; // color16

			//送信処理
			if (send(sock, "0x19FF95", strlen("0x19FF95") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn16, x, y))
		{

			Player1_Color = 0xFF2D37; // color16
			Player2_Color = 0x19FF95; // color15

			//送信処理
			if (send(sock, "0xFF2D37", strlen("0xFF2D37") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn17, x, y))
		{

			Player1_Color = 0x21F0FF; // color17
			Player2_Color = 0xFF1A76; // color18

			//送信処理
			if (send(sock, "0x21F0FF", strlen("0x21F0FF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn18, x, y))
		{

			Player1_Color = 0xFF1A76; // color18
			Player2_Color = 0x21F0FF; // color17

			//送信処理
			if (send(sock, "0xFF1A76", strlen("0xFF1A76") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn19, x, y))
		{

			Player1_Color = 0xFFE71F; // color19
			Player2_Color = 0x32FEFF; // color20

			//送信処理
			if (send(sock, "0xFFE71F", strlen("0xFFE71F") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn20, x, y))
		{

			Player1_Color = 0x32FEFF; // color20
			Player2_Color = 0xFFE71F; // color19

			//送信処理
			if (send(sock, "0x32FEFF", strlen("0x32FEFF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn21, x, y))
		{

			Player1_Color = 0x17FF23; // color21
			Player2_Color = 0xB221FF; // color22

			//送信処理
			if (send(sock, "0x17FF23", strlen("0x17FF23") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}
		if (PtInRegion(rgn22, x, y))
		{

			Player1_Color = 0xB221FF; // color22
			Player2_Color = 0x17FF23; // color21

			//送信処理
			if (send(sock, "0xB221FF", strlen("0xB221FF") + 1, 0) == SOCKET_ERROR)
			{
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd20, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}

			InvalidateRgn(hwnd21, rgn1, TRUE);
			InvalidateRgn(hwnd21, rgn2, TRUE);
		}

		DeleteObject(rgn1);
		DeleteObject(rgn2);
		DeleteObject(rgn3);
		DeleteObject(rgn4);
		DeleteObject(rgn5);
		DeleteObject(rgn6);
		DeleteObject(rgn7);
		DeleteObject(rgn8);
		DeleteObject(rgn9);
		DeleteObject(rgn10);
		DeleteObject(rgn11);
		DeleteObject(rgn12);
		DeleteObject(rgn13);
		DeleteObject(rgn14);
		DeleteObject(rgn15);
		DeleteObject(rgn16);
		DeleteObject(rgn17);
		DeleteObject(rgn18);
		DeleteObject(rgn19);
		DeleteObject(rgn20);
		DeleteObject(rgn21);
		DeleteObject(rgn22);

		return 0;

	case WM_SOCKET: // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0)
			return 0L;

		switch (WSAGETSELECTEVENT(lP))
		{
		case FD_READ:

			char buf[MAX_MESSAGE]; // 受信内容を一時的に格納するバッファ

			if (recv(sock, buf, sizeof(buf) - 1, 0) != SOCKET_ERROR)
			{ // 受信できたなら
				if (!strcmp(buf, "0xFF15F2"))
				{
					Player1_Color = 0xFF15F2; // color3
					Player2_Color = 0x26FFF8; // color4
				}
				if (!strcmp(buf, "0x26FFF8"))
				{
					Player1_Color = 0x26FFF8; // color4
					Player2_Color = 0xFF15F2; // color3
				}
				if (!strcmp(buf, "0xFF2244"))
				{
					Player1_Color = 0xFF2244; // color5
					Player2_Color = 0xB221FF; // color6
				}
				if (!strcmp(buf, "0xB221FF"))
				{
					Player1_Color = 0xB221FF; // color6
					Player2_Color = 0xFF2244; // color5
				}
			}

			return 0L;
		}
	}
	return DefWindowProc(hwnd21, msg, wP, lP);
}

LRESULT CALLBACK WindowProc30(HWND hWnd, UINT uMsg, WPARAM wP, LPARAM lP)
{
	static HWND hWndHost;					   // ホスト名入力用エディットボックス
	static HWND hWndConnect, hWndAccept;	   // [接続]ボタンと[接続待ち]ボタン
	static HWND hWndReject, hWndRejectRequest; // [切断]ボタンと[切断要請]ボタン

	static int board_width, board_height, max_width, max_height; // 生成されるボードの幅、高さとその最大値
	static const DOT min_top_left = { 100, 300 };					 // 生成されるボードの左上の点の限度
	static const DOT max_bottom_right = { 600, 700 };				 // 生成されるボードの右下の点の限度

	static int tmp_line = -1, tmp_row = -1; // 引こうとしている線の始点の行、列を保持する変数（線を引こうとしていない場合は-1が入る）
	static int judgement_circle_radius = 0; // ボードの点の判定範囲となる円の半径

	int line, row; // 点の行、列（点[line][row]）

	switch (uMsg)
	{
	case WM_CREATE:		// ウィンドウが生成された


		/*
		// 文字列表示
		CreateWindow("static", "Host Name",
			WS_CHILD | WS_VISIBLE, 10, 10, 100, 18,
			hWnd, NULL, NULL, NULL);

		// ホスト名入力用エディットボックス
		hWndHost = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
			WS_CHILD | WS_VISIBLE, 10, 30, 200, 25,
			hWnd, (HMENU)IDF_HOSTNAME, NULL, NULL);
		// [接続]ボタン
		hWndConnect = CreateWindow("button", "接続",
			WS_CHILD | WS_VISIBLE, 220, 30, 50, 25,
			hWnd, (HMENU)IDB_CONNECT, NULL, NULL);
		// [接続待ち]ボタン
		hWndAccept = CreateWindow("button", "接続待ち",
			WS_CHILD | WS_VISIBLE, 275, 30, 90, 25,
			hWnd, (HMENU)IDB_ACCEPT, NULL, NULL);
		// [切断要請]ボタン
		hWndRejectRequest = CreateWindow("button", "切断要請",
			WS_CHILD | WS_VISIBLE | WS_DISABLED, 220, 60, 90, 25,
			hWnd, (HMENU)IDB_REJECT_REQUEST, NULL, NULL);
		*/

		// [切断]ボタン
		hWndReject = CreateWindow("button", "EXIT",
			WS_CHILD, 50, 25, 55, 35,
			hWnd, (HMENU)IDB_REJECT, NULL, NULL);

		// SetFocus(hWndHost); //フォーカス指定
		SockInit(hWnd); // ソケット初期化

		return 0L;

	case WM_SETUP:

		hPenPlayer1 = (HPEN)CreatePen(PS_SOLID, 3, Player1_Color); // 太さ3ドットの実線のプレイヤー1の色のペンを生成
		hPenPlayer2 = (HPEN)CreatePen(PS_SOLID, 3, Player2_Color); // 太さ3ドットの実線のプレイヤー2の色のペンを生成
		hBrushPlayer1 = (HBRUSH)CreateSolidBrush(Player1_Color);   // プレイヤー1の色のブラシを生成
		hBrushPlayer2 = (HBRUSH)CreateSolidBrush(Player2_Color);   // プレイヤー2の色のブラシを生成
		hPenDot = (HPEN)CreatePen(PS_DASH, 1, 0xD0CECE);		   // 太さ1ドットの破線の灰色ペンを生成
		hPenGray = (HPEN)CreatePen(PS_SOLID, 1, 0xD0CECE);		   // 太さ1ドットの破線の灰色ペンを生成

		max_width = max_bottom_right.x - min_top_left.x;
		max_height = max_bottom_right.y - min_top_left.y;
		mid_board_pos.x = (max_bottom_right.x + min_top_left.x) / 2;
		mid_board_pos.y = (max_bottom_right.y + min_top_left.y) / 2;

		// 値が小さい方をlen_squareに代入(マスの1辺の長さ決定)
		len_square = ((max_width / board_w_num) < (max_height / board_h_num)) ? (max_width / board_w_num) : (max_height / board_h_num);

		board_width = len_square * board_w_num;
		board_height = len_square * board_h_num;

		// ボード（長方形）の中心の座標は、左上の点の座標にボードの幅と高さの半分を足した値
		top_left.x = mid_board_pos.x - board_width / 2;
		top_left.y = mid_board_pos.y - board_height / 2;

		bottom_right.x = top_left.x + board_width;
		bottom_right.y = top_left.y + board_height;
		top_right.x = bottom_right.x;
		top_right.y = top_left.y;
		bottom_left.x = top_left.x;
		bottom_left.y = bottom_right.y;

		judgement_circle_radius = RATIO_OF_JUDGEMENT_CIRCLE_RADIUS * len_square; // ボードの点の判定範囲となる円の半径

		// 各点の座標を代入する
		for (int line = 0; line < board_dot_h_num; line++)
		{
			for (int row = 0; row < board_dot_w_num; row++)
			{
				board_dot[line][row].x = row * len_square + top_left.x;
				board_dot[line][row].y = line * len_square + top_left.y;
			}
		}

		return 0L;


	case WM_PAINT: // 再描画

		return OnPaint(hWnd, uMsg, wP, lP);

	case WM_COMMAND: // ボタンが押された
		switch (LOWORD(wP))
		{
			/*
			case IDB_ACCEPT: // [接続待ち]ボタン押下(サーバー)
				if (SockAccept(hWnd))
				{			   // 接続待ち要求
					return 0L; // 接続待ち失敗
				}
				EnableWindow(hWndHost, FALSE);		   // [HostName]無効
				EnableWindow(hWndConnect, FALSE);	   // [接続]    無効
				EnableWindow(hWndAccept, FALSE);	   // [接続待ち]無効
				EnableWindow(hWndReject, TRUE);		   // [切断]    有効
				EnableWindow(hWndRejectRequest, TRUE); // [切断要請]有効

				my_id = 1; // サーバー側のIDは1
				return 0L;
			*/

			/*case IDB_CONNECT: // [接続]ボタン押下(クライアント)
				char host[100];
				GetWindowText(hWndHost, host, sizeof(host));
				// 接続要求
				if (SockConnect(hWnd, host))
				{
					SetFocus(hWndHost); // 接続失敗
					return 0L;
				}
				EnableWindow(hWndHost, FALSE);		   // [HostName]無効
				EnableWindow(hWndConnect, FALSE);	   // [接続]    無効
				EnableWindow(hWndAccept, FALSE);	   // [接続待ち]無効
				EnableWindow(hWndReject, TRUE);		   // [切断]    有効
				EnableWindow(hWndRejectRequest, TRUE); // [切断要請]有効

				my_id = 2; // クライアント側のIDは2
				return 0L;
			*/

			/*case IDB_REJECT_REQUEST: // [切断要請]ボタン押下
				char buf_reject_request[MAX_MESSAGE];
				sprintf_s(buf_reject_request, sizeof(buf_reject_request), "REJECT");
				// 送信処理
				if (send(sock, buf_reject_request, strlen(buf_reject_request) + 1, 0) == SOCKET_ERROR)
				{ // 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
				return 0L;
			*/

			/*case IDB_REJECT: // [切断]ボタン押下
				if (sock != INVALID_SOCKET)
				{ // 自分がクライアント側なら
					// ソケットを閉じる
					closesocket(sock);
					sock = INVALID_SOCKET;
				}
				if (sv_sock != INVALID_SOCKET)
				{ // 自分がサーバ側なら
					// サーバ用ソケットを閉じる
					closesocket(sv_sock);
					sv_sock = INVALID_SOCKET;
				}
				phe = NULL;
				EnableWindow(hWndHost, TRUE);			// [HostName]有効
				EnableWindow(hWndConnect, TRUE);		// [接続]    有効
				EnableWindow(hWndAccept, TRUE);			// [接続待ち]有効
				EnableWindow(hWndReject, FALSE);		// [切断]    無効
				EnableWindow(hWndRejectRequest, FALSE); // [切断要請]無効
				DestroyWindow(hWnd);					// ウィンドウを閉じる
				return 0L;
			*/
		}

	case WM_SOCKET: // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0)
			return 0L;

		switch (WSAGETSELECTEVENT(lP))
		{
		case FD_ACCEPT: // 接続待ち完了通知
		{
			SOCKADDR_IN cl_sin;
			int len = sizeof(cl_sin);
			sock = accept(sv_sock, (LPSOCKADDR)&cl_sin, &len);

			if (sock == INVALID_SOCKET)
			{
				MessageBox(hWnd, "Accepting connection failed",
					"Error", MB_OK | MB_ICONEXCLAMATION);
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
				EnableWindow(hWndHost, TRUE);			// [HostName]有効
				EnableWindow(hWndConnect, TRUE);		// [接続]    有効
				EnableWindow(hWndAccept, TRUE);			// [接続待ち]有効
				EnableWindow(hWndReject, FALSE);		// [切断]    無効
				EnableWindow(hWndRejectRequest, FALSE); // [切断要請]無効
				SetFocus(hWndHost);						// フォーカス指定
				return 0L;
			}

#ifndef NO_DNS
			// ホスト名取得
			phe = gethostbyaddr((char*)&cl_sin.sin_addr, 4, AF_INET);
			if (phe)
			{
				SetWindowText(hWndHost, phe->h_name);
			}
#endif NO_DNS

			// 非同期モード (受信＆切断）
			if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE) == SOCKET_ERROR)
			{
				// 接続に失敗したら初期状態に戻す
				MessageBox(hWnd, "WSAAsyncSelect() failed",
					"Error", MB_OK | MB_ICONEXCLAMATION);
				EnableWindow(hWndHost, TRUE);			// [HostName]有効
				EnableWindow(hWndConnect, TRUE);		// [接続]    有効
				EnableWindow(hWndAccept, TRUE);			// [接続待ち]有効
				EnableWindow(hWndReject, FALSE);		// [切断]    無効
				EnableWindow(hWndRejectRequest, FALSE); // [切断要請]無効
				SetFocus(hWndHost);						// フォーカス指定
				return 0L;
			}

			return 0L;

		} /* end of case FD_ACCEPT: */

			/*case FD_CONNECT: // 接続完了通知
				// 非同期モード (受信＆切断)
				if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE) == SOCKET_ERROR)
				{
					// 接続に失敗したら初期状態に戻す
					MessageBox(hWnd, "WSAAsyncSelect() failed",
						"Error", MB_OK | MB_ICONEXCLAMATION);
					EnableWindow(hWndHost, TRUE);			// [HostName]有効
					EnableWindow(hWndConnect, TRUE);		// [接続]    有効
					EnableWindow(hWndAccept, TRUE);			// [接続待ち]有効
					EnableWindow(hWndReject, FALSE);		// [切断]    無効
					EnableWindow(hWndRejectRequest, FALSE); // [切断要請]無効
					SetFocus(hWndHost);						// フォーカス指定
					return 0L;
				}
				return 0L;
			*/

		case FD_READ:			   //メッセージ受信
			char buf[MAX_MESSAGE]; // 受信内容を一時的に格納するバッファ
			BOOL is_vir_line;	   // 引く線が縦線の場合・・・TRUE / 横線の場合・・・FALSE

			if (recv(sock, buf, sizeof(buf) - 1, 0) != SOCKET_ERROR)
			{ // 受信できたなら

				//if (strcmp(buf, "REJECT") == 0)  // 切断要請コマンドREJECTだった場合
				//{ 
				//	MessageBox(hWnd, TEXT("切断ボタンを押して速やかに通信を切断してください。"), TEXT("切断要請"), MB_OK);
				//	return 0L;
				//}			

				int i;
				for (i = 0; buf[i] != '\0'; ++i)  //点数の桁数を調べて、iに格納する
					;
				if (i == 3) //受け取った値が3桁ならば実行する
				{
					// bufから線の情報を受け取る
					sscanf_s(buf, "%1d%1d%1d", &is_vir_line, &line, &row);

					// 相手が引いた線をボードに反映させる
					if (is_vir_line)
					{
						vir_line[line][row] = phase;
					}
					else
					{
						hor_line[line][row] = phase;
					}

					// 受け取った線の情報からボードのマスを更新する
					if (!updateSquare(is_vir_line, line, row, phase))
					{
						// ボードのマスが更新されなかったら、相手のフェイズに移行する
						switchPhase(&phase);
					}

				}

				// 自分と相手の得点（マスの数）を集計する
				point_1 = count_square(1);
				point_2 = count_square(2);

				InvalidateRect(hWnd, &d, TRUE);

				// ボードのマスがすべて塗られているかを判定する
				if (is_board_full())
				{
					// 自分と相手の得点（マスの数）を集計する
					point_1 = count_square(1);
					point_2 = count_square(2);

					if (round_mode == 1) {
						Points_1[0] = point_1;
						Points_2[0] = point_2;
					}
					else if (round_mode == 3 || round_mode == 5) {
						Points_1[round_num - 1] = point_1;
						Points_2[round_num - 1] = point_2;
					}


					MessageBox(hWnd, "対戦が終了しました。\n対戦結果に移ります。", "通知", MB_OK | MB_ICONINFORMATION);

					//画面遷移　Screen30→Screen40 or 41/////////////////////////////

					// Screen30を非表示にする
					ShowWindow(hWnd30, SW_HIDE);

					if (round_mode == 1)
					{
						//SocketメッセージをhWnd40に送られるようにする
						WSAAsyncSelect(sock, hWnd40, WM_SOCKET, FD_READ);

						// Screen40を表示する
						ShowWindow(hWnd40, SW_NORMAL);
						ShowWindow(hWndAgain40, SW_NORMAL); //もう一度
						ShowWindow(hWndTitle40, SW_NORMAL); //タイトルに戻る
						InvalidateRect(hWnd40, NULL, TRUE); //再描画

					}
					else if (round_mode == 3)
					{
						//SocketメッセージをhWnd41に送られるようにする
						WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

						if (round_num >= 1 && round_num <= 2) {
							// Screen41を表示する
							ShowWindow(hWnd41, SW_NORMAL);
							ShowWindow(hWndNext, SW_NORMAL);  //次へ
							InvalidateRect(hWnd41, NULL, TRUE); //再描画


						}
						else if (round_num == 3) {

							//SocketメッセージをhWnd41に送られるようにする
							WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

							// Screen41を表示する
							ShowWindow(hWnd41, SW_NORMAL);
							ShowWindow(hWndAgain41, SW_NORMAL); //もう一度
							ShowWindow(hWndTitle41, SW_NORMAL); //タイトルに戻る
							InvalidateRect(hWnd41, NULL, TRUE); //再描画


						}
					}
					else if (round_mode == 5)
					{
						//SocketメッセージをhWnd41に送られるようにする
						WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

						if (round_num >= 1 && round_num <= 4) {
							// Screen41を表示する
							ShowWindow(hWnd41, SW_NORMAL);
							ShowWindow(hWndNext, SW_NORMAL);  //次へ
							InvalidateRect(hWnd41, NULL, TRUE); //再描画


						}
						else if (round_num == 5) {

							//SocketメッセージをhWnd41に送られるようにする
							WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

							// Screen41を表示する
							ShowWindow(hWnd41, SW_NORMAL);
							ShowWindow(hWndAgain41, SW_NORMAL); //もう一度
							ShowWindow(hWndTitle41, SW_NORMAL); //タイトルに戻る
							InvalidateRect(hWnd41, NULL, TRUE); //再描画


						}
					}

					/////////////////////////////////////////////////////////////////////////

				}
			}
			return 0L;

		case FD_CLOSE: // 切断された
			MessageBox(hWnd, "切断されました。",
				"Information", MB_OK | MB_ICONINFORMATION);
			SendMessage(hWnd, WM_COMMAND, IDB_REJECT, 0); // 切断処理発行
			return 0L;
		} /* end of switch (WSAGETSELECTEVENT(lP)) */
		return 0L;

	case WM_LBUTTONDOWN: // マウスの左ボタンが押された（左クリックがされた）
		DOT clicked_dot; // 左クリックされた点
		clicked_dot = { LOWORD(lP), HIWORD(lP) };

		// クリックされた点がボードの枠外（ボードの4隅の点からjudgement_circle_radiusの長さ以上離れている）場合
		if (clicked_dot.x < top_left.x - judgement_circle_radius || clicked_dot.x > bottom_right.x + judgement_circle_radius ||
			clicked_dot.y < top_left.y - judgement_circle_radius || clicked_dot.y > bottom_right.y + judgement_circle_radius)
		{
			return 0L;
		}

		// 自分のフェイズではなかったら終了する
		if (my_id != phase)
		{
			MessageBox(hWnd, "自分のフェイズではありません。",
				"Information", MB_OK | MB_ICONINFORMATION);
			return 0L;
		}

		// クリックされた点が、ボードの点を中心とした半径judgement_circle_radiusの円の内部に存在するか判定
		if (!judgeDotInCircle(clicked_dot, judgement_circle_radius, &line, &row))
		{
			// 点[line][row]は存在しなかったので、lineとrowを一時保存する各変数に-1を代入する
			tmp_line = -1;
			tmp_row = -1;
			return 0L;
		}

		// クリックされた点が、点[line][row]の判定範囲に存在したので、lineとrowの値を一時保存する
		tmp_line = line;
		tmp_row = row;

		return 0L;

	case WM_LBUTTONUP:					 // マウスの左ボタンが放れた
		char buf_line_info[MAX_MESSAGE]; // 引く線の情報が入る配列（相手に送信する予定）
		BOOL is_vir_line;				 // 引く線が縦線の場合・・・TRUE / 横線の場合・・・FALSE
		DOT released_dot;				 // 左ボタンが放された点
		released_dot = { LOWORD(lP), HIWORD(lP) };

		// 始点が存在しなければ、線は引けないので終了する
		if (tmp_line < 0 || tmp_line > board_dot_h_num || tmp_row < 0 || tmp_row > board_dot_w_num)
		{
			return 0L;
		}

		// クリックされた点が、ボードの点を中心とした半径judgement_circle_radiusの円の内部に存在するか判定
		if (!judgeDotInCircle(released_dot, judgement_circle_radius, &line, &row))
		{
			// 点[line][row]が存在しない場合
			tmp_line = -1;
			tmp_row = -1;
			return 0L;
		}

		// 引こうとしている線の始点と終点の組み合わせが、順不同で「(a, b)と(a+1, b)または(a, b)と(a, b+1)」の場合のみ、プレイヤーは線が引ける
		if (abs(tmp_line - line) + abs(tmp_row - row) != 1)
		{
			// 線の始点[tmp_line][tmp_row]と終点[line][row]の間に線が存在しない場合、
			tmp_line = -1;
			tmp_row = -1;
			return 0L;
		}

		// 点[tmp_line][tmp_row]と点[line][row]間に線を引く

		// 縦線を引く（対応する配列の要素を更新して、引いた線の情報をbuf_line_infoに格納する）
		if (tmp_line - line == 1)
		{
			is_vir_line = TRUE;
			vir_line[line][row] = phase;
			sprintf_s(buf_line_info, sizeof(buf_line_info), "%1d%1d%1d", is_vir_line, line, row);
		}
		else if (tmp_line - line == -1)
		{
			is_vir_line = TRUE;
			vir_line[tmp_line][tmp_row] = phase;
			sprintf_s(buf_line_info, sizeof(buf_line_info), "%1d%1d%1d", is_vir_line, tmp_line, tmp_row);
		}
		// 横線を引く（対応する配列の要素を更新して、引いた線の情報をbuf_line_infoに格納する）
		else if (tmp_row - row == 1)
		{
			is_vir_line = FALSE;
			hor_line[line][row] = phase;
			sprintf_s(buf_line_info, sizeof(buf_line_info), "%1d%1d%1d", is_vir_line, line, row);
		}
		// if (tmp_row - row == -1)
		else
		{
			is_vir_line = FALSE;
			hor_line[tmp_line][tmp_row] = phase;
			sprintf_s(buf_line_info, sizeof(buf_line_info), "%1d%1d%1d", is_vir_line, tmp_line, tmp_row);
		}

		// buf_line_infoから、引いた線の情報を得る
		sscanf_s(buf_line_info, "%1d%1d%1d", &is_vir_line, &line, &row);

		// 引いた線の情報からボードのマスを更新する
		if (updateSquare(is_vir_line, line, row, phase))
		{

			/*if (my_id == 1) {
				point_1++;

				if (send(sock, "POINT_1", strlen("POINT_1") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			} else if (my_id == 2) {
				point_2++;

				if (send(sock, "POINT_2", strlen("POINT_2") + 1, 0) == SOCKET_ERROR)
				{
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
						MB_OK | MB_ICONEXCLAMATION);
				}
			}*/
		}
		else
			// ボードのマスが更新されなかったら、相手のフェイズに移行する
			switchPhase(&phase);

		// 自分と相手の得点（マスの数）を集計する
		point_1 = count_square(1);
		point_2 = count_square(2);

		// ボードを更新する
		InvalidateRect(hWnd, &d, TRUE);

		// 送信処理
		if (send(sock, buf_line_info, strlen(buf_line_info) + 1, 0) == SOCKET_ERROR)
		{
			// 送信に失敗したらエラーを表示
			MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
				MB_OK | MB_ICONEXCLAMATION);
		}

		// ボードのマスがすべて塗られているかを判定する
		if (is_board_full())
		{

			// 自分と相手の得点（マスの数）を集計する
			point_1 = count_square(1);
			point_2 = count_square(2);


			if (round_mode == 1) {
				Points_1[0] = point_1;
				Points_2[0] = point_2;
			}
			else if (round_mode == 3 || round_mode == 5) {
				Points_1[round_num - 1] = point_1;
				Points_2[round_num - 1] = point_2;
			}


			MessageBox(hWnd, "対戦が終了しました。\n対戦結果に移ります。", "通知", MB_OK | MB_ICONINFORMATION);

			//対戦結果をファイルに書き込む
			// FILE *fp;
			// fopen_s(&fp, "output.txt", "w+");
			// fprintf(fp, "point_1 = %d\n", point_1);
			// fprintf(fp, "point_2 = %d\n", point_2);
			// fclose(fp);


			//画面遷移　Screen30→Screen40 or 41////////////////////////

			// Screen30を非表示にする
			ShowWindow(hWndReject, SW_HIDE);
			ShowWindow(hWnd30, SW_HIDE);

			if (round_mode == 1)
			{
				// WM_SOCKETがScreen40に送られるように切り替える
				WSAAsyncSelect(sock, hWnd40, WM_SOCKET, FD_READ);

				// Screen40を表示する
				ShowWindow(hWnd40, SW_NORMAL);
				ShowWindow(hWndAgain40, SW_NORMAL); //もう一度
				ShowWindow(hWndTitle40, SW_NORMAL); //タイトルに戻る
				InvalidateRect(hWnd40, NULL, TRUE); //再描画

			}
			else if (round_mode == 3)
			{

				if (round_num >= 1 && round_num <= 2) {

					// WM_SOCKETがScreen41に送られるように切り替える
					WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

					// Screen41を表示する
					ShowWindow(hWnd41, SW_NORMAL);
					ShowWindow(hWndNext, SW_NORMAL);  //次へ
					InvalidateRect(hWnd41, NULL, TRUE); //再描画

				}
				else if (round_num == 3) {

					// WM_SOCKETがScreen41に送られるように切り替える
					WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

					// Screen41を表示する
					ShowWindow(hWnd41, SW_NORMAL);
					ShowWindow(hWndAgain41, SW_NORMAL); //もう一度
					ShowWindow(hWndTitle41, SW_NORMAL); //タイトルに戻る
					InvalidateRect(hWnd41, NULL, TRUE); //再描画

				}
			}
			else if (round_mode == 5)
			{

				if (round_num >= 1 && round_num <= 4) {

					// WM_SOCKETがScreen41に送られるように切り替える
					WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

					// Screen41を表示する
					ShowWindow(hWnd41, SW_NORMAL);
					ShowWindow(hWndNext, SW_NORMAL);  //次へ
					InvalidateRect(hWnd41, NULL, TRUE); //再描画

				}
				else if (round_num == 5) {

					// WM_SOCKETがScreen41に送られるように切り替える
					WSAAsyncSelect(sock, hWnd41, WM_SOCKET, FD_READ);

					// Screen41を表示する
					ShowWindow(hWnd41, SW_NORMAL);
					ShowWindow(hWndAgain41, SW_NORMAL); //もう一度
					ShowWindow(hWndTitle41, SW_NORMAL); //タイトルに戻る
					InvalidateRect(hWnd41, NULL, TRUE); //再描画

				}
			}

			/////////////////////////////////////////////////////////////

		}

		return 0L;

	case WM_SETFOCUS: // ウィンドウにフォーカスが来たら
		// ホスト名入力欄が入力可ならフォーカス
		if (IsWindowEnabled(hWndHost))
			SetFocus(hWndHost);
		return 0L;

	case WM_DESTROY: // ウィンドウが破棄された
		closesocket(sock);
		DeleteObject(hPenPlayer1);
		DeleteObject(hPenPlayer2);
		DeleteObject(hPenDot);
		DeleteObject(hPenGray);
		PostQuitMessage(0);
		return 0L;
	default:
		return DefWindowProc(hWnd, uMsg, wP, lP); // 標準メッセージ処理
	}
	/* end of switch (uMsg) */
}

////////////////////////////////////////////////////////////////////////////////
//
//  roundが1の時
//
LRESULT CALLBACK WindowProc40(HWND hWnd40, UINT uMsg, WPARAM wP, LPARAM lP)
{
	//定義//////////////////////////////////////////////////////////////////////////////

	hBrushWhite = (HBRUSH)CreateSolidBrush(RGB(255, 255, 255));
	int i, j;

	//処理////////////////////////////////////////////////////////////////////////////
	switch (uMsg) {

	case WM_CREATE:     // ウィンドウが生成された   
		hfon = (HFONT)GetStockObject(SYSTEM_FONT); // フォントを取得

		// ボタン　もう一度
		hWndAgain40 = CreateWindow("button", "もう一度",
			WS_CHILD | WS_VISIBLE, 100, 600, 200, 50,
			hWnd40, (HMENU)IDB_AGAIN, NULL, NULL);
		// ボタン　タイトルへ戻る
		hWndTitle40 = CreateWindow("button", "タイトルへ戻る",
			WS_CHILD | WS_VISIBLE, 400, 600, 200, 50,
			hWnd40, (HMENU)IDB_TITLE, NULL, NULL);

		return 0L;

		//再描画/////////////////////////////////////////////////////////////////////////////
	case WM_PAINT:      // 再描画

		HDC hdc;
		PAINTSTRUCT ps;
		char buf1[10], buf2[10];
		int res;   //1ならクライアントの勝ち、2ならサーバの勝ち
		res = 0;

		hfon1 = CreateFont(50, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ

		hdc = BeginPaint(hWnd40, &ps);
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォントを選択
		//SetTextColor(hdc, 0xFFAF2C);  // 文字色を設定

		TextOut(hdc, 290, 55, "Score", lstrlen("Score"));

		hfon1 = CreateFont(80, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ

		SelectObject(hdc, hfon1);  // フォント選択

		wsprintf(buf1, "%d", Points_1[0]);
		wsprintf(buf2, "%d", Points_2[0]);

		if (Points_1[0] > Points_2[0]) {
			SetTextColor(hdc, Player1_Color);  // 文字色を設定
			TextOut(hdc, 180, 300, buf1, lstrlen(buf1));
			SetTextColor(hdc, 0x000000);  // 文字色を設定
			TextOut(hdc, 480, 300, buf2, lstrlen(buf2));
			res = 1;
		}
		else if (Points_1[0] < Points_2[0]) {
			TextOut(hdc, 180, 300, buf1, lstrlen(buf1));
			SetTextColor(hdc, Player2_Color);  // 文字色を設定
			TextOut(hdc, 480, 300, buf2, lstrlen(buf2));
			SetTextColor(hdc, 0x000000);  // 文字色を設定
			res = 2;
		}
		else {
			TextOut(hdc, 180, 300, buf1, lstrlen(buf1));
			TextOut(hdc, 480, 300, buf2, lstrlen(buf2));
		}

		hfon1 = CreateFont(40, 0, //高さ, 幅
			0, 0, 0,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ

		SelectObject(hdc, hfon1);  // フォント選択


		//プレイヤー名の表示///////////////////////////////////////////////////////////////
		if (my_id == 1)
		{
			TextOut(hdc, 260 - lstrlen(Player1_name) / 2 * 40, 200, Player1_name, lstrlen(Player1_name));

			if (strlen(Player2_name) == 0) {
				TextOut(hdc, 560 - lstrlen("Player2") / 2 * 40, 200, "Player2", lstrlen("Player2"));
			}
			else {
				TextOut(hdc, 560 - lstrlen(Player2_name) / 2 * 40, 200, Player2_name, lstrlen(Player2_name)); //相手の名前
			}

		}
		else if (my_id == 2)
		{
			if (strlen(Player1_name) == 0) {
				TextOut(hdc, 260 - lstrlen("Player1") / 2 * 40, 200, "Player1", lstrlen("Player1"));
			}
			else {
				TextOut(hdc, 260 - lstrlen(Player1_name) / 2 * 40, 200, Player1_name, lstrlen(Player1_name)); //相手の名前
			}

			TextOut(hdc, 560 - lstrlen(Player2_name) / 2 * 40, 200, Player2_name, lstrlen(Player2_name));

		}


		if (res == 0) { // 引き分け
			TextOut(hdc, 250, 450, "引き分けです", lstrlen("引き分けです"));
		}
		else if (my_id == 1 && res == 1) {    // 自分がクライアント側かつクライアントの勝利
			TextOut(hdc, 200, 450, "あなたの勝ちです", lstrlen("あなたの勝ちです"));  //勝敗表示
		}
		else if (my_id == 2 && res == 2) {  // 自分がサーバ側かつサーバの勝利
			TextOut(hdc, 200, 450, "あなたの勝ちです", lstrlen("あなたの勝ちです"));  //勝敗表示
		}
		else {
			TextOut(hdc, 200, 450, "あなたの負けです", lstrlen("あなたの負けです"));  //勝敗表示
		}

		hBrushPrev = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
		hPen = (HPEN)CreatePen(PS_INSIDEFRAME, 1, 0x00000000);

		SelectObject(hdc, hPen);

		Rectangle(hdc, 280, 50, 420, 110);

		SelectObject(hdc, hBrushPrev);
		DeleteObject(SelectObject(hdc, hPenPrev));
		DeleteObject(SelectObject(hdc, hfonPrev));
		DeleteObject(hfon1);
		res = 0;

		//遷移フラグを表示させる///////////////////////////////////////////////////////////////////////////
		if (again1 == 1 && again2 == 1)
		{
			TextOut(hdc, 100, 660, TEXT("(clicked Again Button(2))"), strlen("(clicked Again Button(2))"));
		}
		else if (again1 == 1 || again2 == 1) {
			TextOut(hdc, 100, 660, TEXT("(clicked Again Button(1))"), strlen("(clicked Again Button(1))"));
		}

		EndPaint(hWnd40, &ps);

		return 0L;


		//ボタン関連//////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND:    // ボタンが押された
		switch (LOWORD(wP)) {
		case IDB_AGAIN:   // [もう一度]ボタン押下
			char buf_again[MAX_MESSAGE];
			again1 = 1;

			InvalidateRect(hWnd40, NULL, TRUE);

			sprintf_s(buf_again, sizeof(buf_again), "AGAIN");
			if (send(sock, buf_again, strlen(buf_again) + 1, 0) == SOCKET_ERROR) {    // 送信処理
			 // 送信に失敗したらエラーを表示
				MessageBox(hWnd40, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}
			else if (again2 == 1) {      // 相手も押していたら

				MessageBox(hWnd40, "ゲーム設定の画面に戻ります", "Information", MB_OK | MB_ICONINFORMATION);

				for (i = 0; i < 5; i++) {  // 対戦内容を初期化する
					Points_1[i] = 0;
					Points_2[i] = 0;
				}
				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_w_num + 1; j++) {
						vir_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num + 1; i++) {
					for (j = 0; j <= board_w_num; j++) {
						hor_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_w_num; j++) {
						square[i][j] = 0;
					}
				}

				//【画面遷移】Screen40→Screen20////////////////////////////

				//1回のゲーム内での点数を初期化する
				point_1 = 0;
				point_2 = 0;

				//ラウンド数を初期化する, phaseとround_modeとPlayer_colorは戻さない
				round_num = 1;

				again1 = 0;
				again2 = 0;

				//Screen40を非表示にする
				ShowWindow(hWndAgain40, SW_HIDE);
				ShowWindow(hWndTitle40, SW_HIDE);
				ShowWindow(hWnd40, SW_HIDE);

				//Screen20を表示させる
				ShowWindow(hWnd20, SW_NORMAL);
				ShowWindow(hWndPlayStart, SW_NORMAL);
				ShowWindow(hWndReject, SW_NORMAL);
				ShowWindow(hListVer, SW_NORMAL);
				ShowWindow(hListHol, SW_NORMAL);
				ShowWindow(hRadPlayer1, SW_NORMAL);
				ShowWindow(hRadPlayer2, SW_NORMAL);
				ShowWindow(hRadRound1, SW_NORMAL);
				ShowWindow(hRadRound5, SW_NORMAL);
				InvalidateRect(hWnd20, NULL, TRUE); //再描画

				//SocketメッセージをhWnd20に送られるようにする
				WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

				/////////////////////////////////////////////////////////////				

				return 0L;
			}

			return 0L;

		case IDB_TITLE:    // [タイトルへ]ボタン押下			

			if (sock != INVALID_SOCKET) {    // 自分がクライアント側なら
				// ソケットを閉じる
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			if (sv_sock != INVALID_SOCKET) { // 自分がサーバ側なら
				// サーバ用ソケットを閉じる
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
			}

			//【画面遷移】Screen40→Screen10///////////////////////////////////////

			//初期化する
			point_1 = 0;
			point_2 = 0;

			for (i = 0; i < 5; i++) {  // 対戦内容初期化
				Points_1[i] = 0;
				Points_2[i] = 0;
			}
			for (i = 0; i <= board_h_num; i++) {
				for (j = 0; j <= board_w_num + 1; j++) {
					vir_line[i][j] = 0;
				}
			}
			for (i = 0; i <= board_h_num + 1; i++) {
				for (j = 0; j <= board_w_num; j++) {
					hor_line[i][j] = 0;
				}
			}
			for (i = 0; i <= board_h_num; i++) {
				for (j = 0; j <= board_w_num; j++) {
					square[i][j] = 0;
				}
			}

			board_w_num = 3;
			board_h_num = 3;
			Player1_Color = Player1_Default_Color;
			Player2_Color = Player2_Default_Color;
			phase = 1;
			round_mode = 1;
			round_num = 1;

			next1 = 0; next2 = 0;  //遷移フラグ
			again1 = 0; again2 = 0;

			SendMessage(hListVer, LB_SETCURSEL, 1, 0);
			SendMessage(hListHol, LB_SETCURSEL, 1, 0);
			SendMessage(hRadPlayer1, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hRadPlayer2, BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(hRadRound1, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hRadRound3, BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(hRadRound5, BM_SETCHECK, BST_UNCHECKED, 0);

			SendMessage(hWnd30, WM_SETUP, 0, 0); //初期化したゲーム設定のゲーム画面を構成する

			EnableWindow(hWndConnect, TRUE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndAcceptCancel, FALSE);


			//Screen40を非表示にする
			ShowWindow(hWndAgain40, SW_HIDE);
			ShowWindow(hWndTitle40, SW_HIDE);
			ShowWindow(hWnd40, SW_HIDE);

			//SocketメッセージをhWnd10に送られるようにする
			WSAAsyncSelect(sock, hWnd10, WM_SOCKET, FD_ACCEPT | FD_CONNECT | FD_READ | FD_CLOSE);

			//Screen10を表示させる
			ShowWindow(hWndPlayer, SW_NORMAL);
			ShowWindow(hWndHost, SW_NORMAL);
			ShowWindow(hWndConnect, SW_NORMAL);
			ShowWindow(hWndAccept, SW_NORMAL);
			ShowWindow(hWndExplanation, SW_NORMAL);
			ShowWindow(hWnd10, SW_NORMAL);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndConnect, TRUE);
			InvalidateRect(hWnd10, NULL, TRUE);

			/////////////////////////////////////////////////////////////////////////

			return 0L;
		}

		//通信関連 //////////////////////////////////////////////////////////////////
	case WM_SOCKET:          // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0) { return 0L; }

		switch (WSAGETSELECTEVENT(lP)) {
		case FD_READ:       //メッセージ受信
			char buf[MAX_MESSAGE];                  // 受信内容を一時的に格納するバッファ
			if (recv(sock, buf, sizeof(buf) - 1, 0) != SOCKET_ERROR) { // 受信できたなら
				if (strcmp(buf, "AGAIN") == 0) {

					again2 = 1;

					InvalidateRect(hWnd40, NULL, TRUE);

					if (again1 == 1) {     // 自分も押していたら

						MessageBox(hWnd40, "ゲーム設定の画面に戻ります", "Information", MB_OK | MB_ICONINFORMATION);

						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num + 1; j++) {
								vir_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num + 1; i++) {
							for (j = 0; j <= board_w_num; j++) {
								hor_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num; j++) {
								square[i][j] = 0;
							}
						}

						//【画面遷移】Screen40→Screen20////////////////////////////

						//1回のゲーム内での点数を初期化する
						point_1 = 0;
						point_2 = 0;

						//ラウンド数を初期化する, phaseとround_modeとPlayer_colorは戻さない
						round_num = 1;

						again1 = 0; //遷移フラグ
						again2 = 0;

						//Screen40を非表示にする
						ShowWindow(hWndAgain40, SW_HIDE);
						ShowWindow(hWndTitle40, SW_HIDE);
						ShowWindow(hWnd40, SW_HIDE);

						//Screen20を表示させる
						ShowWindow(hWnd20, SW_NORMAL);
						ShowWindow(hWndPlayStart, SW_NORMAL);
						ShowWindow(hWndReject, SW_NORMAL);
						ShowWindow(hListVer, SW_NORMAL);
						ShowWindow(hListHol, SW_NORMAL);
						ShowWindow(hRadPlayer1, SW_NORMAL);
						ShowWindow(hRadPlayer2, SW_NORMAL);
						ShowWindow(hRadRound1, SW_NORMAL);
						ShowWindow(hRadRound5, SW_NORMAL);
						InvalidateRect(hWnd20, NULL, TRUE); //再描画

						//SocketメッセージをhWnd20に送られるようにする
						WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

						/////////////////////////////////////////////////////////////

					}
				}
			}
			return 0L;
		}
		//デフォルトな処理/////////////////////////////////////////////////////////
	case WM_SETFOCUS:   // ウィンドウにフォーカスが来たら
		return 0L;
	case WM_DESTROY:    // ウィンドウが破棄された
		closesocket(sock);
		PostQuitMessage(0);
		return 0L;
	default:
		return DefWindowProc(hWnd40, uMsg, wP, lP);  // 標準メッセージ処理
	}/* end of switch (uMsg) */
	return 0L;
}

////////////////////////////////////////////////////////////////////////////////
//
//  ウィンドウ関数(イベント処理を記述)
//
LRESULT CALLBACK WindowProc41(HWND hWnd41, UINT uMsg, WPARAM wP, LPARAM lP)
{
	//定義//////////////////////////////////////////////////////////////////////////////

	hBrushWhite = (HBRUSH)CreateSolidBrush(RGB(255, 255, 255));
	int i, j;

	//処理////////////////////////////////////////////////////////////////////////////
	switch (uMsg) {

	case WM_CREATE:     // ウィンドウが生成された   

		hfon = (HFONT)GetStockObject(SYSTEM_FONT); // フォントを取得

		// [次の試合へ]ボタン
		hWndNext = CreateWindow("button", "次の試合へ",
			WS_CHILD | WS_VISIBLE, 240, 650, 200, 50,
			hWnd41, (HMENU)IDB_NEXT, NULL, NULL);

		// [もう一度]ボタン
		hWndAgain41 = CreateWindow("button", "もう一度",
			WS_CHILD, 100, 650, 200, 50,
			hWnd41, (HMENU)IDB_AGAIN, NULL, NULL);
		// [タイトルへ戻る]ボタン
		hWndTitle41 = CreateWindow("button", "タイトルへ戻る",
			WS_CHILD, 400, 650, 200, 50,
			hWnd41, (HMENU)IDB_TITLE, NULL, NULL);

		return 0L;

		//再描画/////////////////////////////////////////////////////////////////////////////
	case WM_PAINT:      // 再描画

		HDC hdc;
		PAINTSTRUCT ps;
		char buf1[2], buf2[2], rbuf[10], sbuf1[2], sbuf2[2];

		score_1 = 0;
		score_2 = 0;

		hfon1 = CreateFont(50, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ

		hdc = BeginPaint(hWnd41, &ps);

		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォントを選択
		//SetTextColor(hdc, 0xFFAF2C);  // 文字色を設定


		//画面の左上のScoreを表示する////////////////////////////////////////////////////////////
		TextOut(hdc, 50, 50, "Score", lstrlen("Score"));


		hfon1 = CreateFont(25, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォント選択


		//各ROUNDの点数を表示する////////////////////////////////////////////////////////////////
		for (i = 0; i < round_num; i++) {
			wsprintf(buf1, "%d", Points_1[i]);
			wsprintf(buf2, "%d", Points_2[i]);

			if (Points_1[i] > Points_2[i]) {
				SetTextColor(hdc, Player1_Color);  // 文字色を設定
				TextOut(hdc, 190, 450 + 35 * i, buf1, lstrlen(buf1));
				SetTextColor(hdc, 0x000000);  // 文字色を設定
				TextOut(hdc, 490, 450 + 35 * i, buf2, lstrlen(buf2));
				score_1 = score_1 + 1;
			}
			else if (Points_1[i] < Points_2[i]) {
				TextOut(hdc, 190, 450 + 35 * i, buf1, lstrlen(buf1));
				SetTextColor(hdc, Player2_Color);  // 文字色を設定
				TextOut(hdc, 490, 450 + 35 * i, buf2, lstrlen(buf2));
				SetTextColor(hdc, 0x000000);  // 文字色を設定
				score_2 = score_2 + 1;
			}
			else {
				TextOut(hdc, 190, 450 + 35 * i, buf1, lstrlen(buf1));
				TextOut(hdc, 490, 450 + 35 * i, buf2, lstrlen(buf2));
			}
		}


		hfon1 = CreateFont(70, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォント選択

		wsprintf(buf1, "%d", Points_1[round_num - 1]);
		wsprintf(buf2, "%d", Points_2[round_num - 1]);
		wsprintf(sbuf1, "%d", score_1);
		wsprintf(sbuf2, "%d", score_2);


		//今回のROUNDの点数を表示する////////////////////////////////////////////////////////////////
		if (Points_1[round_num - 1] > Points_2[round_num - 1]) {
			SetTextColor(hdc, Player1_Color);  // 文字色を設定
			TextOut(hdc, 190, 180, buf1, lstrlen(buf1));
			SetTextColor(hdc, 0x000000);  // 文字色を設定
			TextOut(hdc, 490, 180, buf2, lstrlen(buf2));
		}
		else if (Points_1[round_num - 1] < Points_2[round_num - 1]) {
			TextOut(hdc, 190, 180, buf1, lstrlen(buf1));
			SetTextColor(hdc, Player2_Color);  // 文字色を設定
			TextOut(hdc, 490, 180, buf2, lstrlen(buf2));
			SetTextColor(hdc, 0x000000);  // 文字色を設定
		}
		else {
			TextOut(hdc, 190, 180, buf1, lstrlen(buf1));
			TextOut(hdc, 490, 180, buf2, lstrlen(buf2));
		}

		//総合結果の得点を表示する///////////////////////////////////////////////////////////////////
		TextOut(hdc, 135, 360, sbuf1, lstrlen(sbuf1)); //Player1が勝ったROUND数
		TextOut(hdc, 520, 360, sbuf2, lstrlen(sbuf2)); //Player2が勝ったROUND数


		hfon1 = CreateFont(35, 0, //高さ, 幅
			0, 0, 0,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォント選択


		//今回のROUND数と名前を表示する////////////////////////////////////////////////////////////////
		//ROUND%d
		wsprintf(rbuf, "ROUND%d", round_num);
		TextOut(hdc, 275, 55, rbuf, lstrlen(rbuf));

		//Player1・Player2の名前
		if (my_id == 1)
		{
			TextOut(hdc, 200 - lstrlen(Player2_name) / 2 * 15, 130, Player1_name, lstrlen(Player1_name));

			if (strlen(Player2_name) == 0) {
				TextOut(hdc, 455, 130, "Player2", lstrlen("Player2"));
			}
			else {
				TextOut(hdc, 500 - lstrlen(Player2_name) / 2 * 15, 130, Player2_name, lstrlen(Player2_name)); //相手の名前
			}

		}
		else if (my_id == 2)
		{
			if (strlen(Player1_name) == 0) {
				TextOut(hdc, 155, 130, "Player1", lstrlen("Player1"));
			}
			else {
				TextOut(hdc, 200 - lstrlen(Player1_name) / 2 * 15, 130, Player1_name, lstrlen(Player1_name)); //相手の名前
			}

			TextOut(hdc, 500 - lstrlen(Player2_name) / 2 * 15, 130, Player2_name, lstrlen(Player2_name));

		}


		//～総合結果～
		TextOut(hdc, 240, 300, "～総合結果～", lstrlen("～総合結果～"));

		hfon1 = CreateFont(30, 0, //高さ, 幅
			0, 0, 0,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ
		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォント選択


		//総合結果のPlayer1・Player2の名前
		if (my_id == 1)
		{
			TextOut(hdc, 240 - lstrlen(Player1_name) / 2 * 12, 395, Player1_name, lstrlen(Player1_name));

			if (strlen(Player2_name) == 0) {
				TextOut(hdc, 394, 395, "Player2", lstrlen("Player2"));
			}
			else {
				TextOut(hdc, 430 - lstrlen(Player2_name) / 2 * 12, 395, Player2_name, lstrlen(Player2_name)); //相手の名前
			}

		}
		else if (my_id == 2)
		{
			if (strlen(Player1_name) == 0) {
				TextOut(hdc, 164, 395, "Player1", lstrlen("Player1"));
			}
			else {
				TextOut(hdc, 200 - lstrlen(Player1_name) / 2 * 12, 395, Player1_name, lstrlen(Player1_name)); //相手の名前
			}

			TextOut(hdc, 430 - lstrlen(Player2_name) / 2 * 12, 395, Player2_name, lstrlen(Player2_name));

		}


		hfon1 = CreateFont(20, 0, //高さ, 幅
			0, 0, 0,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ

		hfonPrev = (HFONT)SelectObject(hdc, hfon1);  // フォント選択


		//総合結果のROUND1～ROUND%dの描画//////////////////////////////////////////
		if (round_mode == 3) {  //ROUND3の場合
			for (i = 0; i < 3; i++) {
				wsprintf(rbuf, "ROUND%d", i + 1);
				TextOut(hdc, 305, 450 + 35 * i, rbuf, lstrlen(rbuf));
			}
		}
		else if (round_mode == 5) { //ROUND5の場合
			for (i = 0; i < 5; i++) {
				wsprintf(rbuf, "ROUND%d", i + 1);
				TextOut(hdc, 305, 450 + 35 * i, rbuf, lstrlen(rbuf));
			}
		}

		hBrushPrev = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
		hPen = (HPEN)CreatePen(PS_INSIDEFRAME, 1, 0x00000000);
		hPenPrev = (HPEN)SelectObject(hdc, hPen);


		//枠と線の描画/////////////////////////////////////////////////////////////
		MoveToEx(hdc, 50, 440, 0);
		LineTo(hdc, 650, 440);               //総合結果の名前の下の線
		Rectangle(hdc, 240, 50, 445, 100);   //上部のROUNDの枠
		Rectangle(hdc, 125, 360, 185, 430);  //総合結果のPlayer1の点数の枠
		Rectangle(hdc, 510, 360, 570, 430);  //総合結果のPlayer2の点数の枠

		if (round_mode == 3) {
			RoundRect(hdc, 50, 350, 650, 550, 30, 30);  //総合結果の枠
		}
		else if (round_mode == 5) {
			RoundRect(hdc, 50, 350, 650, 620, 30, 30);  //総合結果の枠
		}



		//王冠を描画する////////////////////////////////////////////////////////////
		if (round_mode == 3) {
			if (round_num == 3) {
				if (score_1 > score_2) {
					MoveToEx(hdc, 105 - 42, 270 + 115, 0); LineTo(hdc, 105 - 42, 300 + 115);
					LineTo(hdc, 155 - 42, 300 + 115); LineTo(hdc, 155 - 42, 270 + 115); LineTo(hdc, 145 - 42, 280 + 115);
					LineTo(hdc, 130 - 42, 265 + 115); LineTo(hdc, 115 - 42, 280 + 115); LineTo(hdc, 105 - 42, 270 + 115);
					Ellipse(hdc, 103 - 42, 265 + 115, 108 - 42, 270 + 115); Ellipse(hdc, 128 - 42, 260 + 115, 133 - 42, 265 + 115);
					Ellipse(hdc, 153 - 42, 265 + 115, 158 - 42, 270 + 115);
				}
				else if (score_1 < score_2) {
					MoveToEx(hdc, 570 + 15, 270 + 115, 0);  LineTo(hdc, 570 + 15, 300 + 115);
					LineTo(hdc, 620 + 15, 300 + 115);  LineTo(hdc, 620 + 15, 270 + 115); LineTo(hdc, 610 + 15, 280 + 115);
					LineTo(hdc, 595 + 15, 265 + 115); LineTo(hdc, 580 + 15, 280 + 115);  LineTo(hdc, 570 + 15, 270 + 115);
					Ellipse(hdc, 568 + 15, 265 + 115, 573 + 15, 270 + 115); Ellipse(hdc, 593 + 15, 260 + 115, 598 + 15, 265 + 115);
					Ellipse(hdc, 618 + 15, 265 + 115, 623 + 15, 270 + 115);
				}
			}
		}
		else if (round_mode == 5) {
			if (round_num == 5) {
				if (score_1 > score_2) {
					MoveToEx(hdc, 105 - 42, 270 + 115, 0); LineTo(hdc, 105 - 42, 300 + 115);
					LineTo(hdc, 155 - 42, 300 + 115); LineTo(hdc, 155 - 42, 270 + 115); LineTo(hdc, 145 - 42, 280 + 115);
					LineTo(hdc, 130 - 42, 265 + 115); LineTo(hdc, 115 - 42, 280 + 115); LineTo(hdc, 105 - 42, 270 + 115);
					Ellipse(hdc, 103 - 42, 265 + 115, 108 - 42, 270 + 115); Ellipse(hdc, 128 - 42, 260 + 115, 133 - 42, 265 + 115);
					Ellipse(hdc, 153 - 42, 265 + 115, 158 - 42, 270 + 115);
				}
				else if (score_1 < score_2) {
					MoveToEx(hdc, 570 + 15, 270 + 115, 0);  LineTo(hdc, 570 + 15, 300 + 115);
					LineTo(hdc, 620 + 15, 300 + 115);  LineTo(hdc, 620 + 15, 270 + 115); LineTo(hdc, 610 + 15, 280 + 115);
					LineTo(hdc, 595 + 15, 265 + 115); LineTo(hdc, 580 + 15, 280 + 115);  LineTo(hdc, 570 + 15, 270 + 115);
					Ellipse(hdc, 568 + 15, 265 + 115, 573 + 15, 270 + 115); Ellipse(hdc, 593 + 15, 260 + 115, 598 + 15, 265 + 115);
					Ellipse(hdc, 618 + 15, 265 + 115, 623 + 15, 270 + 115);
				}
			}
		}


		//遷移フラグを表示させる///////////////////////////////////////////////////////////////////////////
		if (again1 == 1 && again2 == 1)
		{
			TextOut(hdc, 100, 710, TEXT("(clicked Again Button(2))"), strlen("(clicked Again Button(2))"));
		}
		else if (again1 == 1 || again2 == 1) {
			TextOut(hdc, 100, 710, TEXT("(clicked Again Button(1))"), strlen("(clicked Again Button(1))"));
		}

		if (next1 == 1 && next2 == 1)
		{
			TextOut(hdc, 455, 675, TEXT("(clicked Next Button(2))"), strlen("(clicked Next Button(2))"));
		}
		else if (next1 == 1 || next2 == 1) {
			TextOut(hdc, 455, 675, TEXT("(clicked Next Button(1))"), strlen("(clicked Next Button(1))"));
		}


		SelectObject(hdc, hBrushPrev);
		DeleteObject(SelectObject(hdc, hPenPrev));
		DeleteObject(SelectObject(hdc, hfonPrev));
		DeleteObject(hfon1);

		EndPaint(hWnd41, &ps);

		return 0L;

		//ボタン関連//////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND:    // ボタンが押された

		switch (LOWORD(wP)) {

		case IDB_AGAIN:

			char buf_again[MAX_MESSAGE];
			again1 = 1;

			InvalidateRect(hWnd41, NULL, TRUE);

			sprintf_s(buf_again, sizeof(buf_again), "AGAIN");
			if (send(sock, buf_again, strlen(buf_again) + 1, 0) == SOCKET_ERROR) {    // 送信処理
			 // 送信に失敗したらエラーを表示
				MessageBox(hWnd41, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}
			else if (again2 == 1) {      // 相手も押していたら

				MessageBox(hWnd41, "ゲーム設定の画面に戻ります", "Information", MB_OK | MB_ICONINFORMATION);

				for (i = 0; i < 5; i++) {  // 対戦内容を初期化する
					Points_1[i] = 0;
					Points_2[i] = 0;
				}
				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_w_num + 1; j++) {
						vir_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num + 1; i++) {
					for (j = 0; j <= board_w_num; j++) {
						hor_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_w_num; j++) {
						square[i][j] = 0;
					}
				}

				//【画面遷移】Screen41→Screen20////////////////////////////

				//初期化する
				point_1 = 0;
				point_2 = 0;

				round_num = 1;  //phaseとround_modeとPlayer_colorは戻さない

				next1 = 0; next2 = 0;  //遷移フラグ
				again1 = 0; again2 = 0;

				//Screen41を非表示にする
				ShowWindow(hWndAgain41, SW_HIDE);
				ShowWindow(hWndTitle41, SW_HIDE);
				ShowWindow(hWnd41, SW_HIDE);

				//SocketメッセージをhWnd20に送られるようにする
				WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

				//Screen20を表示させる
				ShowWindow(hWnd20, SW_NORMAL);
				ShowWindow(hWndPlayStart, SW_NORMAL);
				ShowWindow(hWndReject, SW_NORMAL);
				ShowWindow(hListVer, SW_NORMAL);
				ShowWindow(hListHol, SW_NORMAL);
				ShowWindow(hRadPlayer1, SW_NORMAL);
				ShowWindow(hRadPlayer2, SW_NORMAL);
				ShowWindow(hRadRound1, SW_NORMAL);
				ShowWindow(hRadRound5, SW_NORMAL);
				InvalidateRect(hWnd20, NULL, TRUE); //再描画

				/////////////////////////////////////////////////////////////

				return 0L;
			}

			return 0L;

		case IDB_TITLE:    // [タイトルへ]ボタン押下	

			if (sock != INVALID_SOCKET) {    // 自分がクライアント側なら
				// ソケットを閉じる
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			if (sv_sock != INVALID_SOCKET) { // 自分がサーバ側なら
				// サーバ用ソケットを閉じる
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
			}

			//画面遷移 Screen41→Screen10///////////////////////////////////////////////////////

			//初期化する
			point_1 = 0;  //ゲーム内の点数
			point_2 = 0;

			for (i = 0; i < 5; i++) {  // 対戦内容初期化
				Points_1[i] = 0;
				Points_2[i] = 0;
			}
			for (i = 0; i <= board_h_num; i++) {
				for (j = 0; j <= board_w_num + 1; j++) {
					vir_line[i][j] = 0;
				}
			}
			for (i = 0; i <= board_h_num + 1; i++) {
				for (j = 0; j <= board_w_num; j++) {
					hor_line[i][j] = 0;
				}
			}
			for (i = 0; i <= board_h_num; i++) {
				for (j = 0; j <= board_w_num; j++) {
					square[i][j] = 0;
				}
			}

			board_w_num = 3;
			board_h_num = 3;
			Player1_Color = Player1_Default_Color;
			Player2_Color = Player2_Default_Color;
			phase = 1;
			round_mode = 1;
			round_num = 1;

			next1 = 0; next2 = 0;  //遷移フラグ
			again1 = 0; again2 = 0;

			SendMessage(hListVer, LB_SETCURSEL, 1, 0);
			SendMessage(hListHol, LB_SETCURSEL, 1, 0);
			SendMessage(hRadPlayer1, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hRadPlayer2, BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(hRadRound1, BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(hRadRound3, BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(hRadRound5, BM_SETCHECK, BST_UNCHECKED, 0);


			SendMessage(hWnd30, WM_SETUP, 0, 0); //初期化したゲーム設定のゲーム画面を構成する

			EnableWindow(hWndConnect, TRUE);
			EnableWindow(hWndConnectCancel, FALSE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndAcceptCancel, FALSE);


			// Screen41を非表示にする
			ShowWindow(hWndAgain41, SW_HIDE);
			ShowWindow(hWndTitle41, SW_HIDE);
			ShowWindow(hWnd41, SW_HIDE);

			// WM_SOCKETがScreen10に送られるように切り替える
			WSAAsyncSelect(sock, hWnd10, WM_SOCKET, FD_ACCEPT | FD_CONNECT | FD_READ | FD_CLOSE);

			// Screen10を表示する
			ShowWindow(hWndPlayer, SW_NORMAL);
			ShowWindow(hWndHost, SW_NORMAL);
			ShowWindow(hWndConnect, SW_NORMAL);
			ShowWindow(hWndAccept, SW_NORMAL);
			ShowWindow(hWndExplanation, SW_NORMAL);
			ShowWindow(hWnd10, SW_NORMAL);
			InvalidateRect(hWnd10, NULL, TRUE);
			EnableWindow(hWndAccept, TRUE);
			EnableWindow(hWndConnect, TRUE);

			///////////////////////////////////////////////////////////////////////////////////////

			return 0L;

		case IDB_NEXT:         // 次の試合へ
			char buf[MAX_MESSAGE];
			next1 = 1;

			InvalidateRect(hWnd41, NULL, TRUE);

			sprintf_s(buf, sizeof(buf), "NEXT");
			if (send(sock, buf, strlen(buf) + 1, 0) == SOCKET_ERROR) {    // 送信処理
			 // 送信に失敗したらエラーを表示
				MessageBox(hWnd41, TEXT("sending failed"), TEXT("Error"),
					MB_OK | MB_ICONEXCLAMATION);
			}
			else if (next2 == 1) {      // 相手も押していたら
				MessageBox(hWnd41, "次の試合を開始します", "Information", MB_OK | MB_ICONINFORMATION);
				round_num++;

				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_h_num + 1; j++) {
						vir_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num + 1; i++) {
					for (j = 0; j <= board_w_num; j++) {
						hor_line[i][j] = 0;
					}
				}
				for (i = 0; i <= board_h_num; i++) {
					for (j = 0; j <= board_w_num; j++) {
						square[i][j] = 0;
					}
				}

				//画面遷移 Screen41→Screen30///////////////////////////////////////////////////////

				//初期化する
				point_1 = 0;
				point_2 = 0;

				next1 = 0;  next2 = 0;  //遷移フラグ
				again1 = 0; again2 = 0;

				// Screen41を非表示にする
				ShowWindow(hWndNext, SW_HIDE);
				ShowWindow(hWnd41, SW_HIDE);

				// WM_SOCKETがScreen30に送られるように切り替える
				WSAAsyncSelect(sock, hWnd30, WM_SOCKET, FD_READ | FD_CLOSE);

				// Screen30を表示する
				ShowWindow(hWnd30, SW_NORMAL);
				InvalidateRect(hWnd30, NULL, TRUE);

				/////////////////////////////////////////////////////////////////

				return 0L;
			}
		}
		return 0L;


		//通信関連 //////////////////////////////////////////////////////////////////
	case WM_SOCKET:          // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0) { return 0L; }

		switch (WSAGETSELECTEVENT(lP)) {
		case FD_READ:       //メッセージ受信
			char buf[MAX_MESSAGE];                  // 受信内容を一時的に格納するバッファ
			if (recv(sock, buf, sizeof(buf) - 1, 0) != SOCKET_ERROR) { // 受信できたなら
				if (strcmp(buf, "NEXT") == 0) {

					next2 = 1;

					InvalidateRect(hWnd41, NULL, TRUE);

					//MessageBox(hWnd41, "次の試合へ進んでください", "Information", MB_OK | MB_ICONINFORMATION);

					if (next1 == 1) {     // 自分も押していたら
						MessageBox(hWnd41, "次の試合を開始します", "Information", MB_OK | MB_ICONINFORMATION);
						round_num++;

						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num + 1; j++) {
								vir_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num + 1; i++) {
							for (j = 0; j <= board_w_num; j++) {
								hor_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num; j++) {
								square[i][j] = 0;
							}
						}

						//画面遷移 Screen41→Screen30///////////////////////////////////////////////////////

						//初期化する
						point_1 = 0;
						point_2 = 0;

						next1 = 0;  next2 = 0;  //遷移フラグ
						again1 = 0; again2 = 0;

						// Screen41を非表示にする
						ShowWindow(hWndNext, SW_HIDE);
						ShowWindow(hWnd41, SW_HIDE);

						// WM_SOCKETがScreen30に送られるように切り替える
						WSAAsyncSelect(sock, hWnd30, WM_SOCKET, FD_READ | FD_CLOSE);

						// Screen30を表示する
						ShowWindow(hWnd30, SW_NORMAL);
						InvalidateRect(hWnd30, NULL, TRUE);

						/////////////////////////////////////////////////////////////////

						return 0L;
					}
				}

				if (strcmp(buf, "AGAIN") == 0) {

					again2 = 1;

					InvalidateRect(hWnd41, NULL, TRUE);

					if (again1 == 1) {     // 自分も押していたら

						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num + 1; j++) {
								vir_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num + 1; i++) {
							for (j = 0; j <= board_w_num; j++) {
								hor_line[i][j] = 0;
							}
						}
						for (i = 0; i <= board_h_num; i++) {
							for (j = 0; j <= board_w_num; j++) {
								square[i][j] = 0;
							}
						}

						//【画面遷移】Screen41→Screen20////////////////////////////

						//初期化する
						point_1 = 0;
						point_2 = 0;

						round_num = 1;  //phaseとround_modeとPlayer_colorは戻さない

						next1 = 0;  next2 = 0;  //遷移フラグ
						again1 = 0; again2 = 0;

						//Screen41を非表示にする
						ShowWindow(hWndAgain41, SW_HIDE);
						ShowWindow(hWndTitle41, SW_HIDE);
						ShowWindow(hWnd41, SW_HIDE);

						//SocketメッセージをhWnd20に送られるようにする
						WSAAsyncSelect(sock, hWnd20, WM_SOCKET, FD_READ | FD_CLOSE);

						//Screen20を表示させる
						ShowWindow(hWnd20, SW_NORMAL);
						ShowWindow(hWndPlayStart, SW_NORMAL);
						ShowWindow(hWndReject, SW_NORMAL);
						ShowWindow(hListVer, SW_NORMAL);
						ShowWindow(hListHol, SW_NORMAL);
						ShowWindow(hRadPlayer1, SW_NORMAL);
						ShowWindow(hRadPlayer2, SW_NORMAL);
						ShowWindow(hRadRound1, SW_NORMAL);
						ShowWindow(hRadRound5, SW_NORMAL);
						InvalidateRect(hWnd20, NULL, TRUE); //再描画

						/////////////////////////////////////////////////////////////

					}

					return 0L;

				}
			}
			return 0L;
		}

		//デフォルト処理/////////////////////////////////////////////////////////
	case WM_SETFOCUS:   // ウィンドウにフォーカスが来たら
		return 0L;

	case WM_DESTROY:    // ウィンドウが破棄された
		closesocket(sock);
		PostQuitMessage(0);
		return 0L;
	default:
		return DefWindowProc(hWnd41, uMsg, wP, lP);  // 標準メッセージ処理
	}/* end of switch (uMsg) */
	return 0L;
}


////////////////////////////////////////////////////////////////////////////////
//
//  ソケット初期化処理
//
BOOL SockInit(HWND hWnd)
{
	WSADATA wsa;
	int ret;
	char ret_buf[80];

	ret = WSAStartup(MAKEWORD(1, 1), &wsa);

	if (ret != 0)
	{
		wsprintf(ret_buf, "%d is the err", ret);
		MessageBox(hWnd, ret_buf, "Error", MB_OK | MB_ICONSTOP);
		exit(-1);
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  ソケット通信をする (クライアント側)
//
BOOL SockConnect(HWND hWnd, LPCSTR host)
{
	SOCKADDR_IN cl_sin; // SOCKADDR_IN構造体

	// ソケットを開く
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{ // ソケット作成失敗
		MessageBox(hWnd, "Socket() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	memset(&cl_sin, 0x00, sizeof(cl_sin)); // 構造体初期化
	cl_sin.sin_family = AF_INET;		   // インターネット
	cl_sin.sin_port = htons(PORT);		   // ポート番号指定

	phe = gethostbyname(host); // アドレス取得

	if (phe == NULL)
	{
		MessageBox(hWnd, "gethostbyname() failed.",
			"Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}
	memcpy(&cl_sin.sin_addr, phe->h_addr, phe->h_length);

	// 非同期モード (対戦する)
	if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_CONNECT) == SOCKET_ERROR)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
		MessageBox(hWnd, "WSAAsyncSelect() failed",
			"Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	// 対戦する処理
	if (connect(sock, (LPSOCKADDR)&cl_sin, sizeof(cl_sin)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
			MessageBox(hWnd, "connect() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  対戦待ち (サーバ側)
//
BOOL SockAccept(HWND hWnd)
{
	SOCKADDR_IN sv_sin; // SOCKADDR_IN構造体

	// サーバ用ソケット
	sv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sv_sock == INVALID_SOCKET)
	{ // ソケット作成失敗
		MessageBox(hWnd, "Socket() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	memset(&sv_sin, 0x00, sizeof(sv_sin));		// 構造体初期化
	sv_sin.sin_family = AF_INET;				// インターネット
	sv_sin.sin_port = htons(PORT);				// ポート番号指定
	sv_sin.sin_addr.s_addr = htonl(INADDR_ANY); // アドレス指定

	if (bind(sv_sock, (LPSOCKADDR)&sv_sin, sizeof(sv_sin)) == SOCKET_ERROR)
	{
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "bind() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	if (listen(sv_sock, 5) == SOCKET_ERROR)
	{
		// 対戦待ち失敗
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "listen() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	// 非同期処理モード (対戦待ち)
	if (WSAAsyncSelect(sv_sock, hWnd, WM_SOCKET, FD_ACCEPT) == SOCKET_ERROR)
	{
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "WSAAsyncSelect() failed",
			"Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}
	return FALSE;
}

LRESULT OnDrawItem(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT nIDCtl = UINT(wParam);
	LPDRAWITEMSTRUCT pds = LPDRAWITEMSTRUCT(lParam);

	// 該当ボタンでなければ戻る
	if (pds->CtlType != ODT_BUTTON || nIDCtl != IDB_PLAYSTART) // ID_BUTTON = ボタンID
	{
		return FALSE;
	}

	HDC hDC = pds->hDC;
	RECT r = pds->rcItem;

	/*
	// 好きな色で普通のボタン面を描く
	if (my_id == 1)
	{ //サーバー
		HBRUSH hBrush = CreateSolidBrush(Player1_Color);
	}
	else if (my_id == 2)
	{ //クライアント
		HBRUSH hBrush = CreateSolidBrush(Player2_Color);
	}
	*/

	HBRUSH hBrush = CreateSolidBrush(Player1_Color);

	FillRect(hDC, &r, hBrush);
	ButtonDraw(hDC, &r, FALSE);

	// フォーカス矩形の設定
	RECT rFocus = pds->rcItem;
	InflateRect(&rFocus, -4, -4);
	BOOL bDrawFocus = FALSE;

	// ボタンにフォーカスが当たっているかの判定
	if (pds->hwndItem == GetFocus())
	{
		// フォーカスが当たっている際の黒枠描画
		FrameRect(hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
		// ボタン矩形は1ピクセル縮まる
		InflateRect(&r, -1, -1);
		// 後で使うフォーカス有無のフラグ
		bDrawFocus = TRUE;
	}

	// ボタンが押されているかの判定
	if (pds->itemAction & ODA_SELECT && pds->itemState & ODS_SELECTED)
	{
		// 押されたボタン枠の描画
		ButtonDraw(hDC, &r, TRUE);
		// テキスト位置をずらす
		OffsetRect(&r, 1, 1);
	}
	else
	{
		// 普通のボタン枠
		ButtonDraw(hDC, &r, FALSE);
	}
	DeleteObject(hBrush);

	// フォーカス矩形の描画
	// XOR描画なので塗りつぶしの後に行う
	if (TRUE == bDrawFocus)
	{
		DrawFocusRect(hDC, &rFocus);
	}

	// ウィンドウテキストの取得
	TCHAR buf[64];
	int len = GetWindowText(pds->hwndItem, buf, 64);
	// テキストの描画
	int bkmode = SetBkMode(hDC, TRANSPARENT);

	//	ここでフォントを設定すれば任意のフォントとなる
	//	HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFontS);
	// SetTextColor(hDC, RGB(255,255,255));	// フォントの色

	DrawText(hDC, "ゲームを始める", strlen("ゲームを始める"), &r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	//	SelectObject(hDC, hOldFont);
	SetBkMode(hDC, bkmode);

	return TRUE;
}

void ButtonDraw(HDC hdc, RECT* pRectBtn, BOOL bPush)
{
	HPEN penBtn;
	if (FALSE != bPush)
	{
		// 黒ペン
		penBtn = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	}
	else
	{
		// 白ペン
		penBtn = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	}
	HPEN hOldPen = (HPEN)SelectObject(hdc, penBtn);
	// ボタンの上線
	MoveToEx(hdc, pRectBtn->left, pRectBtn->top, NULL);
	LineTo(hdc, pRectBtn->right - 1, pRectBtn->top);
	// ボタンの左線
	MoveToEx(hdc, pRectBtn->left, pRectBtn->top, NULL);
	LineTo(hdc, pRectBtn->left, pRectBtn->bottom - 1);
	SelectObject(hdc, hOldPen);
	DeleteObject(penBtn);

	if (FALSE != bPush)
	{
		// 白ペン
		HPEN penBtn = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
		HPEN hOldPen = (HPEN)SelectObject(hdc, penBtn);
		// ボタンの上線
		MoveToEx(hdc, pRectBtn->left + 1, pRectBtn->top + 1, NULL);
		LineTo(hdc, pRectBtn->right - 3, pRectBtn->top + 1);
		// ボタンの左線
		MoveToEx(hdc, pRectBtn->left + 1, pRectBtn->top + 1, NULL);
		LineTo(hdc, pRectBtn->left + 1, pRectBtn->bottom - 3);

		SelectObject(hdc, hOldPen);
		DeleteObject(penBtn);
	}

	//	HPEN penBtn;
	if (FALSE == bPush)
	{
		// 黒ペン
		penBtn = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	}
	else
	{
		// 白ペン
		penBtn = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	}
	hOldPen = (HPEN)SelectObject(hdc, penBtn);
	// ボタンの下線
	MoveToEx(hdc, pRectBtn->left, pRectBtn->bottom - 1, NULL);
	LineTo(hdc, pRectBtn->right - 1, pRectBtn->bottom - 1);
	// ボタンの右線
	MoveToEx(hdc, pRectBtn->right - 1, pRectBtn->top, NULL);
	LineTo(hdc, pRectBtn->right - 1, pRectBtn->bottom - 1);
	SelectObject(hdc, hOldPen);
	DeleteObject(penBtn);

	if (FALSE == bPush)
	{
		// 白ペン
		HPEN penBtn = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
		HPEN hOldPen = (HPEN)SelectObject(hdc, penBtn);
		MoveToEx(hdc, pRectBtn->left + 1, pRectBtn->bottom - 2, NULL);
		LineTo(hdc, pRectBtn->right - 2, pRectBtn->bottom - 2);
		MoveToEx(hdc, pRectBtn->right - 2, pRectBtn->top + 1, NULL);
		LineTo(hdc, pRectBtn->right - 2, pRectBtn->bottom - 2);
		SelectObject(hdc, hOldPen);
		DeleteObject(penBtn);
	}
}

LRESULT CALLBACK OnPaint(HWND hWnd, UINT uMsg, WPARAM wP, LPARAM lP)
{
	HDC hdc;
	PAINTSTRUCT ps;
	const int radius = RATIO_OF_DOT_CIRCLE_RADIUS * len_square; // 各点を表す丸（円）の半径
	const int margin = RATIO_OF_SQUARE_MARGIN * len_square;		// マスの中の角丸長方形とマスを表す正方形の間の余白の長さ

	hdc = BeginPaint(hWnd, &ps); // デバイスコンテキストハンドルをOSから得る

	// ボードの縦線を描画する
	for (int line = 0; line < board_h_num; line++)
	{
		for (int row = 0; row < board_w_num + 1; row++)
		{
			// 縦線[line][row]の値によって、線を引くペンを決める
			switch (vir_line[line][row])
			{
			case 0:
				SelectObject(hdc, hPenDot); // 破線の灰色のペンを選択
				break;
			case 1:
				SelectObject(hdc, hPenPlayer1); // プレイヤー1のペンを選択
				break;
			case 2:
				SelectObject(hdc, hPenPlayer2); // プレイヤー2のペンを選択
				break;
			default:
				MessageBox(hWnd, "drawing vertical line failed", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;
			}

			// 点[line][row]から点[line + 1][row]にかけて、縦線を描画
			MoveToEx(hdc, board_dot[line][row].x, board_dot[line][row].y, NULL);
			LineTo(hdc, board_dot[line + 1][row].x, board_dot[line + 1][row].y);
		}
	}

	// ボードの横線を描画する
	for (int line = 0; line < board_h_num + 1; line++)
	{
		for (int row = 0; row < board_w_num; row++)
		{
			// 横線[line][row]の値によって、線を引くペンを決める
			switch (hor_line[line][row])
			{
			case 0:
				SelectObject(hdc, hPenDot); // 破線の灰色のペンを選択
				break;
			case 1:
				SelectObject(hdc, hPenPlayer1); // プレイヤー1のペンを選択
				break;
			case 2:
				SelectObject(hdc, hPenPlayer2); // プレイヤー2のペンを選択
				break;
			default:
				MessageBox(hWnd, "drawing horizontal line failed", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;
			}

			// 点[line][row]から点[line][row + 1]にかけて、横線を描画
			MoveToEx(hdc, board_dot[line][row].x, board_dot[line][row].y, NULL);
			LineTo(hdc, board_dot[line][row + 1].x, board_dot[line][row + 1].y);
		}
	}

	SelectObject(hdc, hPenGray); // 灰色のペンを選択

	// 各点の丸（円）を描画
	for (int line = 0; line < board_dot_h_num; line++)
	{
		for (int row = 0; row < board_dot_w_num; row++)
		{
			// 楕円を描画する関数：引数（hdc, 左上X座標, 左上Y座標, 右下X座標, 右下Y座標）
			Ellipse(hdc, board_dot[line][row].x - radius, board_dot[line][row].y - radius, board_dot[line][row].x + radius, board_dot[line][row].y + radius);
		}
	}

	SelectObject(hdc, hPenDot); // 破線の灰色のペンを選択

	// マスを描画
	for (int line = 0; line < board_h_num; line++)
	{
		for (int row = 0; row < board_w_num; row++)
		{
			// マス[line][row]の中に、プレイヤーの色で角丸長方形を描画
			switch (square[line][row])
			{
			case 0:
				break;
			case 1:
				SelectObject(hdc, hPenPlayer1);	  // プレイヤー1のペンを選択
				SelectObject(hdc, hBrushPlayer1); // プレイヤー1のブラシを選択

				// 角丸長方形を描画する関数：引数（hdc, 左上X座標, 左上Y座標, 右下X座標, 右下Y座標, 角の楕円の幅, 角の楕円の高さ）
				RoundRect(hdc, board_dot[line][row].x + margin, board_dot[line][row].y + margin, board_dot[line + 1][row + 1].x - margin, board_dot[line + 1][row + 1].y - margin, radius, radius);
				break;
			case 2:
				SelectObject(hdc, hPenPlayer2);	  // プレイヤー2のペンを選択
				SelectObject(hdc, hBrushPlayer2); // プレイヤー2のブラシを選択

				// 角丸長方形を描画する関数：引数（hdc, 左上X座標, 左上Y座標, 右下X座標, 右下Y座標, 角の楕円の幅, 角の楕円の高さ）
				RoundRect(hdc, board_dot[line][row].x + margin, board_dot[line][row].y + margin, board_dot[line + 1][row + 1].x - margin, board_dot[line + 1][row + 1].y - margin, radius, radius);
				break;
			default:
				MessageBox(hWnd, "drawing square failed", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////

	// gc資源（ペン、ブラシ、領域）////////////

	// 25px, 太字
	hfon1 = CreateFont(30, 0,								 //高さ, 幅
		0, 0, FW_BOLD,						 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
		FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
		SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
		VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

	// 100px, 太字
	hfon100px = CreateFont(100, 0,								 //高さ, 幅
		0, 0, FW_BOLD,						 //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
		FALSE, FALSE, FALSE,					 // Italic,  下線,  打消し線
		SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //はみ出した場合の処理,  属性
		VARIABLE_PITCH | FF_ROMAN, NULL);	 //ピッチ、ファミリ,  タイプフェイスのポインタ

	hPenBlackDot = CreatePen(PS_DOT, 1, 0x000000);

	hBrushGray = CreateSolidBrush(0xABABAF);  //「ドット＆ボックス」用のブラシ

	///////////////////////////////////////////////////////////////////////////////////////////


	//タイトル・ラウンド数の表示///////////////////////////////////////////////////////////////

	//タイトルの表示
	hPenPrev = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
	hBrushPrev = (HBRUSH)SelectObject(hdc, hBrushGray);
	Rectangle(hdc, 235, 15, 460, 65);  //灰色の長方形を描画する
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	SelectObject(hdc, GetStockObject(NULL_BRUSH));

	hfonPrev = (HFONT)SelectObject(hdc, hfon1); // フォントを選択
	SetTextColor(hdc, 0xFFFFFF); //白
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, 255, 25, "ドット&ボックス", lstrlen("ドット&ボックス"));  //「ドット＆ボックス」白色


	//ROUND5dの表示
	Rectangle(hdc, 275, 90, 425, 140); //ROUND%dの枠

	SetTextColor(hdc, 0x000000); //黒
	if (round_mode == 1) {
		TextOut(hdc, 285, 100, "ROUND1", lstrlen("ROUND1"));
	}
	else if (round_mode == 3 || round_mode == 5) {
		char buf_round[16];
		sprintf_s(buf_round, "ROUND%d", round_num);
		TextOut(hdc, 285, 100, buf_round, lstrlen(buf_round));
	}


	//プレイヤー名の表示////////////////////////////////////////////////////////////////

	//Player1の名前
	char buf_name1[64];
	if (strlen(Player1_name) == 0) {
		sprintf_s(buf_name1, "Player1");
	}
	else
		sprintf_s(buf_name1, "%s", Player1_name);

	TextOut(hdc, 185, 160, buf_name1, lstrlen(buf_name1)); //名前を描画する

	//Player2の名前
	char buf_name2[64];
	if (strlen(Player2_name) == 0) {
		sprintf_s(buf_name2, "Player2");
	}
	else
		sprintf_s(buf_name2, "%s", Player2_name);

	TextOut(hdc, 375, 160, buf_name2, lstrlen(buf_name2)); //名前を描画する


	//名前の下の線の表示(今、どちらのphaseかを表す)////////////////////////

	if (phase == 1) {

		SelectObject(hdc, hPenPlayer1);
		MoveToEx(hdc, 185, 200, NULL);
		LineTo(hdc, 325, 200);

		SelectObject(hdc, GetStockObject(BLACK_PEN));
		MoveToEx(hdc, 515, 200, NULL);
		LineTo(hdc, 375, 200);

	}
	else if (phase == 2) {

		SelectObject(hdc, GetStockObject(BLACK_PEN));
		MoveToEx(hdc, 185, 200, NULL);
		LineTo(hdc, 325, 200);

		SelectObject(hdc, hPenPlayer2);
		MoveToEx(hdc, 515, 200, NULL);
		LineTo(hdc, 375, 200);

	}


	//Player1の点数を表示する////////////////////////////////////////////////
	SelectObject(hdc, hfon100px); //得点用の100pxフォント
	SelectObject(hdc, hPenPlayer1);			  // プレイヤー1のペンを選択
	RoundRect(hdc, 50, 75, 175, 200, 30, 30); // Player1
	char buf_point1[16];
	sprintf_s(buf_point1, "%d", point_1);
	SetTextColor(hdc, Player1_Color);

	int i;
	for (i = 0; buf_point1[i] != '\0'; ++i)  //点数の桁数を調べて、iに格納する
		;
	if (i == 2)
	{
		TextOut(hdc, 60, 90, buf_point1, lstrlen(buf_point1)); // 2桁
	}
	else if (i == 1)
	{
		TextOut(hdc, 85, 90, buf_point1, lstrlen(buf_point1)); // 1桁
	}


	//Player2の点数を表示する////////////////////////////////////////////////
	SelectObject(hdc, hPenPlayer2);			   // プレイヤー2のペンを選択
	RoundRect(hdc, 525, 75, 650, 200, 30, 30); // Player2
	char buf_point2[16];
	sprintf_s(buf_point2, "%d", point_2);
	SetTextColor(hdc, Player2_Color);

	int j;
	for (j = 0; buf_point2[j] != '\0'; ++j)  //点数の桁数を調べて、jに格納する
		;
	if (j == 2)
	{
		TextOut(hdc, 535, 90, buf_point2, lstrlen(buf_point2)); // 2桁
	}
	else if (j == 1)
	{
		TextOut(hdc, 560, 90, buf_point2, lstrlen(buf_point2)); // 1桁
	}


	//ゲージの表示///////////////////////////////////////////////////////

	SelectObject(hdc, GetStockObject(BLACK_PEN));
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, 50, 220, 650, 260); //ゲージの枠

	SelectObject(hdc, hPenBlackDot);
	MoveToEx(hdc, 350, 220, NULL);  //中央の縦線（破線）
	LineTo(hdc, 350, 260);

	//Player1のゲージを描画する
	SelectObject(hdc, GetStockObject(NULL_PEN));
	SelectObject(hdc, CreateSolidBrush(Player1_Color));
	Rectangle(hdc, 51, 221, 51 + ((650 - 50) * ((double)point_1 / (board_h_num * board_w_num))), 259);   //点数に応じたゲージの描画

	//Player2のゲージを描画する
	DeleteObject(SelectObject(hdc, CreateSolidBrush(Player2_Color)));
	Rectangle(hdc, 649 - ((650 - 50) * ((double)point_2 / (board_h_num * board_w_num))), 221, 649, 259);  //点数に応じたゲージの描画
	DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));



	//後処理////////////////////////////////////////////////////////////

	//元のペン・ブラシ・フォントに戻す
	SelectObject(hdc, hPenPrev);
	SelectObject(hdc, hBrushPrev);
	SelectObject(hdc, hfonPrev);
	SetTextColor(hdc, 0x000000); //黒

	// gc資源をDeleteする
	DeleteObject(hfon1);
	DeleteObject(hfon100px);
	DeleteObject(hBrushGray);
	DeleteObject(hPenBlackDot);

	////////////////////////////////////////////////////////////////////////

	EndPaint(hWnd, &ps); // 描画終了

	return 0L;
}

////////////////////////////////////////////////////////////////////////////////
//
//  引数の点がボードの各点のいずれかの周り（引数の半径の円の内部）にあるか判定する
//  引数の*linepと*rowpには、引数の点が点[line][row]の周りに存在する場合は、そのline、rowの値が入る。存在しない場合は-1が入る
//
BOOL judgeDotInCircle(DOT dot, int radius, int* linep, int* rowp)
{
	// *linepをline、*rowpをrowと表記する

	/* 「点[line][row]の座標 - 1/2 * マスの1辺の長さ <= 引数の点の座標 < 点[line][row]の座標 + 1/2 * マスの1辺の長さ」が成り立つような点[line][row]を探す
		※点[line][row]の座標 = 点の番号(row, line) * マスの1辺の長さ + 左上の点の座標
						↓
		(row, line) <= (引数の点の座標 - 左上の点の座標 + 1/2 * マスの1辺の長さ) / マスの1辺の長さ < (row+1, line+1)
		*/
	*rowp = (dot.x - top_left.x + len_square / 2) / len_square;
	*linep = (dot.y - top_left.y + len_square / 2) / len_square;

	// ボードの各点の列数と行数の制約条件
	if (*rowp < 0 || *rowp > board_dot_w_num || *linep < 0 || *linep > board_dot_h_num)
	{
		*rowp = -1;
		*linep = -1;
		return FALSE;
	}

	// 引数の点は点[line][row]を中心とする1辺の長さがlen_squareの正方形の領域内に存在する

	// 引数の点が判定範囲（点[line][row]を中心とした半径radiusの円）に存在するかを判定する
	if (getDist(dot, board_dot[*linep][*rowp]) > radius)
	{
		*rowp = -1;
		*linep = -1;
		return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// 2点間ABの距離を整数値で返す
//
int getDist(DOT a, DOT b)
{
	return (int)hypot(abs(a.x - b.x), abs(a.y - b.y));
}

////////////////////////////////////////////////////////////////////////////////
//
// フェイズを切り替える
//
void switchPhase(int* phase)
{
	if (*phase == 1)
		*phase = 2;
	else if (*phase == 2)
		*phase = 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// 引数の線[line][row]に関わるマスのみを判定し、マスが囲まれていた場合は、そのマスに値valをセットして更新する
// 返り値はhas_updated（更新したらTRUE、更新しなかったらFALSE）
//
BOOL updateSquare(BOOL is_vir_line, int line, int row, int val)
{
	BOOL has_updated = FALSE; // マスに値valをセットして更新したらTRUE（FALSEで初期化）

	// 縦線の場合
	if (is_vir_line)
	{
		// 引数の縦線[line][row]が引かれていない状態の場合、その左右のマスが新たに囲まれることはない
		if (vir_line[line][row])
		{

			// 縦線[line][row]がボードの左端の線(row = 0)ではなく、縦線の左のマスが囲まれている場合（左のマスの3本の線がいずれも0ではない場合）
			if (row * vir_line[line][row - 1] * hor_line[line][row - 1] * hor_line[line + 1][row - 1] != 0)
			{
				square[line][row - 1] = val;
				has_updated = TRUE;
			}

			// 縦線[line][row]がボードの右端の線ではなく、縦線の右のマスが囲まれている場合
			if (row != board_w_num && vir_line[line][row + 1] * hor_line[line][row] * hor_line[line + 1][row] != 0)
			{
				square[line][row] = val;
				has_updated = TRUE;
			}
		}
	}
	// 横線の場合
	else
	{
		// 引数の横線[line][row]が引かれていない状態の場合、その上下のマスが新たに囲まれることはない
		if (hor_line[line][row])
		{

			// 横線[line][row]がボードの上端の線(line = 0)ではなく、横線の上のマスが囲まれている場合（上のマスの3本の線がいずれも0ではない場合）
			if (line * hor_line[line - 1][row] * vir_line[line - 1][row] * vir_line[line - 1][row + 1] != 0)
			{
				square[line - 1][row] = val;
				has_updated = TRUE;
			}

			// 横線[line][row]がボードの下端の線ではなく、横線の下のマスが囲まれている場合
			if (line != board_h_num && hor_line[line + 1][row] * vir_line[line][row] * vir_line[line][row + 1] != 0)
			{
				square[line][row] = val;
				has_updated = TRUE;
			}
		}
	}

	return has_updated;
}

////////////////////////////////////////////////////////////////////////////////
//
// ボードのすべてのマスが塗られていたら、TRUEを返す。そうでなかったらFALSEを返す
//
BOOL is_board_full()
{
	for (int line = 0; line < board_h_num; line++)
	{
		for (int row = 0; row < board_w_num; row++)
		{
			if (square[line][row] == 0)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// ボードのマスのうち、引数の値をもつマスの個数を数える
// 返り値はret_count（値がvalのマスの個数）
//
int count_square(int val)
{
	int ret_count = 0; // 値がvalのマスの個数

	for (int line = 0; line < board_h_num; line++)
	{
		for (int row = 0; row < board_w_num; row++)
		{
			if (square[line][row] == val)
			{
				ret_count++;
			}
		}
	}

	return ret_count;
}