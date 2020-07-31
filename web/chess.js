// storage存储保存的棋局
let storage = window.localStorage;
//获取Canvas对象(画布)
const canvas = document.getElementById("myCanvas");
const ctx = canvas.getContext("2d");

let isWin = false;
let flag = true; // 黑子先行
let array = []; // 存储棋局
let history = []; // 存储历史棋子
let isRobot = false; // 是否为人机对战
let robot_robot = false;

// 偏离位置
// 查找下棋坐标位置, 以(width/2 - 400, 60)为基准
let width = $(window).width();
let pointX = width / 2 - 400;
let dx = 120, dy = 45;

$(document).ready(function () {
    drawBoard();
    drawBlack();
    drawWhite();

    for (let i = 0; i < 15; i++) {
        array[i] = new Array();
        for (let j = 0; j < 15; j++) {
            array[i][j] = null;
        }
    }
    $.ajax({
        url: "http://127.0.0.1:5000/start_game"
    });
    $(document).mousedown(function (e) {
        let x = e.pageX, y = e.pageY;
        if (isWin || robot_robot) {
            return;
        }
        //以(width/2 - 400, 60) 为基准
        if (x < dx + pointX - 20 || x > dx + pointX + 560 + 20 || y < dy + 60 - 20 || y > dy + 620 + 20) {
            // alert("请在规定范围内落子");
            return;
        } else {
            // 画棋子
            drawChess(x, y);
            if (isRobot) {
                let realX, realY;
                let x1 = parseInt((x - pointX - dx) / 40);
                let x2 = (x - pointX - dx) % 40;
                if (x2 <= 20) {
                    realX = x1 * 40;
                } else {
                    realX = x1 * 40 + 40;
                }

                let y1 = parseInt((y - 60 - dy) / 40);
                let y2 = (y - 60 - dy) % 40;
                if (y2 <= 20) {
                    realY = y1 * 40;
                } else {
                    realY = y1 * 40 + 40;
                }

                // 添加数据到数组
                let idxX = realX / 40, idxY = realY / 40;
                robot_draw(idxY, idxX);
            }
        }
    });
});

function drawBoard() {
    ctx.lineWidth = 1;
    ctx.strokeStyle = "black";

    ctx.beginPath();
    for (let i = 0; i < 15; i++) {
        ctx.moveTo(dx + 0, dy + 40 * i);
        ctx.lineTo(dx + 560, dy + 40 * i);
    }
    for (let i = 0; i < 15; i++) {
        ctx.moveTo(dx + 40 * i, dy + 0);
        ctx.lineTo(dx + 40 * i, dy + 560);
    }

    ctx.stroke();
    ctx.fill();
    ctx.closePath();
}

function drawBlack() {
    var img = new Image();
    //图像被装入后触发
    img.onload = function () {
        ctx.drawImage(img, 0, 300, 80, 80);
    }
    img.src = "黑棋.png";
}

function drawWhite() {
    var img = new Image();
    //图像被装入后触发
    img.onload = function () {
        ctx.drawImage(img, 720, 300, 80, 80);
    }
    img.src = "白棋.png";
}

function drawChess(x, y) {
    let realX, realY;
    let x1 = parseInt((x - pointX - dx) / 40);
    let x2 = (x - pointX - dx) % 40;
    if (x2 <= 20) {
        realX = x1 * 40;
    } else {
        realX = x1 * 40 + 40;
    }

    let y1 = parseInt((y - 60 - dy) / 40);
    let y2 = (y - 60 - dy) % 40;
    if (y2 <= 20) {
        realY = y1 * 40;
    } else {
        realY = y1 * 40 + 40;
    }

    // 添加数据到数组
    let idxX = realX / 40, idxY = realY / 40;
    if (array[idxY][idxX] != null) {
        //alert("该位置已经下过棋！");
        return;
    }
    if (flag) {
        array[idxY][idxX] = 1;
    } else {
        array[idxY][idxX] = 0;
    }
    console.info(realX, realY);

    // 添加到history
    let val = idxY + "," + idxX;
    history.push(val);

    if (flag) {
        ctx.fillStyle = "black";
    } else {
        ctx.fillStyle = "white";
    }

    ctx.beginPath();
    ctx.arc(realX + dx, realY + dy, 16, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.fill();
    ctx.closePath();
    // 判断棋局输赢
    analyse(idxX, idxY, flag);

    flag = !flag;
}

function analyse(idxX, idxY, flag) {
    let color = 0;
    if (flag) {
        color = 1;
    }
    console.info(array);
    console.info(idxX, idxY, color)

    // 判断左右
    let a = idxX, b = idxY, num = 0;
    while (a >= 0) {
        if (array[b][a] == color) {
            num++;
            a--;
        } else {
            break;
        }
    }
    a = idxX;
    while (a < 15) {
        if (array[b][a] == color) {
            num++;
            a++;
        } else {
            break;
        }
    }
    showResult(num);

    // 判断上下
    a = idxX, b = idxY, num = 0;
    while (b >= 0) {
        if (array[b][a] == color) {
            num++;
            b--;
        } else {
            break;
        }
    }
    b = idxY;
    while (b < 15) {
        if (array[b][a] == color) {
            num++;
            b++;
        } else {
            break;
        }
    }
    showResult(num);

    // 判断左上右下
    a = idxX, b = idxY, num = 0;
    while (a >= 0 && b >= 0) {
        if (array[b][a] == color) {
            num++;
            a--;
            b--;
        } else {
            break;
        }
    }
    a = idxX, b = idxY;
    while (a < 15 && b < 15) {
        if (array[b][a] == color) {
            num++;
            a++;
            b++;
        } else {
            break;
        }
    }
    showResult(num);

    // 判断右上左下
    a = idxX, b = idxY, num = 0;
    while (a >= 0 && b < 15) {
        if (array[b][a] == color) {
            num++;
            a--;
            b++;
        } else {
            break;
        }
    }
    a = idxX, b = idxY;
    while (a < 15 && b >= 0) {
        if (array[b][a] == color) {
            num++;
            a++;
            b--;
        } else {
            break;
        }
    }
    showResult(num);
}

function showResult(num) {
    console.info(num);
    if (num >= 6) {
        let msg = "黑棋胜利！";
        if (!flag) {
            msg = "白棋胜利!";
        }
        ctx.beginPath();
        ctx.font = "40px Arial";
        ctx.fillStyle = 'red';
        ctx.fillText(msg, 360, 300);
        ctx.closePath();

        isWin = true;
    }
}

function change() {
    for (let i = 0; i < 15; i++) {
        for (let j = 0; j < 15; j++) {
            let val = array[i][j];
            if (val == null) {
                continue;
            } else if (val == 1) {
                array[i][j] = 0;
                ctx.beginPath();
                ctx.fillStyle = "white";
            } else if (val == 0) {
                array[i][j] = 1;
                ctx.beginPath();
                ctx.fillStyle = "black";
            }
            // 画棋子
            ctx.arc(dx + j * 40, dy + i * 40, 16, 0, 2 * Math.PI);
            ctx.stroke();
            ctx.fill();
            ctx.closePath();
        }
    }
    flag = !flag;
}

function repentChess() {
    if (history.length <= 0) {
        alert("不能悔棋了！");
        return;
    }
    let val = history.pop();
    let idxY = val.split(",")[0];
    let idxX = val.split(",")[1];
    array[idxY][idxX] = null;

    ctx.beginPath();
    ctx.clearRect(dx + idxX * 40 - 20, dy + idxY * 40 - 20, 40, 40);

    // 填补棋盘线条
    ctx.lineWidth = 1;
    ctx.strokeStyle = "black";
    ctx.moveTo(dx + idxX * 40 - 20, dy + idxY * 40);
    ctx.lineTo(dx + idxX * 40 + 20, dy + idxY * 40);
    ctx.moveTo(dx + idxX * 40, dy + idxY * 40 - 20);
    ctx.lineTo(dx + idxX * 40, dy + idxY * 40 + 20);
    ctx.stroke();
    ctx.fill();
    ctx.closePath();

    // 交换棋手
    flag = !flag;
}

function initChess() {
    ctx.clearRect(80, 0, 640, 720);
    // 初始化棋盘
    drawBoard();

    // 初始化棋局
    for (let i = 0; i < 15; i++) {
        array[i] = new Array();
        for (let j = 0; j < 15; j++) {
            array[i][j] = null;
        }
    }
    flag = true; // 黑子先行
    history = [];
    isWin = false;
    isRobot = false;
    robot_robot = false;
}

function saveChess() {
    if (history.length == 0) {
        alert("还没开始下棋，不需保存!");
        return;
    }
    let name = prompt("请输入棋局名称:");
    if (name == null) {
        return;
    }

    // 获取当前时间作为key, 将 array + flag + history + isWin 作为value
    let nameKey = "chess-" + name;
    storage.setItem(nameKey, JSON.stringify(array) + "|" + flag + "|" + JSON.stringify(history) + "|" + isWin);

    alert("保存成功!");
}

function fightWithRobot() {
    isRobot = true;
}

function robotRobot() {
    robot_robot = true;
    $.ajax({
        url: "http://127.0.0.1:5000/start_jiji"
    })
    robotRobotDraw();
}
function robotRobotDraw() {
    var message = String(1) + " " + String(2);
    var data = {
        message: message
    };

    $.getJSON("http://127.0.0.1:5000/ai0_change_to_json", async = false, function (data) {
        var tmp = data.message.split(" ");
        var i = Number(tmp[0]);
        var j = Number(tmp[1]);
        ctx.beginPath();
        if (!flag) {
            array[i][j] = 0;
            ctx.fillStyle = "white";
        }
        else {
            array[i][j] = 1;
            ctx.fillStyle = "black";
        }
        ctx.arc(dx + j * 40, dy + i * 40, 16, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.fill();
        ctx.closePath();

        analyse(j, i, flag);
        flag = !flag;
    });
    $.getJSON("http://127.0.0.1:5000/ai1_change_to_json", async = false, function (data) {
        var tmp = data.message.split(" ");
        var i = Number(tmp[0]);
        var j = Number(tmp[1]);
        ctx.beginPath();
        if (!flag) {
            array[i][j] = 0;
            ctx.fillStyle = "white";
        }
        else {
            array[i][j] = 1;
            ctx.fillStyle = "black";
        }
        ctx.arc(dx + j * 40, dy + i * 40, 16, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.fill();
        ctx.closePath();

        analyse(j, i, flag);
        flag = !flag;
    });
}

function robot_draw(x, y) {
    if (!isWin) {
        console.info(array)
        console.info("robot开始下棋了")

        var message = String(x) + " " + String(y);
        $.ajax({
            url: "http://127.0.0.1:5000/send_message",
            data: {
                message: message
            },
            async: false,
            type: 'GET'
        })

        $.getJSON("http://127.0.0.1:5000/ai1_change_to_json", async = false, function (data) {
            var tmp = data.message.split(" ");
            var i = Number(tmp[0]);
            var j = Number(tmp[1]);

            if (i == -1 && j == -1) {
                change()
                return;
            }
            ctx.beginPath();
            if (!flag) {
                array[i][j] = 0;
                ctx.fillStyle = "white";
            }
            else {
                array[i][j] = 1;
                ctx.fillStyle = "black";
            }
            ctx.arc(dx + j * 40, dy + i * 40, 16, 0, 2 * Math.PI);
            ctx.stroke();
            ctx.fill();
            ctx.closePath();

            analyse(j, i, flag);
            flag = !flag;
        });
    };
}