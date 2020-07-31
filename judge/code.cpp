#include "AIController.h"
#include <utility>
#include <cstring>
#include <vector>
#include <set>
#include <map>
using namespace std;

extern int ai_side; //0: black, 1: white, -1：空
std::string ai_name = "Master of Gomoku";

#define MAX 10000000
#define MIN -10000000
const int DEPTH = 6;
int turn = 0;
int board[15][15], chess_score[2], scores[2][72];

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

vector<std::pair<string, int>> patterns = {
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

struct Coordinate
{
    int x;
    int y;
    int score;
    Coordinate() { x = 0, y = 0, score = 0; };
    Coordinate(int x, int y)
    {
        this->x = x;
        this->y = y;
        score = 0;
    }
    Coordinate(int x, int y, int score)
    {
        this->x = x;
        this->y = y;
        this->score = score;
    }
    bool operator<(const Coordinate &pos) const
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

struct possible_Coordinate
{
    set<Coordinate> current_possible;
    vector<pair<set<Coordinate>, Coordinate>> his;
    int directions[8][2] = {1, 1, 1, -1, -1, 1, -1, -1, 1, 0, -1, 0, 0, 1, 0, -1};
    void add(const Coordinate &p)
    {
        pair<set<Coordinate>, Coordinate> history;
        for (int i = 0; i < 8; i++)
        {
            if (out_board(p.x + directions[i][0], p.y + directions[i][1]))
                continue;

            if (board[p.x + directions[i][0]][p.y + directions[i][1]] == -1)
            {
                Coordinate pos(p.x + directions[i][0], p.y + directions[i][1]);
                pair<set<Coordinate>::iterator, bool> insert_result = current_possible.insert(pos);

                if (insert_result.second)
                    history.first.insert(pos);
            }
        }

        if (current_possible.find(p) != current_possible.end())
        {
            current_possible.erase(p);
            history.second = p;
        }
        else
        {
            history.second.x = -1;
        }

        his.push_back(history);
    }
    void back()
    {
        if (current_possible.empty())
            return;

        pair<set<Coordinate>, Coordinate> history = his.back();
        his.pop_back();
        set<Coordinate>::iterator iter;

        for (iter = history.first.begin(); iter != history.first.end(); iter++) //清除掉前一步加入的点
        {
            current_possible.erase(*iter);
        }

        if (history.second.x != -1)
            current_possible.insert(history.second);
    }
} possible_position;

struct searcher
{
    struct node //trie树节点
    {
        char ch;
        int fail, parent;
        map<char, int> sons;
        vector<int> output;
        node(int p, char c) : parent(p), ch(c), fail(-1) {}
    };
    int max_state;      //最大状态数
    vector<node> nodes; //trie树
    vector<string> pat; //需要匹配的模式

    searcher() : max_state(0)
    {
        add_state(-1, 'l');
        nodes[0].fail = -1;
    }

    void load(const vector<string> &pat)
    {
        this->pat = pat;
    }

    void build()
    {
        int i, j;
        for (i = 0; i < pat.size(); i++)
        {
            int current_index = 0;
            for (j = 0; j < pat[i].size(); j++)
            {
                if (nodes[current_index].sons.find(pat[i][j]) == nodes[current_index].sons.end())
                {
                    nodes[current_index].sons[pat[i][j]] = ++max_state;

                    add_state(current_index, pat[i][j]);
                    current_index = max_state;
                }
                else
                {
                    current_index = nodes[current_index].sons[pat[i][j]];
                }
            }
            nodes[current_index].output.push_back(i);
        }
        vector<int> mid_nodes;

        node root = nodes[0]; //给第一层的节点设置fail为0，并把第二层节点加入到midState里

        map<char, int>::iterator iter1, iter2;
        for (iter1 = root.sons.begin(); iter1 != root.sons.end(); iter1++)
        {
            nodes[iter1->second].fail = 0;
            node &current_node = nodes[iter1->second];

            for (iter2 = current_node.sons.begin(); iter2 != current_node.sons.end(); iter2++)
            {
                mid_nodes.push_back(iter2->second);
            }
        }

        while (mid_nodes.size()) //广度优先遍历
        {
            vector<int> newMid_nodes;

            int i;
            for (i = 0; i < mid_nodes.size(); i++)
            {
                node &current_node = nodes[mid_nodes[i]];

                int current_fail = nodes[current_node.parent].fail;
                while (true)
                {
                    node &current_fail_node = nodes[current_fail];

                    if (current_fail_node.sons.find(current_node.ch) != current_fail_node.sons.end())
                    {
                        current_node.fail = current_fail_node.sons.find(current_node.ch)->second;

                        if (nodes[current_node.fail].output.size())
                        {
                            current_node.output.insert(current_node.output.end(), nodes[current_node.fail].output.begin(), nodes[current_node.fail].output.end());
                        }

                        break;
                    }
                    else
                    {
                        current_fail = current_fail_node.fail;
                    }

                    if (current_fail == -1)
                    {
                        current_node.fail = 0;
                        break;
                    }
                }
                for (iter1 = current_node.sons.begin(); iter1 != current_node.sons.end(); iter1++)
                {
                    newMid_nodes.push_back(iter1->second);
                }
            }
            mid_nodes = newMid_nodes;
        }
    }

    vector<int> search(const string &text)
    {
        vector<int> result;
        int current_index = 0;

        unsigned int i;
        map<char, int>::iterator iter;
        for (i = 0; i < text.size();)
        {
            if ((iter = nodes[current_index].sons.find(text[i])) != nodes[current_index].sons.end())
            {
                current_index = iter->second;
                i++;
            }
            else
            {
                while (nodes[current_index].fail != -1 && nodes[current_index].sons.find(text[i]) == nodes[current_index].sons.end())
                {
                    current_index = nodes[current_index].fail;
                }

                if (nodes[current_index].sons.find(text[i]) == nodes[current_index].sons.end())
                {
                    i++;
                }
            }

            if (nodes[current_index].output.size())
            {
                result.insert(result.end(), nodes[current_index].output.begin(), nodes[current_index].output.end());
            }
        }

        return result;
    }

    void add_state(int parent, char ch)
    {
        nodes.push_back(node(parent, ch));
    }
} acsearch;

int evalueate_point(int x, int y)
{
    int result, i, j, role;

    result = 0;
    role = 1 - ai_side;

    string lines[4][2];
    for (i = max(0, x - 5); i < min(15, x + 6); i++)
    {
        if (i != x)
        {
            lines[0][0].push_back(board[i][y] == role ? '1' : board[i][y] == -1 ? '0' : '2');
            lines[0][1].push_back(board[i][y] == role ? '2' : board[i][y] == -1 ? '0' : '1');
        }
        else
        {
            lines[0][0].push_back('1');
            lines[0][1].push_back('1');
        }
    }
    for (i = max(0, y - 5); i < min(15, y + 6); i++)
    {
        if (i != y)
        {
            lines[1][0].push_back(board[x][i] == role ? '1' : board[x][i] == -1 ? '0' : '2');
            lines[1][1].push_back(board[x][i] == role ? '2' : board[x][i] == -1 ? '0' : '1');
        }
        else
        {
            lines[1][0].push_back('1');
            lines[1][1].push_back('1');
        }
    }
    for (i = x - min(min(x, y), 5), j = y - min(min(x, y), 5); i < min(15, x + 6) && j < min(15, y + 6); i++, j++)
    {
        if (i != x)
        {
            lines[2][0].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
            lines[2][1].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
        }
        else
        {
            lines[2][0].push_back('1');
            lines[2][1].push_back('1');
        }
    }
    for (i = x + min(min(y, 15 - 1 - x), 5), j = y - min(min(y, 15 - 1 - x), 5); i >= max(0, x - 5) && j < min(15, y + 6); i--, j++)
    {
        if (i != x)
        {
            lines[3][0].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
            lines[3][1].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
        }
        else
        {
            lines[3][0].push_back('1');
            lines[3][1].push_back('1');
        }
    }

    for (i = 0; i < 4; i++)
    {
        for (int k = 0; k < 2; k++)
        {
            vector<int> tmp = acsearch.search(lines[i][k]);
            for (j = 0; j < tmp.size(); j++)
            {
                result += patterns[tmp[j]].second;
            }
        }
    }

    return result;
}

void update_score(Coordinate p)
{
    string lines[4][2];
    int role = 1 - ai_side, i, j;

    for (i = 0; i < 15; i++) //竖
    {
        lines[0][0].push_back(board[i][p.y] == role ? '1' : board[i][p.y] == -1 ? '0' : '2');
        lines[0][1].push_back(board[i][p.y] == role ? '2' : board[i][p.y] == -1 ? '0' : '1');
    }
    for (i = 0; i < 15; i++) //横
    {
        lines[1][0].push_back(board[p.x][i] == role ? '1' : board[p.x][i] == -1 ? '0' : '2');
        lines[1][1].push_back(board[p.x][i] == role ? '2' : board[p.x][i] == -1 ? '0' : '1');
    }
    for (i = p.x - min(p.x, p.y), j = p.y - min(p.x, p.y); i < 15 && j < 15; i++, j++) //反斜杠
    {
        lines[2][0].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
        lines[2][1].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
    }
    for (i = p.x + min(p.y, 15 - 1 - p.x), j = p.y - min(p.y, 15 - 1 - p.x); i >= 0 && j < 15; i--, j++) //斜杠
    {
        lines[3][0].push_back(board[i][j] == role ? '1' : board[i][j] == -1 ? '0' : '2');
        lines[3][1].push_back(board[i][j] == role ? '2' : board[i][j] == -1 ? '0' : '1');
    }

    int line_score[4][2] = {0};

    for (i = 0; i < 4; i++) //计算分数
    {
        for (int k = 0; k < 2; k++)
        {
            vector<int> result = acsearch.search(lines[i][k]);
            for (j = 0; j < result.size(); j++)
            {
                line_score[i][k] += patterns[result[j]].second;
            }
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
    scores[0][a] = line_score[0][0];
    scores[1][a] = line_score[0][1];
    scores[0][b] = line_score[1][0];
    scores[1][b] = line_score[1][1];

    for (i = 0; i < 2; i++)
    {
        chess_score[i] += scores[i][a];
        chess_score[i] += scores[i][b];
    }

    if (p.y - p.x >= -10 && p.y - p.x <= 10)
    {

        for (i = 0; i < 2; i++)
            chess_score[i] -= scores[i][c];

        scores[0][c] = line_score[2][0];
        scores[1][c] = line_score[2][1];

        for (i = 0; i < 2; i++)
            chess_score[i] += scores[i][c];
    }

    if (p.x + p.y >= 4 && p.x + p.y <= 24)
    {

        for (i = 0; i < 2; i++)
            chess_score[i] -= scores[i][d];

        scores[0][d] = line_score[3][0];
        scores[1][d] = line_score[3][1];

        for (i = 0; i < 2; i++)
            chess_score[i] += scores[i][d];
    }
}

//alpha-beta剪枝
int abSearch(int depth, int alpha, int beta, int role)
{
    int my_score = (role == ai_side) ? chess_score[1] : chess_score[0];
    int competitor_score = (role == ai_side) ? chess_score[0] : chess_score[1];

    if (my_score >= 50000)
    {
        return MAX - 1000;
    }
    if (competitor_score >= 50000)
    {
        return MIN + 1000;
    }
    if (depth == 0)
    {
        return my_score - competitor_score;
    }

    set<Coordinate> possible;
    const set<Coordinate> &temp = possible_position.current_possible;
    set<Coordinate>::iterator iter;
    for (iter = temp.begin(); iter != temp.end(); iter++)
    {
        possible.insert(Coordinate(iter->x, iter->y, evalueate_point(iter->x, iter->y)));
    }
    int cnt = 0;
    while (cnt < 9 && !possible.empty())
    {
        Coordinate p = *possible.begin();
        possible.erase(possible.begin());

        board[p.x][p.y] = role;
        update_score(p);

        p.score = 0;
        possible_position.add(p);

        int val = -abSearch(depth - 1, -beta, -alpha, 1 - role);
        possible_position.back();

        board[p.x][p.y] = -1;
        update_score(p);

        if (val >= beta)
        {
            return beta;
        }
        if (val > alpha)
        {
            alpha = val;
            if (depth == DEPTH)
            {
                next_point = p;
            }
        }
        cnt++;
    }
    return alpha;
}

void init()
{
    memset(board, -1, sizeof(board));
    vector<string> pat_strings;
    for (int i = 0; i < patterns.size(); i++)
    {
        pat_strings.push_back(patterns[i].first);
    }

    acsearch.load(pat_strings);
    acsearch.build();
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
            board[2][7] = ai_side;
            update_score(Coordinate(2, 7));
            next_point = Coordinate(2, 7);
            possible_position.add(Coordinate(2, 7));
            return std::make_pair(2, 7);
        }
        if (turn == 3)
        {
            int tmp;
            tmp = chess_score[0];
            chess_score[0] = chess_score[1];
            chess_score[1] = tmp;
            for (int i = 0; i < 15; ++i)
                for (int j = 0; j < 15; ++j)
                    if (board[i][j] != -1)
                    {
                        board[i][j] = board[i][j] ^ 1;
                    }

            abSearch(DEPTH, MIN, MAX, ai_side);
            board[next_point.x][next_point.y] = ai_side;

            update_score(next_point);
            possible_position.add(next_point);
            return std::make_pair(next_point.x, next_point.y);
        }
    }

    if (turn == 2 && ai_side == 0)
    {
        next_point = Coordinate(0, 1);
        board[next_point.x][next_point.y] = ai_side;
        update_score(next_point);
        possible_position.add(next_point);
        return std::make_pair(0, 1);
    }

    board[x][y] = 1 - ai_side;
    update_score(Coordinate(x, y));
    possible_position.add(Coordinate(x, y));

    abSearch(DEPTH, MIN, MAX, ai_side);
    board[next_point.x][next_point.y] = ai_side;
    update_score(next_point);
    possible_position.add(next_point);
    return std::make_pair(next_point.x, next_point.y);
}