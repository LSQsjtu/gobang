import subprocess
import timeout_decorator
import time
import numpy as np
import sys
from flask import Flask, render_template, request, jsonify

app = Flask(__name__)


class AI:
    def __init__(self, path, id):
        self.path = path
        if path == 'human':
            self.human = 1
        else:
            self.human = 0
        self.id = id

    def send(self, message):
        value = str(message) + '\n'
        value = bytes(value, 'UTF-8')
        self.proc.stdin.write(value)
        self.proc.stdin.flush()

    def receive(self):
        return self.proc.stdout.readline().strip().decode()

    def init(self):
        if self.human == 0:
            self.proc = subprocess.Popen(self.path,
                                         stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE)
            self.send(self.id)
            self.name = self.receive()

    def action(self, a, b):
        if self.human == 1:
            value = str(a) + str(b)
        else:
            self.send(str(a) + ' ' + str(b))
            value = self.receive().split(' ')
        return int(value[0]), int(value[1])


class Board:
    def __init__(self):
        self.board = -np.ones((15, 15), dtype=int)

    def action(self, side, turn, x, y):
        if turn == 2 and side == 1 and x == -1 and y == -1:
            self.board = np.where(self.board != -1, 1 - self.board, self.board)
        else:
            self.board[x][y] = side


def try_init(ai0, ai1):
    ai0.init()
    ai1.init()


@app.route('/')
def index():
    return render_template("/web/chess.js")


@app.route('/start_game')
def start_game():
    global board
    global turn
    global ai0, ai1

    turn = 0
    board = Board()
    a1 = ["./code"]
    b1 = ["./baseline"]
    ai0, ai1 = AI(a1, 0), AI(b1, 1)
    try_init(ai0, ai1)
    return "游戏开始！"


@app.route('/send_message', methods=['GET'])
def send_message():
    message_get = ""
    message_get = request.args['message']

    global a, b
    global ai0
    global board
    global turn
    ++turn
    tmp = message_get.split()
    a = int(tmp[0])
    b = int(tmp[1])
    a, b = ai0.action(a, b)
    board.action(0, turn, a, b)

    return "收到消息"


@app.route('/change_to_json', methods=['GET'])
def change_to_json():
    global a, b
    global board
    global turn
    global ai1

    a, b = ai1.action(a, b)
    board.action(1, turn, a, b)
    print("后端回应")
    print(a)
    print(b)
    message_back = "".join(str(a) + " " + str(b))
    message_json = {"message": message_back}

    return jsonify(message_json)


if __name__ == '__main__':
    app.run(debug=true)
