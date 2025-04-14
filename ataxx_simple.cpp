#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>

static std::chrono::steady_clock::time_point globalStart;
static double globalTimeLimit;

using namespace std::chrono;
using namespace std;

int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 2, whitePieceCount = 2;
int startX, startY, resultX, resultY;
static int delta[24][2] = {
    { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
    { -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
    { 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
    { 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
    { -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
    { 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 }
};

// 判断是否在地图内
inline bool inMap(int x, int y)
{
    if (x < 0 || x > 6 || y < 0 || y > 6)
        return false;
    return true;
}

// 向Direction方向改动坐标，并返回是否越界
inline bool MoveStep(int &x, int &y, int Direction)
{
    x = x + delta[Direction][0];
    y = y + delta[Direction][1];
    return inMap(x, y);
}

// 在坐标处落子，检查是否合法或模拟落子
bool ProcStep(int x0, int y0, int x1, int y1, int color)
{
    if (color == 0)
        return false;
    if (x1 == -1) // 无路可走，跳过此回合
        return true;
    if (!inMap(x0, y0) || !inMap(x1, y1)) // 超出边界
        return false;
    if (gridInfo[x0][y0] != color)
        return false;
    int dx, dy, x, y, currCount = 0, dir;

    dx = abs((x0 - x1));
    dy = abs((y0 - y1));

    // 保证不会移动到原来位置，而且移动始终在5×5区域内
    if ((dx == 0 && dy == 0) || dx > 2 || dy > 2)
        return false;

    // 保证移动到的位置为空
    if (gridInfo[x1][y1] != 0)
        return false;

    // 如果走的是5×5的外围，则不是复制粘贴
    if (dx == 2 || dy == 2)
        gridInfo[x0][y0] = 0;
    else {
        if (color == 1)
            blackPieceCount++;
        else
            whitePieceCount++;
    }

    gridInfo[x1][y1] = color;

    // 翻转周围8个格子的对方棋子
    for (dir = 0; dir < 8; dir++) {
        x = x1 + delta[dir][0];
        y = y1 + delta[dir][1];
        if (!inMap(x, y))
            continue;
        if (gridInfo[x][y] == -color) {
            currCount++;
            gridInfo[x][y] = color;
        }
    }

    // 更新棋子计数
    if (currCount != 0) {
        if (color == 1) {
            blackPieceCount += currCount;
            whitePieceCount -= currCount;
        } else {
            whitePieceCount += currCount;
            blackPieceCount -= currCount;
        }
    }
    return true;
}

// 估值函数: 返回我方(currBotColor)视角下的分值
int EvaluateBoard(int botColor)
{
    if (botColor == 1)
        return blackPieceCount - whitePieceCount;
    else
        return whitePieceCount - blackPieceCount;
}

// 检查某颜色是否无路可走
bool HasNoMove(int color)
{
    for (int yy = 0; yy < 7; yy++) {
        for (int xx = 0; xx < 7; xx++) {
            if (gridInfo[xx][yy] != color) continue;
            for(int dd=0; dd<24; dd++) {
                int nx = xx + delta[dd][0];
                int ny = yy + delta[dd][1];
                if(inMap(nx, ny) && gridInfo[nx][ny] == 0)
                    return false;
            }
        }
    }
    return true;
}

// Alpha-Beta搜索函数
int AlphaBetaSearch(int depth, int alpha, int beta, bool maximizingPlayer, int color, int originalBotColor)
{
    // 若无路可走或达到深度，则返回估值
    if (depth == 0 || HasNoMove(color)) {
        return EvaluateBoard(originalBotColor);
    }

    // 收集所有走法
    int localBegin[1000][2], localPos[1000][2];
    int lCount = 0;
    for (int yy = 0; yy < 7; yy++) {
        for (int xx = 0; xx < 7; xx++) {
            if (gridInfo[xx][yy] != color) continue;
            for(int dd=0; dd<24; dd++) {
                int nx = xx + delta[dd][0];
                int ny = yy + delta[dd][1];
                if(inMap(nx, ny) && gridInfo[nx][ny] == 0) {
                    localBegin[lCount][0] = xx;
                    localBegin[lCount][1] = yy;
                    localPos[lCount][0] = nx;
                    localPos[lCount][1] = ny;
                    lCount++;
                }
            }
        }
    }
    // 若依旧没有走法
    if(lCount == 0) {
        return EvaluateBoard(originalBotColor);
    }

    // 备份
    int oldBlack = blackPieceCount;
    int oldWhite = whitePieceCount;
    int backupGrid[7][7];
    memcpy(backupGrid, gridInfo, sizeof(backupGrid));

    int bestVal = maximizingPlayer ? -9999999 : 9999999;

    // 遍历走法
    for(int i = 0; i < lCount; i++) {
        bool ok = ProcStep(localBegin[i][0], localBegin[i][1], localPos[i][0], localPos[i][1], color);
        if(!ok) continue;

        int val = AlphaBetaSearch(depth - 1, alpha, beta, !maximizingPlayer, -color, originalBotColor);

        // 恢复
        memcpy(gridInfo, backupGrid, sizeof(gridInfo));
        blackPieceCount = oldBlack;
        whitePieceCount = oldWhite;

        if(maximizingPlayer) {
            if(val > bestVal) bestVal = val;
            if(val > alpha) alpha = val;
            if(beta <= alpha) break;
        } else {
            if(val < bestVal) bestVal = val;
            if(val < beta) beta = val;
            if(beta <= alpha) break;
        }
    }
    return bestVal;
}

int main()
{
    istream::sync_with_stdio(false);

    int turnID;
    cin >> turnID;

    // 根据回合号决定时间限制（第一回合 2s，其余 1s）
    if (turnID == 1) {
        globalTimeLimit = 2.0;
    } else {
        globalTimeLimit = 1.0;
    }

    globalStart = std::chrono::steady_clock::now();

    int x0, y0, x1, y1;

    // 初始化棋盘
    gridInfo[0][0] = gridInfo[6][6] = 1;  //黑
    gridInfo[6][0] = gridInfo[0][6] = -1; //白

    currBotColor = -1; // 假定我方是白方

    // 分析自己收到的输入和自己过往的输出，并恢复状态
    for (int i = 0; i < turnID - 1; i++) {
        // 根据这些输入输出逐渐恢复状态到当前回合
        cin >> x0 >> y0 >> x1 >> y1;
        if (x1 >= 0)
            ProcStep(x0, y0, x1, y1, -currBotColor); // 模拟对方落子
        else
            currBotColor = 1; // 第一回合收到坐标是-1, -1, -1, -1 说明我是黑方

        cin >> x0 >> y0 >> x1 >> y1;
        if (x1 >= 0)
            ProcStep(x0, y0, x1, y1, currBotColor); // 模拟己方落子
    }

    // 看看自己本回合输入，即对方上一轮的决策结果
    cin >> x0 >> y0 >> x1 >> y1;
    if (x1 >= 0)
        ProcStep(x0, y0, x1, y1, -currBotColor);
    else
        currBotColor = 1;

    // 此时gridInfo[][]里存储的就是当前棋盘的所有棋子信息

    // 找出合法落子点
    int beginPos[1000][2], possiblePos[1000][2], posCount = 0, dir;
    for (y0 = 0; y0 < 7; y0++) {
        for (x0 = 0; x0 < 7; x0++) {
            if (gridInfo[x0][y0] != currBotColor)
                continue;
            for (dir = 0; dir < 24; dir++) {
                x1 = x0 + delta[dir][0];
                y1 = y0 + delta[dir][1];
                if (!inMap(x1, y1))
                    continue;
                if (gridInfo[x1][y1] != 0)
                    continue;
                beginPos[posCount][0] = x0;
                beginPos[posCount][1] = y0;
                possiblePos[posCount][0] = x1;
                possiblePos[posCount][1] = y1;
                posCount++;
            }
        }
    }

    // 做出决策（你只需修改以下部分）下面仅为示例代码，可删除
    {
        int searchDepth = 5; // 可以根据需要调整深度

        // 若无路可走
        if (posCount == 0) {
            startX = -1;
            startY = -1;
            resultX = -1;
            resultY = -1;
        } else {
            int bestScore = -9999999;
            for (int i = 0; i < posCount; i++) {
                // 备份
                int tmpGrid[7][7];
                memcpy(tmpGrid, gridInfo, sizeof(tmpGrid));
                int bCount = blackPieceCount;
                int wCount = whitePieceCount;

                bool ok = ProcStep(beginPos[i][0], beginPos[i][1], possiblePos[i][0], possiblePos[i][1], currBotColor);
                if(!ok) continue;

                double usedSec = duration_cast<milliseconds>(std::chrono::steady_clock::now() - globalStart).count() / 1000.0;
                if(usedSec > globalTimeLimit * 0.95) {
                    memcpy(gridInfo, tmpGrid, sizeof(gridInfo));
                    blackPieceCount = bCount;
                    whitePieceCount = wCount;
                    break;
                }

                int val = AlphaBetaSearch(searchDepth - 1, -9999999, 9999999, false, -currBotColor, currBotColor);

                memcpy(gridInfo, tmpGrid, sizeof(gridInfo));
                blackPieceCount = bCount;
                whitePieceCount = wCount;

                if (val > bestScore) {
                    bestScore = val;
                    startX = beginPos[i][0];
                    startY = beginPos[i][1];
                    resultX = possiblePos[i][0];
                    resultY = possiblePos[i][1];
                }
            }

            if (startX < 0) {
                startX = -1;
                startY = -1;
                resultX = -1;
                resultY = -1;
            }
        }
    }
    // 决策结束

    cout << startX << " " << startY << " " << resultX << " " << resultY;
    return 0;
}
