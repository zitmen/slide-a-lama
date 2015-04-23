/* Slide-a-lama solver - not a cheater! ;]
    by Martin Ovesny alias zitmen

	Mozna vylepseni - koncovka je zbytecne opatrna; vytezem je ten, kdo utece souperi o 300 a vice bodu.
	                ==> finta: pokud jsem na tahu a udelam tah, ktery me dostane pres 300b rozdil, tak ho udelam,
					           i kdyby to znamenalo, ze bych souperi v dalsim tahu nahral na nejaky vrazedny kombo
							   --> dalsi tah totiz diky vyhre uz nebude ;]
*/
#include <windows.h>
#include <tchar.h>

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

enum { SEVEN, BAR, CHERRY, PEAR, PLUM, BANANA, BELL, EMPTY };
enum { LEFT, TOP, RIGHT, ANY };

struct Cluster
{
	// [top,left] - leva horni pozice shluku (0-4)
	int top;
	int left;
	// [bottom, right] - prava dolni pozice shluku (0-4)
	int bottom;
	int right;	
	// type of tiles that is the cluster made of
	int tiles;
	// constructors
	Cluster() { clear(); }
	Cluster(int t, int l, int b, int r, int tile) { top = t; left = l; bottom = b; right = r; tiles = tile; }
	// methods
	int count() const;	// count of tiles in the cluster
	int eval() const;	// value of the cluster
	void clear();		// resets all values
};

HWND GetBoard(int board[5][5], int next[3]);
void InsertTile(int board[5][5], int tile, int direction, int row_col, int newboard[5][5]);
Cluster GetTheBestCluster(int board[5][5]);
void RemoveClusterFromBoard(int board[5][5], const Cluster &c);
int FindTheBestMove(int board[5][5], int tile, int &direction, int &row_col);
int FindSolution(int board[5][5], int next[3], int &direction, int &row_col);

int main()
{
	bool autochck;	// if true there will be checking periodically if the oponent already played his move
	string names[8] = { "SEV", "BAR", "CHE", "PEA", "PLU", "BAN", "BEL", "EMPTY" };
	string dir_names[4] = { "LEFT", "TOP", "RIGHT", "ANY" };
	string tile;
	int board[5][5];
	int upcoming[3];
	//
	// User interaction - initialize first tile by hand
	cout << "Autochecking [0/1]? ";
	cin >> autochck;
	upcoming[0] = EMPTY;
	cout << "\nAvailable tiles: ";
	for(int i = 0; i < 8; i++)
		cout << names[i] << ' ';
	cout << "\n\nWhat tile do you start with? ";
	cin >> tile;
	for(int i = 0; i < 7; i++)
		if(tile == names[i])
			upcoming[0] = i;
	//
	if(upcoming[0] == EMPTY)	// error!
	{
		cout << "\nGame over." << endl;
		return 1;
	}
	//
	while(1)
	{
		cout << "\n=========================================\n";
		//
		// Get Slide-a-lama game board
		HWND hGameWnd = GetBoard(board, upcoming);
		//
		// Show results
		cout << "\nGame board" << endl;
		for(int s = 0; s < 5; s++)
		{
			for(int r = 0; r < 5; r++)
				cout << names[board[r][s]] << ' ';
			cout << endl;
		}
		//
		cout << "\nUpcoming tiles: " << names[upcoming[0]];
		for(int i = 1; i < 3; i++)
			cout << ", " << names[upcoming[i]];
		cout << endl;
		//
		// Compute the best move
		Cluster c;
		int d = ANY, rc = -1;
		int score = FindSolution(board, upcoming, d, rc);
		cout << "\nMove direction     = " << dir_names[d]
			 << "\nMove row/col [1-5] = " << (rc + 1)
			 << "\nScore              = " << score << endl;
		//
		// Move cursor to the appropriate field in the game window and click - one move
		//
		SetForegroundWindow(hGameWnd);	// switch to the apropriate window
		SetFocus(hGameWnd);
		Sleep(500);	// wait - switching to flash window may be slow
		//
		RECT rect;
		GetWindowRect(hGameWnd, &rect);
		POINT mouse_point[3][5] =
		{
			{ { 130,  92 }, { 130, 151 }, { 130, 210 }, { 130, 269 }, { 130, 328 } },
			{ { 220,  20 }, { 270,  20 }, { 320,  20 }, { 370,  20 }, { 420,  20 } },
			{ { 500,  92 }, { 500, 151 }, { 500, 210 }, { 500, 269 }, { 500, 328 } }
		};
		SetCursorPos(rect.left + mouse_point[d][rc].x, rect.top + mouse_point[d][rc].y);	// maybe this could not be here ;]
		Sleep(500);	// wait a moment for the tile animation
		// clicks using DirectInput
		INPUT inputs[2];
		memset(inputs, 0, 2*sizeof(INPUT));
		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dx = mouse_point[d][rc].x;
		inputs[0].mi.dy = mouse_point[d][rc].y;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		inputs[1] = inputs[0];
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2, inputs, sizeof(INPUT));
		//
		// Set tile for the next move and wait for the next turn
		upcoming[0] = upcoming[2];
		HDC hDC = GetWindowDC(hGameWnd);
		// check if CTRL is still pressed - if it is then wait until it's released,
		// otherwise the next move would be skipped and other moves would be incorrect
		if(GetAsyncKeyState(VK_CONTROL))
		{
			cout << "\nRelease the CTRL key, please." << endl;
			do{ Sleep(500); } while(GetAsyncKeyState(VK_CONTROL));
		}
		//
		if(autochck)
		{
			cout << "\nIf the game is over, hold ESCAPE shortly." << endl;
			cout << "If the waiting gets stucked (it's already your turn and still waiting), hold CTRL shortly." << endl;
			cout << "Waiting for opponent's turn..." << endl;
			while((GetPixel(hDC,  10, 14) != 0xFFFFFF) && !GetAsyncKeyState(VK_CONTROL))	// then check if it's opponent's move
			{	// keep the game window on top and focused!
				SetForegroundWindow(hGameWnd);	// switch to the apropriate window
				SetFocus(hGameWnd);
				Sleep(500);	// wait - switching to flash window may be slow
				if(GetAsyncKeyState(VK_ESCAPE)) break;	// quit?
			}
			while((GetPixel(hDC, 477, 14) != 0xFFFFFF) && !GetAsyncKeyState(VK_CONTROL))	// then check if it's my turn
			{	// keep the game window on top and focused!
				SetForegroundWindow(hGameWnd);	// switch to the apropriate window
				SetFocus(hGameWnd);
				Sleep(500);	// wait - switching to flash window may be slow
				if(GetAsyncKeyState(VK_ESCAPE)) break;	// quit?
			}
		}
		else
		{
			cout << "\nIf the game is over, hold ESCAPE shortly." << endl;
			cout << "\nIf it's already your turn, hold CTRL shortly." << endl;
			while(!GetAsyncKeyState(VK_CONTROL))	// check if it's already my move
			{
				Sleep(500);
				if(GetAsyncKeyState(VK_ESCAPE)) break;	// quit?
			}
		}
		if(GetAsyncKeyState(VK_ESCAPE)) break;	// quit?
		cout << "My turn.\n" << endl;
	}
	cout << "\nGame over." << endl;
	return 0;
}

/* Ziska herni plochu
   ------------------
   board - hraci pole 5x5, jiz predalokovane, takze funkce ho jen vyplni
   next  - nasledujici 3 kostky --> vraci pouze 2. a 3., protoze prvni nelze urcit, jelikoz neni na pevne pozici

   Funkce vraci handle na okno hry.
*/
HWND GetBoard(int board[5][5], int next[3])
{
	//
    // Hraci pole 5x5 policek o rozmerech w=50xh=59.
    // Pocatecni pozice (levy horni roh) je [195,62].
    // => Prvni stredovy pixel je tedy na pozici [x=220,y=92].
    // => Dalsi stredove pixely jsou v posloupnosti [x+s*w,y+r*h], kde r radek < 5 a s sloupec < 5.
	// --> kostky ukazujici nahled dalsich tahu maji velikost w=36xh=42
	// --> 1. tah neanalyzuji, protoze se pohybuje nekde kolem hraci plochy
	// --> 2. tah je na pevne pozici [602,123] = stred kostky
	// --> 3. tah je na pevne pozici [618, 64] = stred kostky
    //
    // Barevne kody (RGB)
    // ==================
    // SEVEN  - #FF46B7
    // BAR    - #FFFFFF
    // CHERRY - #DA0000
    // PAIR   - #99CC00
    // PLUM   - #6666FF
    // BANANA - #FFCC00
    // BELL   - #BF7F00
    //
    int tiles [7] = { SEVEN, BAR, CHERRY, PEAR, PLUM, BANANA, BELL };
    int colors[7] = { 0xB746FF, 0xFFFFFF, 0x0000DA, 0x00CC99, 0xFF6666, 0x00CCFF, 0x007FBF };	// v BGR!
    //
	// Find game window
	HWND hAppWnd = FindWindow(NULL, _T("Slide-a-lama"));
	HWND hGameWnd = FindWindowEx(hAppWnd, NULL, NULL, _T(""));
	//
	// Get raster
	SetForegroundWindow(hGameWnd);
	SetFocus(hGameWnd);
	Sleep(500);	// wait - switching to flash window may be slow
	HDC hDC = GetWindowDC(hGameWnd);
	//
	// Get board fields
	COLORREF color;
	int min_index;
    int x = 220, y = 92, w = 50, h = 59;
    for(int r = 0; r < 5; r++)
    {
        for(int s = 0; s < 5; s++)
        {
			// is there a tile or not?
			color = GetPixel(hDC, x - 5 + r * w, y - 25 + s * h);
			// check for the color at the top margin of the tile - tiles has light bgcolor -- all BGR components are greater than 0x9F
			if(((color & 0xFF) > 0x9F) && (((color & 0xFF00) >> 8) > 0x9F) && (((color & 0xFF0000) >> 16) > 0x9F))
			{
				// get pixel
				color = GetPixel(hDC, x + r * w, y + s * h);
				// count colors differences
				min_index = -1;
				for(int i = 0, diff = 0, min_diff = INT_MAX; i < 7; i++, diff = 0)	// 7 je pocet barev/druhu dlazdic
				{
					diff += (int)abs((double) (colors[i] & 0xFF)            - ((color & 0xFF)));
					diff += (int)abs((double)((colors[i] & 0xFF00) >> 8)    - ((color & 0xFF00) >> 8));
					diff += (int)abs((double)((colors[i] & 0xFF0000) >> 16) - ((color & 0xFF0000) >> 16));
					if(min_diff > diff)
					{
						min_diff = diff;
						min_index = i;
						if(diff == 0) break;
					}
				}
				// what type of field is it?
				board[r][s] = tiles[min_index];
			}
			else
				board[r][s] = EMPTY;
        }
    }
	//
	// --> 2. tah [602,123]
	// get pixel
	color = GetPixel(hDC, 602, 123);
    // count colors differences
    min_index = -1;
    for(int i = 0, diff = 0, min_diff = INT_MAX; i < 7; i++, diff = 0)	// 7 je pocet barev/druhu dlazdic
    {
        diff += (int)abs((double) (colors[i] & 0xFF)            - ((color & 0xFF)));
        diff += (int)abs((double)((colors[i] & 0xFF00) >> 8)    - ((color & 0xFF00) >> 8));
        diff += (int)abs((double)((colors[i] & 0xFF0000) >> 16) - ((color & 0xFF0000) >> 16));
        if(min_diff > diff)
        {
            min_diff = diff;
            min_index = i;
			if(diff == 0) break;
        }
    }
    // what type of field is it?
    next[1] = tiles[min_index];
	//
	// --> 3. tah [618, 64]
	// get pixel
	color = GetPixel(hDC, 618, 64);
    // count colors differences
    min_index = -1;
    for(int i = 0, diff = 0, min_diff = INT_MAX; i < 7; i++, diff = 0)	// 7 je pocet barev/druhu dlazdic
    {
        diff += (int)abs((double) (colors[i] & 0xFF)            - ((color & 0xFF)));
        diff += (int)abs((double)((colors[i] & 0xFF00) >> 8)    - ((color & 0xFF00) >> 8));
        diff += (int)abs((double)((colors[i] & 0xFF0000) >> 16) - ((color & 0xFF0000) >> 16));
        if(min_diff > diff)
        {
            min_diff = diff;
            min_index = i;
			if(diff == 0) break;
        }
    }
    // what type of field is it?
    next[2] = tiles[min_index];
	//
	return hGameWnd;
}

/* Vlozi jednu dlazdici na hraci plochu
   ------------------------------------
   board     - vyplnene hraci pole 5x5
   tile      - druh dlazdice, ktery bude vlozen
   direction - smer, ze ktereho bude dlazdice vlozena (LEFT, TOP, RIGHT)
   row_col   - cislo sloupce/radku, do ktereho bude dlazdice vlozena (sloupec se pocita shora(0) dolu(4), radek se pocita zleva(0) doprava(4))
   newboard  - timto argumentem funkce vraci novou herni desku 5x5 s nove vlozenou dlazdici.
*/
void InsertTile(int board[5][5], int tile, int direction, int row_col, int newboard[5][5])
{
	// Fill the new board with original data
	for(int i = 0; i < 5; i++)
		for(int j = 0; j < 5; j++)
			newboard[i][j] = board[i][j];
	//
	switch(direction)
	{
		case LEFT:
			for(int i = 1; i <= 4; i++)
			{
				if(board[i-1][row_col] == EMPTY)
					break;
				newboard[i][row_col] = board[i-1][row_col];
			}
			newboard[0][row_col] = tile;
			break;

		case RIGHT:
			for(int i = 3; i >= 0; i--)
			{
				if(board[i+1][row_col] == EMPTY)
					break;
				newboard[i][row_col] = board[i+1][row_col];
			}
			newboard[4][row_col] = tile;
			break;

		case TOP:
			for(int i = 1; i <= 4; i++)
			{
				if(board[row_col][i-1] == EMPTY)
					break;
				newboard[row_col][i] = board[row_col][i-1];
			}
			newboard[row_col][0] = tile;
			break;
	}
	// gravity
	for(int i = 0; i < 5; i++)
	{
		for(int j = 4; j >= 0; j--)
		{
			for(int r = 0; (r < 5) && (newboard[i][j] == EMPTY); r++)
			{
				for(int k = j; k > 0; k--)
					newboard[i][k] = newboard[i][k-1];
				newboard[i][0] = EMPTY;
			}
		}
	}
}

int Cluster::count() const
{
	int horz = right - left + 1;
	int vert = bottom - top + 1;
	return horz * vert;
}

int Cluster::eval() const
{
/***********************
	+-----------+---+
	|SEVEN		|150|
	|BAR		|100|
	|CHERRY		| 70|
	|PAIR		| 40|
	|PLUM		| 30|
	|BANNANA	| 20|
	|BELL		| 10|
	+-----------+---+
	|4 of kind	| x2|
	|5 of kind	| x3|
	+-----------+---+
************************/
	static int values[7] = { 150, 100, 70, 40, 30, 20, 10 };
	int count = this->count();
	//
	if((tiles < SEVEN) || (tiles > BELL)) return 0;
	if((count < 3    ) || (count > 5   )) return 0;
	//
	return (values[tiles] * (count - 2));
}

void Cluster::clear()
{
	top = bottom = left = right = -1;
	tiles = EMPTY;
}

//
// true moves the element to the front, false moves the element to the back
bool ClustersComparator(const Cluster *c1, const Cluster *c2)
{
	int c1_val = c1->eval(), c2_val = c2->eval();
	//
	if(c1_val > c2_val) return true;
	if(c1_val < c2_val) return false;
	if(c1->count() > c2->count()) return true;
	return false;
}

/* Projde hraci plochu a najde nejvetsi skupinu stejnych dlazdic
   -------------------------------------------------------------
   board           - vyplnene hraci pole 5x5

   Funkce vraci shluku s nejvetsim moznym bodovym ziskem.
*/
Cluster GetTheBestCluster(int board[5][5])
{
	vector<Cluster *> clusters;
	// Hledani ve vertikalnim smeru
	for(int i = 0; i < 5; i++)
	{
		int size = 1;
		for(int j = 1; j < 5; j++)
		{
			if((board[i][j] != EMPTY) && (board[i][j-1] == board[i][j]))
				size++;
			else if(size >= 3)
			{
				clusters.push_back(new Cluster(j - size, i, j - 1, i, board[i][j-1]));
				size = 1;
			}
			else
				size = 1;
		}
		if(size >= 3)
			clusters.push_back(new Cluster(5 - size, i, 4, i, board[i][4]));
	}
	// Hledani v horizontalnim smeru
	for(int j = 0; j < 5; j++)
	{
		int size = 1;
		for(int i = 1; i < 5; i++)
		{
			if((board[i][j] != EMPTY) && (board[i-1][j] == board[i][j]))
				size++;
			else if(size >= 3)
			{
				clusters.push_back(new Cluster(j, i - size, j, i - 1, board[i-1][j]));
				size = 1;
			}
			else
				size = 1;
		}
		if(size >= 3)
			clusters.push_back(new Cluster(j, 5 - size, j, 4, board[4][j]));
	}
	//
	Cluster c;
	if(clusters.size() > 0)
	{
		sort(clusters.begin(), clusters.end(), ClustersComparator);
		c = *(clusters.front());
		//
		for(size_t i = 0; i < clusters.size(); i++)
			delete clusters[i];
		clusters.clear();
	}
	//
	return c;
}

/* Odstrani shluk dlazdic z herniho pole
   -------------------------------------
   board - vyplnene hraci pole 5x5 -- zaroven se v nem vraci jako vysledek nove hraci pole po odstraneni shluku
*/
void RemoveClusterFromBoard(int board[5][5], const Cluster &c)
{
	int horz = c.right - c.left + 1, vert = c.bottom - c.top + 1;
	if(horz > 1)	// is the cluster horizontal?
	{
		// tiles have to be moved verticaly
		for(int j = c.left; j <= c.right; j++)
		{
			for(int i = c.top; i > 0; i--)
				board[j][i] = board[j][i-1];
			//
			board[j][0] = EMPTY;
		}
	}
	else	// (vert > 1) --> no, it's vertical
	{
		// tiles have to be moved verticaly
		for(int i = 0; i < c.top; i++)
			board[c.left][i+vert] = board[c.left][i];
		for(int i = 0; i < vert; i++)
			board[c.left][i] = EMPTY;
	}
}

/* Najde nejlepsi mozny tah (tzn. zisk maxima bodu)
   ------------------------------------------------
   board - vyplnene hraci pole 5x5
   next - nasledujici 3 dlazdice
   depth - hloubka rekurze - index tahu/dlazdice [0-2]
   direction - vraci smer (LEFT, RIGHT, TOP) nejlepsiho tahu
   row_col - vraci radek/sloupec (0-4) nejlepsiho tahu

   Funkce vraci skore tahu, coz je pocet bodu, ktery hrac ziska nejlepsim moznym tahem,
   a to vcetne odecteni skore, ktere ziska protihrac nejlepsim moznym protitahem.
   Nejlepsi tah znamena maximalizovani zisku bodu hrace a minimalizovani zisku bodu soupere.

   Herni deska se neaktualizuje, protoze to pro dalsi zpracovani nepotrebuju.
*/
int FindTheBestMove(int board[5][5], int next[3], int depth, int &direction, int &row_col)
{
	Cluster c;
	int score_best = INT_MIN, points, score_move, opponent_dir, opponent_rowcol;
	for(int rc = 0; rc < 5; rc++)
	{
		for(int d = 0; d < 3; d++)	// LEFT, RIGHT, TOP
		{
			int board_tmp[5][5];
			InsertTile(board, next[depth], d, rc, board_tmp);
			c = GetTheBestCluster(board_tmp);
			points = c.eval();
			score_move = points;
			while(points > 0)	// check for combos
			{
				RemoveClusterFromBoard(board_tmp, c);
				c = GetTheBestCluster(board_tmp);
				points = c.eval();
				score_move += points;
			}
			// opponent's move
			if(depth < 2)
				score_move -= FindTheBestMove(board_tmp, next, depth + 1, opponent_dir, opponent_rowcol);
			// 
			if(score_move > score_best)
			{
				score_best = score_move;
				direction = d;
				row_col = rc;
			}
		}
	}
	//
	return score_best;	// points gained by the best move
}

/* Vyzkousi vsechny mozne kombinace tahu tak, aby byl nalezen nejlepsi mozny tah
   -----------------------------------------------------------------------------
   board - vyplnene hraci pole 5x5
   next - nasledujici 3 dlazdice
   direction - vraci smer (LEFT, RIGHT, TOP) nejlepsiho tahu
   row_col - vraci radek/sloupec (0-4) nejlepsiho tahu

   Funkce vraci skore tahu nejlepsiho tahu.
   Funkce interne vola FindTheBestMove, coz je rekurzivni funkce.

   Herni deska se neaktualizuje, protoze to pro dalsi zpracovani nepotrebuju.
*/
inline int FindSolution(int board[5][5], int next[3], int &direction, int &row_col)
{
	return FindTheBestMove(board, next, 0, direction, row_col);
}
