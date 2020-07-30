#include "AIController.h"
#include <utility>
#include <cstring>
#include<set>
#include<algorithm>

extern int ai_side; //0: black, 1: white
std::string ai_name = "your_ai_name_here";

int turn = 0;
int board[15][15];
bool is_swap;


int points[7] = { 0, 0, 10, 100, 1000, 10000, 100000 };
long long scores[2];
int THREE_LIVE = 0, FOUR_BLOCK = 0, FOUR_LIVE = 0;
int DEPTH = 6;
int r[2], c[2];

struct Coordinate
{
    int x, y;
    long long weight;
    long long scores[2];

    Coordinate(const int& _x, const int& _y, long long* _scores = NULL, const long long& _w = -1e18) : x(_x), y(_y), weight(_w)
    {
        if (_scores != NULL) {
            scores[0] = _scores[0];
            scores[1] = _scores[1];
        }
    }

    bool operator<(const Coordinate& other) const
    {
        return weight == other.weight ? other.x < x : weight > other.weight;
    }

    bool operator==(const Coordinate& other) const
    {
        return x == other.x && y == other.y;
    }
};

void swap_scores(long long* scores);
void update_generator(int* r, int* c, const int& x, const int& y);
long long solve(int steps, const int& color, int* _r, int* _c, int& x, int& y, long long* _scores, long long high);
long long update_point(const int& x, const int& y, const int& color, long long* scores);
void sort_point(int* _r, int* _c, int color, long long* _scores, std::set<Coordinate>& generator_sort);

//init function is called once at the beginning
void init() {
    /* TODO: Replace this by your code */

    scores[0] = scores[1] = 0;
    r[0] = c[0] = 14, r[1] = c[1] = 0;
    is_swap = false;

    memset(board, -1, sizeof(board));
}

// loc is the action of your opponent
// Initially, loc being (-1,-1) means it's your first move
// If this is the third step(with 2 black ), where you can use the swap rule, your output could be either (-1, -1) to indicate that you choose a swap, or a coordinate (x,y) as normal.

std::pair<int, int> action(std::pair<int, int> loc) {
    /* TODO: Replace this by your code */
    /* This is now a random strategy */
    turn++;
    int x = loc.first, y = loc.second;

    if (x == -1 && y == -1) {
        if (turn == 1) {
            board[7][7] = ai_side;
            r[0] = c[0] = 4;
            r[1] = c[1] = 10;
            scores[0] = points[2];

            return std::make_pair(7, 7);
        }

        if (turn == 3) {
            swap_scores(scores);
            for (int i = 0; i < 15; ++i)
                for (int j = 0; j < 15; ++j)
                    if (board[i][j] != -1)
                        board[i][j] = board[i][j] ^ 1;

            solve(DEPTH, ai_side, r, c, x, y, scores, -1e18);
            board[x][y] = ai_side;
            update_point(x, y, ai_side, scores);
            update_generator(r, c, x, y);

            return std::make_pair(x, y);
        }
    }



    board[x][y] = 1 ^ ai_side;
    update_point(x, y, 1 ^ ai_side, scores);
    update_generator(r, c, x, y);



    if (turn == 2 && ai_side == 1 && !is_swap) {
        is_swap = true;
        swap_scores(scores);
        for (int i = 0; i < 15; ++i)
            for (int j = 0; j < 15; ++j)
                if (board[i][j] != -1)
                    board[i][j] = board[i][j] ^ 1;

        return std::make_pair(-1, -1);

    }
    else {
        solve(DEPTH, ai_side, r, c, x, y, scores, -1e18);
        board[x][y] = ai_side;
        update_point(x, y, ai_side, scores);
        update_generator(r, c, x, y);

        return std::make_pair(x, y);
    }
}




void swap_scores(long long* scores)
{
    long long tmp;
    tmp = scores[0];
    scores[0] = scores[1];
    scores[1] = tmp;
}


void update_generator(int* r, int* c, const int& x, const int& y)
{
    r[0] = ((x - 3 > 0) ? (x - 3 < r[0] ? x - 3 : r[0]) : 0);
    r[1] = ((x + 3 < 14) ? (x + 3 >= r[1] ? x + 3 : r[1]) : 14);
    c[0] = ((y - 3 > 0) ? (y - 3 < c[0] ? y - 3 : c[0]) : 0);
    c[1] = ((y + 3 < 14) ? (y + 3 >= c[1] ? y + 3 : c[1]) : 14);
}


long long solve(int steps, const int& color, int* _r, int* _c, int& x, int& y, long long* _scores, long long high)
{
    int r[2], c[2];
    long long s, ms = -1e18;
    long long scores[2];
    std::set<Coordinate> generator_sort;

    sort_point(_r, _c, color, _scores, generator_sort);

    auto it = generator_sort.begin();
    for (int k = 0; /*k < 10 &&*/ k < int(generator_sort.size()); ++k, ++it) {
        int i = it->x, j = it->y;
        board[i][j] = color;

        r[0] = (i - 3 > 0 ? (i - 3 < _r[0] ? i - 3 : _r[0]) : 0);
        r[1] = (i + 3 < 14 ? (i + 3 >= _r[1] ? i + 3 : _r[1]) : 14);
        c[0] = (j - 3 > 0 ? (j - 3 < _c[0] ? j - 3 : _c[0]) : 0);
        c[1] = (j + 3 < 14 ? (j + 3 >= _c[1] ? j + 3 : _c[1]) : 14);

        
        if (steps > 1) {
            scores[0] = it->scores[0], scores[1] = it->scores[1];
            s = solve(steps - 1, color ^ 1, r, c, x, y, scores, ms);
        }
        else s = it->weight;

        board[i][j] = -1;


        if (s > ms) {
            ms = s;
            if (steps == DEPTH) { x = i; y = j; }
        }

        if (-s <= high) break;


    }
    return -ms;
}


long long update_point(const int& x, const int& y, const int& color, long long* scores)
{
    const int d[4][2] = { 0,1,1,1,1,0,1,-1 };
    bool lblank = false, rblank = false;
    int l = 0, r = 0;

    for (int i = 0; i < 4; ++i) {
        for (l = 0; x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15; ++l)
            if (board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] != color)
                break;

        for (r = 0; x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15; ++r)
            if (board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] != color)
                break;

        lblank = ((x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15) && board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] == -1);
        rblank = ((x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15) && board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] == -1);


        if (lblank)  scores[color] -= points[l + 1];
        else scores[color] -= points[l];

        if (rblank)  scores[color] -= points[r + 1];
        else scores[color] -= points[r];

        if (l + r + 1 >= 5) scores[color] += points[6];
        else {
            if (!lblank && !rblank) scores[color] += points[0];
            else scores[color] += points[l + r + lblank + rblank];
        }

        if (l == 0 && !lblank) {
            for (l = 1; x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15; ++l)
                if (board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] != (color ^ 1))
                    break;

            lblank = ((x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15) && board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] == -1);

            if (lblank) { scores[1 ^ color] -= points[l + 1]; scores[1 ^ color] += points[l]; }
            else { scores[1 ^ color] -= points[l]; scores[color ^ 1] += points[0]; }

        }

        if (r == 0 && !rblank) {
            for (r = 1; x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15; ++r)
                if (board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] != (color ^ 1))
                    break;

            rblank = ((x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15) && board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] == -1);

            if (rblank) { scores[1 ^ color] -= points[r + 1]; scores[1 ^ color] += points[r]; }
            else { scores[1 ^ color] -= points[r]; scores[color ^ 1] += points[0]; }

        }
    }
    //算杀暂未实现

    return scores[color] - scores[color ^ 1];
}

void sort_point(int* _r, int* _c, int color, long long* _scores, std::set<Coordinate>& generator_sort)
{
    const int d[4][2] = { 0,1,1,1,1,0,1,-1 };
    bool lblank = false, rblank = false;
   
    int l = 0, r = 0;
    long long scores[2];

    for (int x = _r[0]; x <= _r[1]; ++x)
        for (int y = _c[0]; y <= _c[1]; ++y) if (board[x][y] == -1) {
            scores[0] = _scores[0], scores[1] = _scores[1];

            for (int i = 0; i < 4; ++i) {
                bool lchange = false, rchange = false;

                for (l = 0; x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15; ++l)
                    if (board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] != color)
                        break;

                for (r = 0; x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15; ++r)
                    if (board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] != color)
                        break;

                lblank = ((x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15) && board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] == -1);
                rblank = ((x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15) && board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] == -1);


                if (lblank)  scores[color] -= points[l + 1];
                else scores[color] -= points[l];

                if (rblank)  scores[color] -= points[r + 1];
                else scores[color] -= points[r];

                if (l + r + 1 >= 5) scores[color] += points[6];
                else {
                    if (!lblank && !rblank) scores[color] += points[0];
                    else scores[color] += points[l + r + lblank + rblank];
                }




                if (l == 0 && !lblank) {
                    for (l = 1; x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15; ++l)
                        if (board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] != (color ^ 1))
                            break;

                    lblank = ((x - d[i][0] * (l + 1) >= 0 && y - d[i][1] * (l + 1) >= 0 && y - d[i][1] * (l + 1) < 15) && board[x - d[i][0] * (l + 1)][y - d[i][1] * (l + 1)] == -1);

                    if (lblank) { scores[1 ^ color] -= points[l + 1]; scores[1 ^ color] += points[l]; }
                    else { scores[1 ^ color] -= points[l]; scores[color ^ 1] += points[0]; }

                    lchange = true;

                }

                if (r == 0 && !rblank) {
                    for (r = 1; x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15; ++r)
                        if (board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] != (color ^ 1))
                            break;

                    rblank = ((x + d[i][0] * (r + 1) < 15 && y + d[i][1] * (r + 1) >= 0 && y + d[i][1] * (r + 1) < 15) && board[x + d[i][0] * (r + 1)][y + d[i][1] * (r + 1)] == -1);

                    if (rblank) { scores[1 ^ color] -= points[r + 1]; scores[1 ^ color] += points[r]; }
                    else { scores[1 ^ color] -= points[r]; scores[color ^ 1] += points[0]; }

                    rchange = true;
                }


                if ((l == 4 && !lchange) || (r == 4 && !rchange)) {
                    generator_sort.clear();
                    scores[0] = _scores[0], scores[1] = _scores[1];
                    long long tmp = update_point(x, y, color, scores);
                    generator_sort.insert(Coordinate(x, y, scores, tmp));
                    return;
                }
            }

            generator_sort.insert(Coordinate(x, y, scores, scores[color] - scores[1 ^ color]));
        }
}