#include "AIController.h"
#include <utility>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <cassert>
//#include <iostream>
using namespace std;

extern int ai_side; //0: black, 1: white, -1：空
std::string ai_name = "Master of Gomoku";

#define MAX 10000000
#define MIN -10000000
const int DEPTH = 7;
long long ZobristValue, boardZobristValue[15][15][2];
int turn = 0;
int board[15][15], chess_score[2], scores[2][72];

struct Pattern
{
    string pattern;
    int score;
};

vector<Pattern> patterns = {
    {"11111", 50000},
    {"011110", 4320},
    {"011100", 720},
    {"001110", 720},
    {"011010", 720},
    {"010110", 720},
    {"11110", 720},
    {"01111", 720},
    {"11011", 720},
    {"10111", 720},
    {"11101", 720},
    {"001100", 120},
    {"001010", 120},
    {"010100", 120},
    {"000100", 20},
    {"001000", 20},
};

enum index
{
    ALPHA,
    BETA,
    EXACT,
    EMPTY = -1
};

long long random64()
{
    return (long long)rand() | ((long long)rand() << 15) | ((long long)rand() << 30) | ((long long)rand() << 45) | ((long long)rand() << 60);
}

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
//trie树节点
struct node
{
    char ch;
    map<char, int> sons;
    int fail;
    vector<int> output;
    int parent;
    node(int p, char c) : parent(p), ch(c), fail(-1) {}
};

struct searcher
{
    int maxState;           //最大状态数
    vector<node> nodes;     //trie树
    vector<string> paterns; //需要匹配的模式

    searcher() : maxState(0)
    {
        //初始化根节点
        AddState(-1, 'a');
        nodes[0].fail = -1;
    }

    void LoadPattern(const vector<string> &paterns)
    {
        this->paterns = paterns;
    }

    void BuildGotoTable()
    {
        assert(nodes.size());

        unsigned int i, j;
        for (i = 0; i < paterns.size(); i++)
        {
            int currentIndex = 0;
            for (j = 0; j < paterns[i].size(); j++)
            {
                if (nodes[currentIndex].sons.find(paterns[i][j]) == nodes[currentIndex].sons.end())
                {
                    nodes[currentIndex].sons[paterns[i][j]] = ++maxState;

                    AddState(currentIndex, paterns[i][j]);
                    currentIndex = maxState;
                }
                else
                {
                    currentIndex = nodes[currentIndex].sons[paterns[i][j]];
                }
            }

            nodes[currentIndex].output.push_back(i);
        }
    }

    void BuildFailTable()
    {
        assert(nodes.size());

        vector<int> midNodesIndex;

        //给第一层的节点设置fail为0，并把第二层节点加入到midState里
        node root = nodes[0];

        map<char, int>::iterator iter1, iter2;
        for (iter1 = root.sons.begin(); iter1 != root.sons.end(); iter1++)
        {
            nodes[iter1->second].fail = 0;
            node &currentNode = nodes[iter1->second];

            //收集第三层节点
            for (iter2 = currentNode.sons.begin(); iter2 != currentNode.sons.end(); iter2++)
            {
                midNodesIndex.push_back(iter2->second);
            }
        }

        //广度优先遍历
        while (midNodesIndex.size())
        {
            vector<int> newMidNodesIndex;

            unsigned int i;
            for (i = 0; i < midNodesIndex.size(); i++)
            {
                node &currentNode = nodes[midNodesIndex[i]];

                //以下循环为寻找当前节点的fail值
                int currentFail = nodes[currentNode.parent].fail;
                while (true)
                {
                    node &currentFailNode = nodes[currentFail];

                    if (currentFailNode.sons.find(currentNode.ch) != currentFailNode.sons.end())
                    {
                        currentNode.fail = currentFailNode.sons.find(currentNode.ch)->second;

                        if (nodes[currentNode.fail].output.size())
                        {
                            currentNode.output.insert(currentNode.output.end(), nodes[currentNode.fail].output.begin(), nodes[currentNode.fail].output.end());
                        }

                        break;
                    }
                    else
                    {
                        currentFail = currentFailNode.fail;
                    }

                    if (currentFail == -1)
                    {
                        currentNode.fail = 0;
                        break;
                    }
                }
                for (iter1 = currentNode.sons.begin(); iter1 != currentNode.sons.end(); iter1++)
                {
                    newMidNodesIndex.push_back(iter1->second);
                }
            }
            midNodesIndex = newMidNodesIndex;
        }
    }

    vector<int> search(const string &text)
    {
        vector<int> result;
        int currentIndex = 0;

        unsigned int i;
        map<char, int>::iterator iter;
        for (i = 0; i < text.size();)
        {
            if ((iter = nodes[currentIndex].sons.find(text[i])) != nodes[currentIndex].sons.end())
            {
                currentIndex = iter->second;
                i++;
            }
            else
            {
                while (nodes[currentIndex].fail != -1 && nodes[currentIndex].sons.find(text[i]) == nodes[currentIndex].sons.end())
                {
                    currentIndex = nodes[currentIndex].fail;
                }

                if (nodes[currentIndex].sons.find(text[i]) == nodes[currentIndex].sons.end())
                {
                    i++;
                }
            }

            if (nodes[currentIndex].output.size())
            {
                result.insert(result.end(), nodes[currentIndex].output.begin(), nodes[currentIndex].output.end());
            }
        }

        return result;
    }

    void AddState(int parent, char ch)
    {
        nodes.push_back(node(parent, ch));
    }
} acs;

int evalueate_point(int x, int y)
{
    int result, i, j, role;

    result = 0;
    role = 1 - ai_side;

    string lines[4];
    string lines1[4];
    for (i = max(0, x - 5); i < min(15, x + 6); i++)
    {
        if (i != x)
        {
            lines[0].push_back(board[i][y] == role ? '1' : board[i][y] == -1 ? '0' : '2');
            lines1[0].push_back(board[i][y] == role ? '2' : board[i][y] == -1 ? '0' : '1');
        }
        else
        {
            lines[0].push_back('1');
            lines1[0].push_back('1');
        }
    }
    for (i = max(0, y - 5); i < min(15, y + 6); i++)
    {
        if (i != y)
        {
            lines[1].push_back(board[x][i] == role ? '1' : board[x][i] == -1 ? '0' : '2');
            lines1[1].push_back(board[x][i] == role ? '2' : board[x][i] == -1 ? '0' : '1');
        }
        else
        {
            lines[1].push_back('1');
            lines1[1].push_back('1');
        }
    }
    for (i = x - min(min(x, y), 5), j = y - min(min(x, y), 5); i < min(15, x + 6) && j < min(15, y + 6); i++, j++)
    {
        if (i != x)
        {
            lines[2].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
            lines1[2].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
        }
        else
        {
            lines[2].push_back('1');
            lines1[2].push_back('1');
        }
    }
    for (i = x + min(min(y, 15 - 1 - x), 5), j = y - min(min(y, 15 - 1 - x), 5); i >= max(0, x - 5) && j < min(15, y + 6); i--, j++)
    {
        if (i != x)
        {
            lines[3].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
            lines1[3].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
        }
        else
        {
            lines[3].push_back('1');
            lines1[3].push_back('1');
        }
    }

    for (i = 0; i < 4; i++)
    {
        vector<int> tmp = acs.search(lines[i]);
        for (j = 0; j < tmp.size(); j++)
        {
            result += patterns[tmp[j]].score;
        }

        tmp = acs.search(lines1[i]);
        for (j = 0; j < tmp.size(); j++)
        {
            result += patterns[tmp[j]].score;
        }
    }

    return result;
}

void update_score(position p)
{
    string lines[4];
    string lines1[4];
    int i, j;
    int role = 1 - ai_side;

    for (i = 0; i < 15; i++) //竖
    {
        lines[0].push_back(board[i][p.y] == role ? '1' : board[i][p.y] == -1 ? '0' : '2');
        lines1[0].push_back(board[i][p.y] == role ? '2' : board[i][p.y] == -1 ? '0' : '1');
    }
    for (i = 0; i < 15; i++) //横
    {
        lines[1].push_back(board[p.x][i] == role ? '1' : board[p.x][i] == -1 ? '0' : '2');
        lines1[1].push_back(board[p.x][i] == role ? '2' : board[p.x][i] == -1 ? '0' : '1');
    }
    for (i = p.x - min(p.x, p.y), j = p.y - min(p.x, p.y); i < 15 && j < 15; i++, j++) //反斜杠
    {
        lines[2].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
        lines1[2].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
    }
    for (i = p.x + min(p.y, 15 - 1 - p.x), j = p.y - min(p.y, 15 - 1 - p.x); i >= 0 && j < 15; i--, j++) //斜杠
    {
        lines[3].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
        lines1[3].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
    }

    int line_score[4];
    int line1_score[4];
    memset(line_score, 0, sizeof(line_score));
    memset(line1_score, 0, sizeof(line1_score));

    for (i = 0; i < 4; i++) //计算分数
    {
        vector<int> result = acs.search(lines[i]);
        for (j = 0; j < result.size(); j++)
        {
            line_score[i] += patterns[result[j]].score;
        }

        result = acs.search(lines1[i]);
        for (j = 0; j < result.size(); j++)
        {
            line1_score[i] += patterns[result[j]].score;
        }
    }

    int a = p.y;
    int b = 15 + p.x;
    int c = 2 * 15 + (p.y - p.x + 10);
    int d = 2 * 15 + 21 + (p.x + p.y - 4);
    //减去以前的记录
    for (i = 0; i < 2; i++)
    {
        chess_score[i] -= scores[i][a];
        chess_score[i] -= scores[i][b];
    }

    //scores顺序 竖、横、\、/
    scores[0][a] = line_score[0];
    scores[1][a] = line1_score[0];
    scores[0][b] = line_score[1];
    scores[1][b] = line1_score[1];

    //加上新的记录
    for (i = 0; i < 2; i++)
    {
        chess_score[i] += scores[i][a];
        chess_score[i] += scores[i][b];
    }

    if (p.y - p.x >= -10 && p.y - p.x <= 10)
    {

        for (i = 0; i < 2; i++)
            chess_score[i] -= scores[i][c];

        scores[0][c] = line_score[2];
        scores[1][c] = line1_score[2];

        for (i = 0; i < 2; i++)
            chess_score[i] += scores[i][c];
    }

    if (p.x + p.y >= 4 && p.x + p.y <= 24)
    {

        for (i = 0; i < 2; i++)
            chess_score[i] -= scores[i][d];

        scores[0][d] = line_score[3];
        scores[1][d] = line1_score[3];

        for (i = 0; i < 2; i++)
            chess_score[i] += scores[i][d];
    }
}

void record_hashItem(int depth, int score, index state)
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
int get_hashItemScore(int depth, int alpha, int beta)
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
    int score = get_hashItemScore(depth, alpha, beta);
    if (score != MAX + 1 && depth != DEPTH)
    {
        return score;
    }
    int score1 = (role == ai_side) ? chess_score[1] : chess_score[0];
    int score2 = (role == ai_side) ? chess_score[0] : chess_score[1];

    if (score1 >= 50000)
    {
        return MAX - 1000 - (DEPTH - depth);
    }
    if (score2 >= 50000)
    {
        return MIN + 1000 + (DEPTH - depth);
    }
    if (depth == 0)
    {
        record_hashItem(depth, score1 - score2, EXACT);
        return score1 - score2;
    }

    int cnt = 0;
    set<position> possible;
    const set<position> &temp = ppm.current_possible;

    //对当前可能出现的位置进行粗略评分
    set<position>::iterator iter;
    for (iter = temp.begin(); iter != temp.end(); iter++)
    {
        possible.insert(position(iter->x, iter->y, evalueate_point(iter->x, iter->y)));
    }

    while (!possible.empty())
    {
        position p = *possible.begin();

        possible.erase(possible.begin());

        //放置棋子
        board[p.x][p.y] = role;
        ZobristValue ^= boardZobristValue[p.x][p.y][role];
        update_score(p);

        //增加可能出现的位置
        p.score = 0;
        ppm.add(board, p);

        int val = -abSearch(depth - 1, -beta, -alpha, 1 - role);

        //取消上一次增加的可能出现的位置
        ppm.back();

        //取消放置
        board[p.x][p.y] = -1;
        ZobristValue ^= boardZobristValue[p.x][p.y][role];
        update_score(p);

        if (val >= beta)
        {
            record_hashItem(depth, beta, BETA);
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

    record_hashItem(depth, alpha, state);
    return alpha;
}

void init()
{
    memset(board, -1, sizeof(board));
    ZobristValue = random64();
    vector<string> patternStrs;
    for (size_t i = 0; i < patterns.size(); i++)
    {
        patternStrs.push_back(patterns[i].pattern);
    }

    //初始化ACSearcher
    acs.LoadPattern(patternStrs);
    acs.BuildGotoTable();
    acs.BuildFailTable();

    for (int i = 0; i < 15; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                boardZobristValue[i][j][k] = random64();
            }
        }
    }
}

// loc is the action of your opponent
// Initially, loc being (-1,-1) means it's your first move
// If this is the third step(with 2 black ), where you can use the swap rule, your output could be either (-1, -1) to indicate that you choose a swap, or a coordinate (x,y) as normal.

std::pair<int, int> action(std::pair<int, int> loc)
{
    turn++;
    int x = loc.first, y = loc.second;
    if (x == -1 && y == -1)
    {
        if (turn == 1)
        {
            ZobristValue ^= boardZobristValue[7][7][ai_side];
            board[7][7] = ai_side;
            update_score(position(7, 7));
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
                    {
                        board[i][j] = board[i][j] ^ 1;
                        update_score(position(i, j));
                    }

            abSearch(DEPTH, MIN, MAX, ai_side);
            board[next_point.x][next_point.y] = ai_side;
            ZobristValue ^= boardZobristValue[next_point.x][next_point.y][ai_side];
            update_score(next_point);
            ppm.add(board, next_point);
            return std::make_pair(next_point.x, next_point.y);
        }
    }

    board[x][y] = 1 - ai_side;
    ZobristValue ^= boardZobristValue[x][y][1 - ai_side];
    update_score(position(x, y));
    ppm.add(board, position(x, y));

    abSearch(DEPTH, MIN, MAX, ai_side);
    board[next_point.x][next_point.y] = ai_side;
    ZobristValue ^= boardZobristValue[next_point.x][next_point.y][ai_side];
    update_score(next_point);
    ppm.add(board, next_point);
    return std::make_pair(next_point.x, next_point.y);
}