
/*

MIT License

Copyright (c) 2019 ouuan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <algorithm> 
#include "AIController.h"
#include <utility>

using namespace std;

typedef long long ll;

const int DEPTH = 12;
const int FIRST_THREE_DEPTH = 6;

const int WIDTH[DEPTH + 1] = {0, 0, 3, 3, 3, 3, 3, 4, 4, 5, 6, 7, 9};
const int FIRST_THREE_WIDTH[FIRST_THREE_DEPTH + 1] = {0, 0, 3, 3, 4, 5, 7};

const ll WEIGHT[4][2] = {
	{3, 1},
	{1000, 3},
	{1000000, 1000},
	{10000000000ll, 1000000}
};

const int N = 15;
const int M = 15;

struct Coordinate
{
	int x, y, w;
	
	Coordinate(int _x = 0, int _y = 0): x(_x), y(_y), w(min(_x + 1, N - _x) * min(_y + 1, M - _y)) {}
	
	Coordinate operator+(const Coordinate& b) const
	{
		return Coordinate(x + b.x, y + b.y);
	}
	Coordinate operator-(const Coordinate& b) const
	{
		return Coordinate(x - b.x, y - b.y);
	}
	Coordinate operator*(int b) const
	{
		return Coordinate(x * b, y * b);
	}
	friend Coordinate operator*(int a, const Coordinate& b)
	{
		return b * a;
	}
	
	bool operator==(const Coordinate& b) const
	{
		return x == b.x && y == b.y;
	}
	
	bool operator<(const Coordinate& b) const // used for set sorting
	{
		return w == b.w ? (x == b.x ? y < b.y : x < b.x) : w < b.w;
	}
};

const Coordinate dir[4] = {Coordinate(0, 1), Coordinate(1, 0), Coordinate(1, 1), Coordinate(1, -1)};

struct Status
{
	int l, r; // [coor - d * l, coor + d * r] are color
	bool lblank, rblank; // is coor - d * (l + 1) / coor + d * (r + 1) blank
	
	Status(): l(0), r(0), lblank(false), rblank(false) {}
	
	int len() const { return l + r + 1; }
	
	int bl() const { return l + lblank; }
	
	int br() const { return r + rblank; }
	
	ll w() const // the value of the status
	{
		if (len() >= 5) return 1e15;
		if (!lblank && !rblank) return 0;
		return WEIGHT[len() - 1][lblank ^ rblank];
	}
};

struct Blank
{
	Coordinate coor;
	ll w;
	
	Blank(Coordinate _coor, ll _w): coor(_coor), w(_w) {}
	
	bool operator<(const Blank& b) const
	{
		return w == b.w ? b.coor < coor : w > b.w;
	}
};

class Board
{
private:
	
	int out_of_board = -1; // declare this because the function a(coor) returns a reference
	vector<vector<int> > board; // the board, 0 for self, 1 for opponent, 2 for blank
	vector<vector<vector<vector<Status> > > > status; // the status of connected blocks in four directions in two colors
	set<Blank> blank; // blanks sorted by max(weight for black, weight for white)
	
	int& a(const Coordinate& coor) // get color of coor
	{
		if (coor.x < 0 || coor.x >= N || coor.y < 0 || coor.y >= M) return out_of_board;
		return board[coor.x][coor.y];
	}
	
	vector<vector<Status> >& s(const Coordinate& coor) // get status of coor
	{
		return status[coor.x][coor.y];
	}
	
	ll calc(const Coordinate& coor, int color) // get weight for coor for color
	{
		return s(coor)[0][color].w() + s(coor)[1][color].w() + s(coor)[2][color].w() + s(coor)[3][color].w();
	}
	
	void update(const Coordinate& coor, int direction) // update the status of coor in direction
	{
		if (a(coor) == out_of_board) return;
		if (a(coor) == 2) blank.erase(Blank(coor, max(calc(coor, 0), calc(coor, 1))));
		Coordinate d = dir[direction];
		for (int j = 0; j < 2; ++j)
		{
			Status & cur = s(coor)[direction][j];
			for (cur.l = 0; a(coor - (cur.l + 1) * d) == j; ++cur.l);
			for (cur.r = 0; a(coor + (cur.r + 1) * d) == j; ++cur.r);
			cur.lblank = a(coor - (cur.l + 1) * d) == 2;
			cur.rblank = a(coor + (cur.r + 1) * d) == 2;
		}
		if (a(coor) == 2) blank.insert(Blank(coor, max(calc(coor, 0), calc(coor, 1))));
	}
	
	void modify(const Coordinate& coor, int color) // modify the color of coor
	{
		if (a(coor) == out_of_board) return;
		if (a(coor) == 2) blank.erase(Blank(coor, max(calc(coor, 0), calc(coor, 1))));
		if (color == 2) blank.insert(Blank(coor, max(calc(coor, 0), calc(coor, 1))));
		a(coor) = color;
		for (int i = 0; i < 4; ++i)
		{
			update(coor, i);
			Coordinate d = dir[i];
			vector<Status> & cur = s(coor)[i];
			for (int j = -max(cur[0].bl(), cur[1].bl()); j <= cur[0].br() || j <= cur[1].br(); ++j)
			{
				if (j)
				{
					update(coor + j * d, i);
				}
			}
		}
	}
	
	Blank solve(int color, int k, const int* width, ll low = -1e18, ll high = 1e18) // next turn is color, k steps remaining, low and high for alpha-beta pruning
	{
		if (k == 1 || blank.size() == 1) return Blank(blank.begin()->coor, (k + 10) * calc(blank.begin()->coor, color));
		
		ll mx = -1e18;
		Coordinate choose;
		auto it = blank.begin();
		
		for (int i = 0; i < width[k] && i < int(blank.size()); ++i, ++it)
		{
			Coordinate cur = it->coor;
			ll tmp = calc(cur, color) * (k + 10);
			if (tmp < 1e16)
			{
				modify(cur, color);
				tmp -= solve(color ^ 1, k - 1, width, tmp - high, tmp - low).w;
				modify(cur, 2);
				it = blank.find(Blank(cur, max(calc(cur, 0), calc(cur, 1))));
			}
			if (tmp > mx)
			{
				mx = tmp;
				choose = cur;
			}
			low = max(low, mx);
			if (low >= high) break; // alpha-beta pruning
		}
		
		return Blank(choose, mx);
	}

public:
	Board(): board(N, vector<int>(M, 2)),
		status(N, vector<vector<vector<Status> > >(M, vector<vector<Status> >(4, vector<Status>(2))))
	{
		for (int i = 0; i < N; ++i)
		{
			for (int j = 0; j < M; ++j)
			{
				for (int d = 0; d < 4; ++d)
				{
					update(Coordinate(i, j), d);
				}
			}
		}
	}
	
	void modify(int x, int y, int color)
	{
		if (x == -1 && y == -1)
		{
			for (int i = 0; i < N; ++i)
			{
				for (int j = 0; j < M; ++j)
				{
					if (board[i][j] != 2)
					{
						modify(i, j, board[i][j] ^ 1);
					}
				}
			}
		}
		else modify(Coordinate(x, y), color);
	}

	Coordinate turn()
	{
		return solve(0, DEPTH, WIDTH).coor;
	}
	
	Coordinate second()
	{
		ll mx = -1;
		Coordinate ret;
		for (int i = 0; i < N; ++i)
		{
			for (int j = 0; j < M; ++j)
			{
				if (board[i][j] == 2)
				{
					modify(i, j, 0);
					ll res = abs(solve(1, FIRST_THREE_DEPTH, FIRST_THREE_WIDTH).w);
					if (res > mx)
					{
						mx = res;
						ret = Coordinate(i, j);
					}
					modify(i, j, 2);
				}
			}
		}
		return ret;
	}
	
	Coordinate is_change()
	{
		auto res = solve(0, DEPTH, WIDTH);
		if (res.w > 0) return res.coor;
		else return Coordinate(-1, -1);
	}
	
	Coordinate first_two_black()
	{
		ll mn = 1e18;
		Coordinate ret;
		for (int i = 0; i < N; ++i)
		{
			for (int j = 0; j < M; ++j)
			{
				if (board[i][j] == 2)
				{
					modify(i, j, 0);
					ll res = abs(solve(1, FIRST_THREE_DEPTH, FIRST_THREE_WIDTH).w);
					if (res < mn)
					{
						mn = res;
						ret = Coordinate(i, j);
					}
					modify(i, j, 2);
				}
			}
		}
		return ret;
	}
};

extern int ai_side; //0: black, 1: white
std::string ai_name = "BASELINE";
std::vector<std::pair<int,int>> other, mine;

int turnID;
Board board;

void init() {
    turnID = 0;
}

std::pair<int, int> action(std::pair<int, int> loc) {
    board = Board();
    for (int i = 0; i < turnID; i++) {
        // std::cerr << other[i].first << " " << other[i].second << " " << mine[i].first << " " << mine[i].second << std::endl;
        board.modify(other[i].first, other[i].second, 1);
        board.modify(mine[i].first, mine[i].second, 0);
    }
    board.modify(loc.first, loc.second, 1);
    other.push_back(loc);

    Coordinate res;
	if (turnID <= 1 && ai_side == 0)
		res = board.first_two_black();
    else if (turnID == 0)
	    res = board.second();
	else if (turnID == 1)
	    res = board.is_change();
	else
	    res = board.turn();

    turnID++;
    mine.emplace_back(res.x, res.y);

    return std::make_pair(res.x, res.y);
}
