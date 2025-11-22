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

// Game constants
const int WIDTH = 40;
const int HEIGHT = 20;
const int INITIAL_SPEED = 150;
const int MIN_SPEED = 30;
const int SPEED_DECREMENT = 3;

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };
enum GameState { RUNNING, PAUSED, GAME_OVER, MENU };

class SnakeGame {
private:
    GameState state;
    int score;
    int highScore;
    vector<pair<int, int>> snake;
    vector<pair<int, int>> foods;
    Direction dir;
    int speed;
    int level;
    
    random_device rd;
    mt19937 gen;

    // Double buffering variables
    string buffer;
    HANDLE hConsole;
    DWORD bytesWritten;

public:
    SnakeGame() : gen(rd()), state(MENU), score(0), highScore(0), 
                 speed(INITIAL_SPEED), level(1) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        buffer.reserve((WIDTH + 3) * (HEIGHT + 5)); // Pre-allocate buffer
        initializeGame();
    }

    void initializeGame() {
        score = 0;
        level = 1;
        speed = INITIAL_SPEED;
        dir = STOP;
        snake.clear();
        foods.clear();
        
        int startX = WIDTH / 2;
        int startY = HEIGHT / 2;
        for (int i = 0; i < 3; i++) {
            snake.push_back({startX - i, startY});
        }

        generateFoods(2);
    }

    void generateFoods(int count) {
        vector<pair<int, int>> emptyCells;
        
        for (int x = 1; x < WIDTH - 1; x++) {
            for (int y = 1; y < HEIGHT - 1; y++) {
                if (!isPositionOccupied(x, y)) {
                    emptyCells.push_back({x, y});
                }
            }
        }
        
        shuffle(emptyCells.begin(), emptyCells.end(), gen);
        
        for (int i = 0; i < min(count, (int)emptyCells.size()); i++) {
            foods.push_back(emptyCells[i]);
        }
    }

    bool isPositionOccupied(int x, int y) {
        for (const auto& segment : snake) {
            if (segment.first == x && segment.second == y) return true;
        }
        for (const auto& food : foods) {
            if (food.first == x && food.second == y) return true;
        }
        return false;
    }

    void draw() {
        buffer.clear();
        
        // Draw border
        string border(WIDTH + 2, '#');
        buffer += border + "\n";

        // Draw game area with double buffering
        for (int y = 0; y < HEIGHT; y++) {
            buffer += "#";
            for (int x = 0; x < WIDTH; x++) {
                char output = ' ';
                
                if (x == snake[0].first && y == snake[0].second) 
                    output = 'O';
                else if (isSnakeBody(x, y)) 
                    output = 'o';
                else {
                    for (const auto& food : foods) {
                        if (food.first == x && food.second == y) {
                            output = (food == foods[0]) ? 'F' : '$';
                            break;
                        }
                    }
                }
                buffer += output;
            }
            buffer += "#\n";
        }
        buffer += border + "\n";

        // Draw HUD
        drawHUD();
        
        // Write entire buffer at once to reduce flickering
        COORD coord = {0, 0};
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, ' ', consoleSize, coord, &bytesWritten);
        SetConsoleCursorPosition(hConsole, coord);
        WriteConsole(hConsole, buffer.c_str(), buffer.length(), &bytesWritten, NULL);
    }

    void drawHUD() {
        buffer += "Score: " + to_string(score) + " | High Score: " + to_string(highScore);
        buffer += " | Level: " + to_string(level) + " | Speed: " + to_string((INITIAL_SPEED - speed) / SPEED_DECREMENT) + "\n";
        
        switch (state) {
            case RUNNING: 
                buffer += "WASD: Move | P: Pause | X: Exit\n";
                break;
            case PAUSED: 
                buffer += "*** PAUSED - Press P to resume ***\n";
                break;
            case GAME_OVER: 
                buffer += "*** GAME OVER ***\n";
                buffer += "R: Restart | X: Exit\n";
                break;
            case MENU:
                buffer += "*** SNAKE GAME ***\n";
                buffer += "Press SPACE to start | X: Exit\n";
                break;
        }
    }

    bool isSnakeBody(int x, int y) {
        return any_of(snake.begin() + 1, snake.end(),
                     [x, y](const auto& segment) {
                         return segment.first == x && segment.second == y;
                     });
    }

    void handleInput() {
        if (!_kbhit()) return;
        
        int key = tolower(_getch());
        
        switch (state) {
            case MENU:
                if (key == ' ') state = RUNNING;
                else if (key == 'x') state = GAME_OVER;
                break;
                
            case RUNNING:
                handleRunningInput(key);
                break;
                
            case PAUSED:
                if (key == 'p') state = RUNNING;
                break;
                
            case GAME_OVER:
                if (key == 'r') {
                    initializeGame();
                    state = RUNNING;
                }
                break;
        }
    }

    void handleRunningInput(int key) {
        Direction newDir = dir;
        
        switch (key) {
            case 'a': newDir = LEFT; break;
            case 'd': newDir = RIGHT; break;
            case 'w': newDir = UP; break;
            case 's': newDir = DOWN; break;
            case 'p': state = PAUSED; return;
            case 'x': state = GAME_OVER; return;
        }
        
        if (!isOppositeDirection(newDir)) {
            dir = newDir;
        }
    }

    bool isOppositeDirection(Direction newDir) {
        return (newDir == LEFT && dir == RIGHT) ||
               (newDir == RIGHT && dir == LEFT) ||
               (newDir == UP && dir == DOWN) ||
               (newDir == DOWN && dir == UP);
    }

    void update() {
        if (state != RUNNING || dir == STOP) return;

        static auto lastUpdate = chrono::steady_clock::now();
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastUpdate);
        
        if (elapsed.count() < speed) return;
        lastUpdate = now;

        pair<int, int> newHead = snake[0];
        switch (dir) {
            case LEFT: newHead.first--; break;
            case RIGHT: newHead.first++; break;
            case UP: newHead.second--; break;
            case DOWN: newHead.second++; break;
        }

        if (checkCollision(newHead)) {
            state = GAME_OVER;
            updateHighScore();
            return;
        }

        snake.insert(snake.begin(), newHead);

        bool ateFood = false;
        for (auto it = foods.begin(); it != foods.end();) {
            if (*it == newHead) {
                score += (it == foods.begin()) ? 10 : 20;
                ateFood = true;
                it = foods.erase(it);
                
                if (score >= level * 50) {
                    level++;
                    if (speed > MIN_SPEED) speed -= SPEED_DECREMENT;
                    generateFoods(min(level, 4));
                }
            } else {
                ++it;
            }
        }

        if (!ateFood) {
            snake.pop_back();
        } else if (foods.empty()) {
            generateFoods(min(level + 1, 4));
        }
    }

    bool checkCollision(const pair<int, int>& pos) {
        if (pos.first < 0 || pos.first >= WIDTH || 
            pos.second < 0 || pos.second >= HEIGHT) {
            return true;
        }
        
        return isSnakeBody(pos.first, pos.second);
    }

    void updateHighScore() {
        highScore = max(highScore, score);
    }

    void run() {
        while (state != GAME_OVER || _kbhit()) {
            draw();
            handleInput();
            update();
            this_thread::sleep_for(chrono::milliseconds(10)); // Reduced sleep for smoother rendering
        }
    }
};

int main() {
    SetConsoleTitleA("Advanced Snake Game - Flicker Free");
    system("chcp 65001 > nul");
    
    // Hide cursor for better visual experience
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    SnakeGame game;
    game.run();
    
    return 0;
}