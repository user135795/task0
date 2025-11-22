#include <windows.h>
#include <conio.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <iostream>

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
    vector<pair<int, int>> snake; // 蛇身坐标
    pair<int, int> food;          // 食物坐标
    Direction dir;
    int speed;

    // 随机数生成器
    random_device rd;
    mt19937 gen;
    uniform_int_distribution<int> distX;
    uniform_int_distribution<int> distY;

public:
    SnakeGame() : gen(rd()), distX(1, WIDTH - 2), distY(1, HEIGHT - 2) {
        setup();
    }

    void setup() {
        gameOver = false;
        score = 0;
        speed = INITIAL_SPEED;
        dir = STOP;

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
        cout << "得分: " << score << endl;
        cout << "使用WASD控制方向，按X退出游戏" << endl;
        
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

    void input() {
        if (_kbhit()) {
            switch (_getch()) {
                case 'a': case 'A':
                    if (dir != RIGHT) dir = LEFT;
                    break;
                case 'd': case 'D':
                    if (dir != LEFT) dir = RIGHT;
                    break;
                case 'w': case 'W':
                    if (dir != DOWN) dir = UP;
                    break;
                case 's': case 'S':
                    if (dir != UP) dir = DOWN;
                    break;
                case 'x': case 'X':
                    gameOver = true;
                    break;
                case 'r': case 'R':
                    if (gameOver) setup();
                    break;
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
        if (newHead.first <= 0 || newHead.first >= WIDTH - 1 ||
            newHead.second < 0 || newHead.second >= HEIGHT) {
            gameOver = true;
            return;
        }

        // 检查碰撞自身
        for (size_t i = 0; i < snake.size(); i++) {
            if (snake[i] == newHead) {
                gameOver = true;
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

    void run() {
        while (!gameOver) {
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