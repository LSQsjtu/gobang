#include "AIController.h"
#include <utility>
#include <cstring>
#include <vector>
#include <set>

using namespace std;

extern int ai_side; //0: black, 1: white, -1：空
std::string ai_name = "Master of Gomoku";

#define MAX 1000000
#define MIN -1000000
const int DEPTH = 7;
int points[7] = {0, 0, 10, 100, 1000, 10000, 100000};
long long ZobristValue, boardZobristValue[15][15][2];
int range[4] = {0, 15, 0, 15};
int turn = 0;
int board[15][15], chess_score[2];

void update_react(int x, int y)
{
    range[0] = ((x - 3 > 0) ? (x - 3 < range[0] ? x - 3 : range[0]) : 0);
    range[1] = ((x + 3 < 14) ? (x + 3 >= range[1] ? x + 3 : range[1]) : 14);
    range[2] = ((y - 3 > 0) ? (y - 3 < range[2] ? y - 3 : range[2]) : 0);
    range[3] = ((y + 3 < 14) ? (y + 3 >= range[3] ? y + 3 : range[3]) : 14);
}

enum ChessType
{
    live_Five = 100000,
    live_Four = 10000,
    sleep_Four = 1000,
    live_Three = 1000,
    live_Two = 100,
    sleep_Three = 100,
    live_One = 10,
    sleep_Two = 10,
    sleep_One = 1,
    unknown = 0,
};

enum index
{
    ALPHA,
    BETA,
    EXACT,
    EMPTY = -1
};

struct position
{
    int x;
    int y;
    int score;
    position() { x = 0, y = 0, score = 0; };
    position(int x, int y)
    {
        this->x = x;
        this->y = y;
        score = 0;
    }
    position(int x, int y, int score)
    {
        this->x = x;
        this->y = y;
        this->score = score;
    }
    bool operator<(const position &pos) const
    {
        if (score != pos.score)
        {
            return score > pos.score;
        }
        if (x != pos.x)
        {
            return x < pos.x;
        }
        else
        {
            return y < pos.y;
        }
    }
} next_point;

struct history
{
    set<position> addedPositions;
    position removedPosition;
};

//保存棋局的哈希表条目
struct HashItem
{
    long long key;
    int depth;
    int score;
    index state;
} hashItems[0xffff];

bool out_board(int x, int y)
{
    if (x >= 0 && x < 15 && y >= 0 && y < 15)
    {
        return false;
    }
    else
    {
        return true;
    }
}

struct possible_position
{
    set<position> current_possible;
    vector<history> his;
    vector<pair<int, int>> directions;
    int (*evaluateFunc)(char board[15][15], position p);
    possible_position()
    {
        directions.push_back(pair<int, int>(1, 1));
        directions.push_back(pair<int, int>(1, -1));
        directions.push_back(pair<int, int>(-1, 1));
        directions.push_back(pair<int, int>(-1, -1));
        directions.push_back(pair<int, int>(1, 0));
        directions.push_back(pair<int, int>(0, 1));
        directions.push_back(pair<int, int>(-1, 0));
        directions.push_back(pair<int, int>(0, -1));
    }
    void add(int board[15][15], const position &p)
    {
        set<position> addedPositions;

        for (int i = 0; i < 8; i++)
        {
            //判断范围
            if (out_board(p.x + directions[i].first, p.y + directions[i].second))
                continue;

            if (board[p.x + directions[i].first][p.y + directions[i].second] == EMPTY)
            {
                position pos(p.x + directions[i].first, p.y + directions[i].second);
                pair<set<position>::iterator, bool> insertResult = current_possible.insert(pos);

                //如果插入成功
                if (insertResult.second)
                    addedPositions.insert(pos);
            }
        }

        history hi;
        hi.addedPositions = addedPositions;

        if (current_possible.find(p) != current_possible.end())
        {
            current_possible.erase(p);
            hi.removedPosition = p;
        }
        else
        {
            hi.removedPosition.x = -1;
        }

        his.push_back(hi);
    }
    void back()
    {
        if (current_possible.empty())
            return;

        history hi = his[his.size() - 1];
        his.pop_back();

        set<position>::iterator iter;

        //清除掉前一步加入的点
        for (iter = hi.addedPositions.begin(); iter != hi.addedPositions.end(); iter++)
        {
            current_possible.erase(*iter);
        }

        //加入前一步删除的点
        if (hi.removedPosition.x != -1)
            current_possible.insert(hi.removedPosition);
    }
} ppm;

set<position> current_position;

ChessType scoretable(int number, int kong) //计分板
{
    switch (number)
    {
    case 5:
        return live_Five;
        break;
    case 4:
        if (kong == 2)
            return live_Four;
        else if (kong == 1)
            return sleep_Four;
        break;
    case 3:
        if (kong == 2)
            return live_Three;
        else if (kong == 1)
            return sleep_Three;
        break;
    case 2:
        if (kong == 2)
            return live_Two;
        else if (kong == 1)
            return sleep_Two;
        break;
    case 1:
        if (kong == 2)
            return live_One;
        else if (kong == 1)
            return sleep_One;
        break;
    default:
        return unknown;
        break;
    }
}

int cntscore(vector<int> n, int role) //一维数组来计算
{
    int score = 0;
    int len = n.size();
    int kong = 0;
    int number = 0;
    if (n[0] == -1)
        ++kong;
    else if (n[0] == role)
        ++number;
    int i = 1;
    while (i < len)
    {
        if (n[i] == role)
            ++number;
        else if (n[i] == -1)
        {
            if (number == 0)
                kong = 1;
            else
            {
                score += scoretable(number, kong + 1);
                kong = 1;
                number = 0;
            }
        }
        else
        {
            score += scoretable(number, kong);
            kong = 0;
            number = 0;
        }
        ++i;
    }
    score += scoretable(number, kong);
    return score;
}

int evalueate_point(int x, int y, int ai_side)
{
    int score = 0, i, j;

    vector<int> lines[4];
    for (i = max(0, x - 5); i < min(15, x + 6); i++)
    {
        if (i < x)
        {
            if (board[i][y] == 1 - ai_side)
            {
                lines[0].clear();
                lines[0].push_back(board[i][y]);
            }
            else
                lines[0].push_back(board[i][y]);
        }
        else if (i == x)
        {
            lines[0].push_back(ai_side);
        }
        else
        {
            lines[0].push_back(board[i][y]);
            if (board[i][y] != ai_side)
            {
                break;
            }
        }
    }
    for (i = max(0, y - 5); i < min(15, y + 6); i++)
    {
        if (i < y)
        {
            if (board[x][i] == 1 - ai_side)
            {
                lines[1].clear();
                lines[1].push_back(board[x][i]);
            }
            else
            {
                lines[1].push_back(board[x][i]);
            }
        }
        else if (i == y)
        {
            lines[1].push_back(ai_side);
        }
        else
        {
            lines[1].push_back(board[x][i]);
            if (board[x][i] != ai_side)
            {
                break;
            }
        }
    }
    for (i = x - min(min(x, y), 5), j = y - min(min(x, y), 5); i < min(15, x + 6) && j < min(15, y + 6); i++, j++)
    {
        if (i < x)
        {
            if (board[i][j] == 1 - ai_side)
            {
                lines[2].clear();
                lines[2].push_back(board[i][j]);
            }
            else
            {
                lines[2].push_back(board[i][j]);
            }
        }
        else if (i == x)
        {
            lines[2].push_back(ai_side);
        }
        else
        {
            lines[2].push_back(board[i][j]);
            if (board[i][j] != ai_side)
            {
                break;
            }
        }
    }
    for (i = x + min(min(y, 15 - 1 - x), 5), j = y - min(min(y, 15 - 1 - x), 5); i >= max(0, x - 5) && j < min(15, y + 6); i--, j++)
    {
        if (i < x)
        {
            if (board[i][j] == 1 - ai_side)
            {
                lines[3].clear();
                lines[3].push_back(board[i][j]);
            }
            else
            {
                lines[3].push_back(board[i][j]);
            }
        }
        else if (i == x)
        {
            lines[3].push_back(ai_side);
        }
        else
        {
            lines[3].push_back(board[i][j]);
            if (board[i][j] != ai_side)
            {
                break;
            }
        }
    }
    for (int i = 0; i < 4; i++)
        score += cntscore(lines[i], ai_side);
    return score;
}

void update_score(position p, int role)
{
    int x = p.x, y = p.y;
    const int d[4][2] = {0, 1, 1, 1, 1, 0, 1, -1};
    std::pair<int, int> direction[4];
    direction[0] = pair<int, int>(0, 1);
    direction[1] = pair<int, int>(1, 1);
    direction[2] = pair<int, int>(1, 0);
    direction[3] = pair<int, int>(1, -1);
    bool lblank = false, rblank = false;
    int l = 0, r = 0;

    for (int i = 0; i < 4; ++i)
    {
        for (l = 0; x - direction[i].first * (l + 1) >= 0 && y - direction[i].second * (l + 1) >= 0 && y - direction[i].second * (l + 1) < 15; ++l)
            if (board[x - direction[i].first * (l + 1)][y - direction[i].second * (l + 1)] != role)
                break;

        for (r = 0; x + direction[i].first * (r + 1) < 15 && y + direction[i].second * (r + 1) >= 0 && y + direction[i].second * (r + 1) < 15; ++r)
            if (board[x + direction[i].first * (r + 1)][y + direction[i].second * (r + 1)] != role)
                break;

        lblank = ((x - direction[i].first * (l + 1) >= 0 && y - direction[i].second * (l + 1) >= 0 && y - direction[i].second * (l + 1) < 15) && board[x - direction[i].first * (l + 1)][y - direction[i].second * (l + 1)] == -1);
        rblank = ((x + direction[i].first * (r + 1) < 15 && y + direction[i].second * (r + 1) >= 0 && y + direction[i].second * (r + 1) < 15) && board[x + direction[i].first * (r + 1)][y + direction[i].second * (r + 1)] == -1);

        if (lblank)
            chess_score[role] -= points[l + 1];
        else
            chess_score[role] -= points[l];

        if (rblank)
            chess_score[role] -= points[r + 1];
        else
            chess_score[role] -= points[r];

        if (l + r + 1 >= 5)
            chess_score[role] += points[6];
        else
        {
            if (!lblank && !rblank)
                chess_score[role] += points[0];
            else
                chess_score[role] += points[l + r + lblank + rblank];
        }

        if (l == 0 && !lblank)
        {
            for (l = 1; x - direction[i].first * (l + 1) >= 0 && y - direction[i].second * (l + 1) >= 0 && y - direction[i].second * (l + 1) < 15; ++l)
                if (board[x - direction[i].first * (l + 1)][y - direction[i].second * (l + 1)] != (role ^ 1))
                    break;

            lblank = ((x - direction[i].first * (l + 1) >= 0 && y - direction[i].second * (l + 1) >= 0 && y - direction[i].second * (l + 1) < 15) && board[x - direction[i].first * (l + 1)][y - direction[i].second * (l + 1)] == -1);

            if (lblank)
            {
                chess_score[1 ^ role] -= points[l + 1];
                chess_score[1 ^ role] += points[l];
            }
            else
            {
                chess_score[1 ^ role] -= points[l];
                chess_score[role ^ 1] += points[0];
            }
        }

        if (r == 0 && !rblank)
        {
            for (r = 1; x + direction[i].first * (r + 1) < 15 && y + direction[i].second * (r + 1) >= 0 && y + direction[i].second * (r + 1) < 15; ++r)
                if (board[x + direction[i].first * (r + 1)][y + direction[i].second * (r + 1)] != (role ^ 1))
                    break;

            rblank = ((x + direction[i].first * (r + 1) < 15 && y + direction[i].second * (r + 1) >= 0 && y + direction[i].second * (r + 1) < 15) && board[x + direction[i].first * (r + 1)][y + direction[i].second * (r + 1)] == -1);

            if (rblank)
            {
                chess_score[1 ^ role] -= points[r + 1];
                chess_score[1 ^ role] += points[r];
            }
            else
            {
                chess_score[1 ^ role] -= points[r];
                chess_score[role ^ 1] += points[0];
            }
        }
    }
}

void recordHashItem(int depth, int score, index state)
{
    int index = (int)(ZobristValue & 0xffff);
    HashItem *phashItem = &hashItems[index];

    if (phashItem->state != EMPTY && phashItem->depth > depth)
    {
        return;
    }

    phashItem->key = ZobristValue;
    phashItem->score = score;
    phashItem->state = state;
    phashItem->depth = depth;
}

//在哈希表中取得计算好的局面的分数
int getHashItemScore(int depth, int alpha, int beta)
{
    int index = (int)(ZobristValue & 0xffff);
    HashItem *phashItem = &hashItems[index];

    if (phashItem->state == EMPTY)
        return MAX + 1;

    if (phashItem->key == ZobristValue)
    {
        if (phashItem->depth >= depth)
        {
            if (phashItem->state == EXACT)
            {
                return phashItem->score;
            }
            if (phashItem->state == ALPHA && phashItem->score <= alpha)
            {
                return alpha;
            }
            if (phashItem->state == BETA && phashItem->score >= beta)
            {
                return beta;
            }
        }
    }
    return MAX + 1;
}

//alpha-beta剪枝
int abSearch(int depth, int alpha, int beta, int role)
{
    index state = ALPHA;
    int score = getHashItemScore(depth, alpha, beta);
    if (score != MAX + 1 && depth != DEPTH)
    {
        return score;
    }
    if (chess_score[0] >= 100000)
    {
        return MAX - 1000 - (DEPTH - depth);
    }
    if (chess_score[1] >= 100000)
    {
        return MIN + 1000 + (DEPTH - depth);
    }
    if (depth == 0)
    {
        recordHashItem(depth, chess_score[0] - chess_score[1], EXACT);
        return chess_score[0] - chess_score[1];
    }

    int cnt = 0;
    set<position> possible;
    const set<position> &temp = ppm.current_possible;

    //对当前可能出现的位置进行粗略评分
    set<position>::iterator iter;
    for (iter = temp.begin(); iter != temp.end(); iter++)
    {
        possible.insert(position(iter->x, iter->y, evalueate_point(iter->x, iter->y, role)));
    }

    while (!possible.empty())
    {
        position p = *possible.begin();

        possible.erase(possible.begin());

        //放置棋子
        board[p.x][p.y] = role;
        ZobristValue ^= boardZobristValue[p.x][p.y][role];
        update_score(p, role);

        //增加可能出现的位置
        p.score = 0;
        ppm.add(board, p);

        int val = -abSearch(depth - 1, -beta, -alpha, 1 - role);

        //取消上一次增加的可能出现的位置
        ppm.back();

        //取消放置
        board[p.x][p.y] = -1;
        ZobristValue ^= boardZobristValue[p.x][p.y][role];
        update_score(p, role);

        if (val >= beta)
        {
            recordHashItem(depth, beta, BETA);
            return beta;
        }
        if (val > alpha)
        {
            state = EXACT;
            alpha = val;
            if (depth == DEPTH)
            {
                next_point = p;
            }
        }

        cnt++;
        if (cnt >= 9)
        {
            break;
        }
    }

    recordHashItem(depth, alpha, state);
    return alpha;
}

//init function is called once at the beginning
void init()
{
    /* TODO: Replace this by your code */
    memset(board, -1, sizeof(board));
    ZobristValue = rand();
    for (int i = 0; i < 15; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                boardZobristValue[i][j][k] = rand();
            }
        }
    }
}

// loc is the action of your opponent
// Initially, loc being (-1,-1) means it's your first move
// If this is the third step(with 2 black ), where you can use the swap rule, your output could be either (-1, -1) to indicate that you choose a swap, or a coordinate (x,y) as normal.

std::pair<int, int> action(std::pair<int, int> loc)
{
    /* TODO: Replace this by your code */
    /* This is now a random strategy */

    turn++;
    int x = loc.first, y = loc.second;
    if (x == -1 && y == -1)
    {
        if (turn == 1)
        {
            board[7][7] = ai_side;
            update_score(position(7, 7), ai_side);
            next_point = position(7, 7);
            ppm.add(board, position(7, 7));
            return std::make_pair(7, 7);
        }
        if (turn == 3)
        {
            ai_side = 1 - ai_side;
            int tmp;
            tmp = chess_score[0];
            chess_score[0] = chess_score[1];
            chess_score[1] = tmp;
            for (int i = 0; i < 15; ++i)
                for (int j = 0; j < 15; ++j)
                    if (board[i][j] != -1)
                        board[i][j] = board[i][j] ^ 1;

            abSearch(DEPTH, MIN, MAX, ai_side);
            board[next_point.x][next_point.y] = ai_side;
            ppm.add(board, next_point);
            update_score(next_point, ai_side);
            return std::make_pair(next_point.x, next_point.y);
        }
    }

    board[x][y] = 1 - ai_side;
    ppm.add(board, position(x, y));
    update_score(position(x, y), 1 - ai_side);
    abSearch(DEPTH, MIN, MAX, ai_side);
    board[next_point.x][next_point.y] = ai_side;
    ppm.add(board, next_point);
    update_score(next_point, ai_side);
    return std::make_pair(next_point.x, next_point.y);
}