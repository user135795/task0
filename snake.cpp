#include <windows.h>
#include <conio.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

// 游戏常量
const int WIDTH = 40;
const int HEIGHT = 20;
const int INITIAL_SPEED = 150; // 毫秒

// 方向枚举
enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };

class SnakeGame {
private:
    bool gameOver;
    int score;
    int highScore;
    vector<pair<int, int>> snake; // 蛇身坐标
    pair<int, int> food;          // 食物坐标
    Direction dir;
    int speed;

    // 随机数生成器
    random_device rd;
    mt19937 gen;
    uniform_int_distribution<int> distX;
    uniform_int_distribution<int> distY;

    // 性能统计
    int frameCount;
    chrono::steady_clock::time_point lastFpsTime;
    double fps;

public:
    SnakeGame() : gen(rd()), distX(1, WIDTH - 2), distY(1, HEIGHT - 2), 
                 highScore(0), frameCount(0), fps(0.0) {
        setup();
    }

    void setup() {
        gameOver = false;
        score = 0;
        speed = INITIAL_SPEED;
        dir = STOP;
        lastFpsTime = chrono::steady_clock::now();

        // 初始化蛇身（3节）
        snake.clear();
        snake.push_back({WIDTH / 2, HEIGHT / 2});     // 蛇头
        snake.push_back({WIDTH / 2 - 1, HEIGHT / 2}); // 蛇身
        snake.push_back({WIDTH / 2 - 2, HEIGHT / 2}); // 蛇身

        // 生成第一个食物
        generateFood();
    }

    void generateFood() {
        while (true) {
            food = {distX(gen), distY(gen)};
            
            // 检查食物是否生成在蛇身上
            bool onSnake = false;
            for (const auto& segment : snake) {
                if (segment == food) {
                    onSnake = true;
                    break;
                }
            }
            
            if (!onSnake) break;
        }
    }

    void updateFPS() {
        frameCount++;
        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(currentTime - lastFpsTime);
        
        if (elapsed.count() >= 1) {
            fps = frameCount / elapsed.count();
            frameCount = 0;
            lastFpsTime = currentTime;
        }
    }

    void draw() {
        system("cls"); // 清屏

        // 绘制上边界
        for (int i = 0; i < WIDTH + 2; i++)
            cout << "#";
        cout << endl;

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                // 绘制左边界
                if (x == 0)
                    cout << "#";

                // 绘制蛇头
                if (x == snake[0].first && y == snake[0].second)
                    cout << "O";
                // 绘制蛇身
                else if (isSnakeBody(x, y))
                    cout << "o";
                // 绘制食物
                else if (x == food.first && y == food.second)
                    cout << "F";
                // 绘制空白
                else
                    cout << " ";

                // 绘制右边界
                if (x == WIDTH - 1)
                    cout << "#";
            }
            cout << endl;
        }

        // 绘制下边界
        for (int i = 0; i < WIDTH + 2; i++)
            cout << "#";
        cout << endl;

        // 显示分数和提示
        cout << "得分: " << score << "  最高分: " << highScore;
        cout << "  FPS: " << static_cast<int>(fps) << endl;
        cout << "使用WASD控制方向，P暂停，X退出游戏" << endl;
        
        if (gameOver) {
            cout << "游戏结束! 按R重新开始" << endl;
        }
    }

    bool isSnakeBody(int x, int y) {
        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i].first == x && snake[i].second == y)
                return true;
        }
        return false;
    }

    // 检查是否为相反方向
    bool isOppositeDirection(Direction newDir, Direction currentDir) {
        return (newDir == LEFT && currentDir == RIGHT) ||
               (newDir == RIGHT && currentDir == LEFT) ||
               (newDir == UP && currentDir == DOWN) ||
               (newDir == DOWN && currentDir == UP);
    }

    void input() {
        if (_kbhit()) {
            int key = _getch();
            Direction newDir = dir;
            
            switch (tolower(key)) {
                case 'a': newDir = LEFT; break;
                case 'd': newDir = RIGHT; break;
                case 'w': newDir = UP; break;
                case 's': newDir = DOWN; break;
                case 'x': 
                    gameOver = true;
                    break;
                case 'r': 
                    if (gameOver) setup();
                    break;
                case 'p': // 暂停功能
                    cout << "游戏暂停，按P继续..." << endl;
                    while (true) {
                        if (_kbhit() && _getch() == 'p') break;
                        this_thread::sleep_for(chrono::milliseconds(100));
                    }
                    break;
            }
            
            // 防止反向移动
            if (!isOppositeDirection(newDir, dir)) {
                dir = newDir;
            }
        }
    }

    void logic() {
        if (dir == STOP || gameOver) return;

        // 保存蛇头位置
        pair<int, int> newHead = snake[0];

        // 根据方向移动蛇头
        switch (dir) {
            case LEFT:  newHead.first--; break;
            case RIGHT: newHead.first++; break;
            case UP:    newHead.second--; break;
            case DOWN:  newHead.second++; break;
        }

        // 检查碰撞边界
        if (newHead.first < 0 || newHead.first >= WIDTH ||
            newHead.second < 0 || newHead.second >= HEIGHT) {
            gameOver = true;
            updateHighScore();
            return;
        }

        // 检查碰撞自身
        for (size_t i = 0; i < snake.size(); i++) {
            if (snake[i] == newHead) {
                gameOver = true;
                updateHighScore();
                return;
            }
        }

        // 移动蛇身
        snake.insert(snake.begin(), newHead);

        // 检查是否吃到食物
        if (newHead == food) {
            score += 10;
            generateFood();
            // 随着分数增加，速度加快
            if (speed > 50) speed -= 2;
        } else {
            // 如果没有吃到食物，移除尾部
            snake.pop_back();
        }
    }

    void updateHighScore() {
        if (score > highScore) {
            highScore = score;
        }
    }

    void run() {
        while (!gameOver) {
            updateFPS();
            draw();
            input();
            logic();
            this_thread::sleep_for(chrono::milliseconds(speed));
        }
        
        // 游戏结束后的显示
        draw();
        cout << "最终得分: " << score << endl;
        cout << "按任意键退出..." << endl;
        _getch();
    }
};

int main() {
    // 设置控制台窗口标题
    SetConsoleTitleA("贪吃蛇游戏");
    
    // 设置控制台编码为UTF-8以支持中文
    system("chcp 65001 > nul");
    
    SnakeGame game;
    game.run();
    
    return 0;
}