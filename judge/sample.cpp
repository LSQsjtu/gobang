#include "AIController.h"
#include <utility>
#include <cstring>

extern int ai_side; //0: black, 1: white
std::string ai_name = "your_ai_name_here";

int turn = 0;
int board[15][15];

//init function is called once at the beginning
void init() {
	/* TODO: Replace this by your code */
	memset(board, -1, sizeof(board));
}

// loc is the action of your opponent
// Initially, loc being (-1,-1) means it's your first move
// If this is the third step(with 2 black ), where you can use the swap rule, your output could be either (-1, -1) to indicate that you choose a swap, or a coordinate (x,y) as normal.

std::pair<int, int> getRandom() {
    while (true) {
        int x = rand() % 15;
        int y = rand() % 15;
        if (board[x][y] == -1) {
            board[x][y] = ai_side;
            return std::make_pair(x, y);
        }
    }
}

std::pair<int, int> action(std::pair<int, int> loc) {
	/* TODO: Replace this by your code */
	/* This is now a random strategy */

    turn++;
    board[loc.first][loc.second] = 1 - ai_side;
    if (turn == 2 && ai_side == 1) {
        return std::make_pair(-1, -1);
    } else {
        return getRandom();
    }
}