#include <windows.h>
#include <conio.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// Game constants
const int WIDTH = 40;
const int HEIGHT = 20;
const int INITIAL_SPEED = 150;
const int MIN_SPEED = 30;
const int SPEED_DECREMENT = 3;

enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };
enum GameState { RUNNING, PAUSED, GAME_OVER, MENU };

class Logger {
private:
    ofstream logFile;
    chrono::steady_clock::time_point startTime;
    
public:
    Logger() {
        logFile.open("snake_game_log.txt", ios::app);
        startTime = chrono::steady_clock::now();
        log("Game session started");
    }
    
    ~Logger() {
        log("Game session ended");
        logFile.close();
    }
    
    void log(const string& message) {
        auto now = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(now - startTime);
        
        logFile << "[" << duration.count() << "ms] " << message << endl;
    }
    
    void logGameEvent(int score, int level, const string& event) {
        log("Score: " + to_string(score) + " | Level: " + to_string(level) + " | " + event);
    }
};

class ConfigManager {
private:
    string configFile = "snake_config.txt";
    
public:
    struct GameConfig {
        int width = WIDTH;
        int height = HEIGHT;
        int initialSpeed = INITIAL_SPEED;
        int minSpeed = MIN_SPEED;
    };
    
    GameConfig loadConfig() {
        GameConfig config;
        ifstream file(configFile);
        
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                istringstream iss(line);
                string key, value;
                if (getline(iss, key, '=') && getline(iss, value)) {
                    if (key == "width") config.width = stoi(value);
                    else if (key == "height") config.height = stoi(value);
                    else if (key == "initial_speed") config.initialSpeed = stoi(value);
                    else if (key == "min_speed") config.minSpeed = stoi(value);
                }
            }
            file.close();
        }
        
        return config;
    }
    
    void saveConfig(const GameConfig& config) {
        ofstream file(configFile);
        if (file.is_open()) {
            file << "width=" << config.width << endl;
            file << "height=" << config.height << endl;
            file << "initial_speed=" << config.initialSpeed << endl;
            file << "min_speed=" << config.minSpeed << endl;
            file.close();
        }
    }
    
    void createDefaultConfig() {
        GameConfig defaultConfig;
        saveConfig(defaultConfig);
    }
};

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

    // System components
    string buffer;
    HANDLE hConsole;
    DWORD bytesWritten;
    Logger logger;
    ConfigManager configManager;
    ConfigManager::GameConfig config;

    // Performance metrics
    int frameCount;
    chrono::steady_clock::time_point fpsStartTime;
    double averageFPS;

public:
    SnakeGame() : gen(rd()), state(MENU), score(0), highScore(0), 
                 level(1), frameCount(0), averageFPS(0.0) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        
        // Load configuration
        config = configManager.loadConfig();
        if (config.width < 10) configManager.createDefaultConfig();
        
        speed = config.initialSpeed;
        buffer.reserve((config.width + 3) * (config.height + 5));
        
        fpsStartTime = chrono::steady_clock::now();
        initializeGame();
    }

    void initializeGame() {
        score = 0;
        level = 1;
        speed = config.initialSpeed;
        dir = STOP;
        snake.clear();
        foods.clear();
        
        int startX = config.width / 2;
        int startY = config.height / 2;
        for (int i = 0; i < 3; i++) {
            snake.push_back({startX - i, startY});
        }

        generateFoods(2);
        logger.logGameEvent(score, level, "Game initialized");
    }

    void generateFoods(int count) {
        vector<pair<int, int>> emptyCells;
        
        for (int x = 1; x < config.width - 1; x++) {
            for (int y = 1; y < config.height - 1; y++) {
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

    void updateFPS() {
        frameCount++;
        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(currentTime - fpsStartTime);
        
        if (elapsed.count() >= 1) {
            averageFPS = frameCount / elapsed.count();
            frameCount = 0;
            fpsStartTime = currentTime;
        }
    }

    void draw() {
        buffer.clear();
        
        // Draw border
        string border(config.width + 2, '#');
        buffer += border + "\n";

        // Draw game area
        for (int y = 0; y < config.height; y++) {
            buffer += "#";
            for (int x = 0; x < config.width; x++) {
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

        drawHUD();
        
        // Single console write for double buffering
        COORD coord = {0, 0};
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, ' ', consoleSize, coord, &bytesWritten);
        SetConsoleCursorPosition(hConsole, coord);
        WriteConsole(hConsole, buffer.c_str(), buffer.length(), &bytesWritten, NULL);
        
        updateFPS();
    }

    void drawHUD() {
        buffer += "Score: " + to_string(score) + " | High: " + to_string(highScore);
        buffer += " | Level: " + to_string(level) + " | FPS: " + to_string((int)averageFPS) + "\n";
        
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
            case 'p': 
                state = PAUSED;
                logger.log("Game paused");
                break;
            case 'x': 
                state = GAME_OVER;
                logger.log("Game exited by user");
                break;
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
            logger.logGameEvent(score, level, "Game over - collision");
            return;
        }

        snake.insert(snake.begin(), newHead);

        bool ateFood = false;
        for (auto it = foods.begin(); it != foods.end();) {
            if (*it == newHead) {
                score += (it == foods.begin()) ? 10 : 20;
                ateFood = true;
                logger.logGameEvent(score, level, "Food collected");
                it = foods.erase(it);
                
                if (score >= level * 50) {
                    level++;
                    if (speed > config.minSpeed) speed -= SPEED_DECREMENT;
                    generateFoods(min(level, 4));
                    logger.logGameEvent(score, level, "Level up");
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
        if (pos.first < 0 || pos.first >= config.width || 
            pos.second < 0 || pos.second >= config.height) {
            return true;
        }
        
        return isSnakeBody(pos.first, pos.second);
    }

    void updateHighScore() {
        if (score > highScore) {
            highScore = score;
            logger.log("New high score: " + to_string(highScore));
        }
    }

    void run() {
        logger.log("Game loop started");
        while (state != GAME_OVER || _kbhit()) {
            draw();
            handleInput();
            update();
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        logger.log("Game loop ended");
    }
};

int main() {
    SetConsoleTitleA("Professional Snake Game");
    system("chcp 65001 > nul");
    
    // Hide cursor
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    try {
        SnakeGame game;
        game.run();
    } catch (const exception& e) {
        cerr << "Game error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}