#include <windows.h>
#include <conio.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

using namespace std;

// 游戏常量
const int WIDTH = 40;
const int HEIGHT = 20;
const int INITIAL_SPEED = 150;
const int MIN_SPEED = 50;

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };

class SnakeGame {
private:
    bool gameOver;
    bool isPaused;
    int score;
    int highScore;
    vector<pair<int, int>> snake;
    pair<int, int> food;
    Direction dir;
    int speed;
    int foodsEaten;

    random_device rd;
    mt19937 gen;
    uniform_int_distribution<int> distX;
    uniform_int_distribution<int> distY;

public:
    SnakeGame() : gen(rd()), distX(1, WIDTH - 2), distY(1, HEIGHT - 2), 
                 highScore(0), foodsEaten(0), isPaused(false) {
        setup();
    }

    void setup() {
        gameOver = false;
        isPaused = false;
        score = 0;
        foodsEaten = 0;
        speed = INITIAL_SPEED;
        dir = STOP;
        snake.clear();
        
        int startX = WIDTH / 2;
        int startY = HEIGHT / 2;
        for (int i = 0; i < 3; i++) {
            snake.push_back({startX - i, startY});
        }

        generateFood();
    }

    void generateFood() {
        vector<pair<int, int>> emptyCells;
        
        // 收集所有空单元格
        for (int x = 0; x < WIDTH; x++) {
            for (int y = 0; y < HEIGHT; y++) {
                if (!isSnakePosition(x, y)) {
                    emptyCells.push_back({x, y});
                }
            }
        }
        
        if (!emptyCells.empty()) {
            uniform_int_distribution<int> dist(0, emptyCells.size() - 1);
            food = emptyCells[dist(gen)];
        }
    }

    bool isSnakePosition(int x, int y) {
        return any_of(snake.begin(), snake.end(), 
                     [x, y](const auto& segment) {
                         return segment.first == x && segment.second == y;
                     });
    }

    void draw() {
        system("cls");

        // 绘制游戏区域
        for (int i = 0; i < WIDTH + 2; i++) cout << "#";
        cout << endl;

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (x == 0) cout << "#";

                if (x == snake[0].first && y == snake[0].second)
                    cout << "O";
                else if (isSnakeBody(x, y))
                    cout << "o";
                else if (x == food.first && y == food.second)
                    cout << "F";
                else
                    cout << " ";

                if (x == WIDTH - 1) cout << "#";
            }
            cout << endl;
        }

        for (int i = 0; i < WIDTH + 2; i++) cout << "#";
        cout << endl;

        cout << "Score: " << score << " | High Score: " << highScore;
        cout << " | Speed: " << (INITIAL_SPEED - speed) / 2 << endl;
        cout << "WASD: Move | P: Pause | R: Restart | X: Exit" << endl;
        
        if (isPaused) cout << "*** PAUSED ***" << endl;
        if (gameOver) cout << "*** GAME OVER - Press R to restart ***" << endl;
    }

    bool isSnakeBody(int x, int y) {
        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i].first == x && snake[i].second == y)
                return true;
        }
        return false;
    }

    bool isOppositeDirection(Direction newDir) {
        return (newDir == LEFT && dir == RIGHT) ||
               (newDir == RIGHT && dir == LEFT) ||
               (newDir == UP && dir == DOWN) ||
               (newDir == DOWN && dir == UP);
    }

    bool checkCollision(const pair<int, int>& head) {
        if (head.first < 0 || head.first >= WIDTH || 
            head.second < 0 || head.second >= HEIGHT) {
            return true;
        }
        
        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i] == head) return true;
        }
        
        return false;
    }

    void input() {
        if (_kbhit()) {
            int key = _getch();
            
            switch (tolower(key)) {
                case 'a': if (!isOppositeDirection(LEFT)) dir = LEFT; break;
                case 'd': if (!isOppositeDirection(RIGHT)) dir = RIGHT; break;
                case 'w': if (!isOppositeDirection(UP)) dir = UP; break;
                case 's': if (!isOppositeDirection(DOWN)) dir = DOWN; break;
                case 'x': gameOver = true; break;
                case 'r': if (gameOver) setup(); break;
                case 'p': isPaused = !isPaused; break;
            }
        }
    }

    void logic() {
        if (isPaused || dir == STOP || gameOver) return;

        pair<int, int> newHead = snake[0];
        switch (dir) {
            case LEFT: newHead.first--; break;
            case RIGHT: newHead.first++; break;
            case UP: newHead.second--; break;
            case DOWN: newHead.second++; break;
        }

        if (checkCollision(newHead)) {
            gameOver = true;
            updateHighScore();
            return;
        }

        snake.insert(snake.begin(), newHead);

        if (newHead == food) {
            score += 10;
            foodsEaten++;
            generateFood();
            if (speed > MIN_SPEED && foodsEaten % 3 == 0) {
                speed -= 5;
            }
        } else {
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
            draw();
            input();
            logic();
            this_thread::sleep_for(chrono::milliseconds(speed));
        }
        draw();
        cout << "Final Score: " << score << " - Press any key to exit..." << endl;
        _getch();
    }
};

int main() {
    SetConsoleTitleA("Enhanced Snake Game");
    system("chcp 65001 > nul");
    SnakeGame game;
    game.run();
    return 0;
}