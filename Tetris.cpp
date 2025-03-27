#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

using namespace std;

const int WIDTH = 10;
const int HEIGHT = 20;
const string EMPTY = "  ";

// Tetromino shapes and colors
const string COLORS[7] = {"🟦", "🟨", "🟪", "🟩", "🟥", "🟧", "⬜"};
const int tetrominoes[7][4][4] = {
    // I
    {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // O
    {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // T
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // S
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // Z
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // J
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // L
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
};

class Tetromino {
public:
    int shape[4][4];
    int x, y;
    string color;

    Tetromino(int type) : color(COLORS[type]) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shape[i][j] = tetrominoes[type][i][j];
        x = WIDTH / 2 - 2;
        y = 0;
    }

    void rotate() {
        int temp[4][4] = {0};
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                temp[j][3 - i] = shape[i][j];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shape[i][j] = temp[i][j];
    }
};

class GameBoard {
public:
    string grid[HEIGHT][WIDTH];
    int score;
    int level;
    int linesCleared;

    GameBoard() : score(0), level(1), linesCleared(0) {
        for (int i = 0; i < HEIGHT; i++)
            for (int j = 0; j < WIDTH; j++)
                grid[i][j] = EMPTY;
    }

    void draw(Tetromino &t) {
        cout << "\033[H\033[J"; // Clear screen
        for (int i = 0; i < HEIGHT; i++) {
            cout << "|";
            for (int j = 0; j < WIDTH; j++) {
                string displayChar = grid[i][j];
                // Overlay current Tetromino if it's within this cell
                for (int ti = 0; ti < 4; ti++) {
                    for (int tj = 0; tj < 4; tj++) {
                        if (t.shape[ti][tj] && (i == t.y + ti) && (j == t.x + tj))
                            displayChar = t.color;
                    }
                }
                cout << displayChar;
            }
            cout << "|\n";
        }
        cout << " ";
        for (int i = 0; i < WIDTH; i++) cout << "--";
        cout << "\n";
        cout << "Score: " << score << "  Level: " << level << "  Lines: " << linesCleared << "\n";
        cout << "[A] Left  [D] Right  [S] Down  [W] Rotate  [Space] Hard Drop\n";
        cout << "[P] Pause  [R] Restart  [X] Exit\n";
    }

    bool canMove(Tetromino &t, int dx, int dy) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (t.shape[i][j]) {
                    int newX = t.x + j + dx;
                    int newY = t.y + i + dy;
                    if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || (newY >= 0 && grid[newY][newX] != EMPTY))
                        return false;
                }
        return true;
    }

    void placeTetromino(Tetromino &t) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (t.shape[i][j])
                    grid[t.y + i][t.x + j] = t.color;
        clearLines();
    }

    void clearLines() {
        int fullLines = 0;
        for (int i = 0; i < HEIGHT; i++) {
            bool full = true;
            for (int j = 0; j < WIDTH; j++)
                if (grid[i][j] == EMPTY) {
                    full = false;
                    break;
                }
            if (full) {
                fullLines++;
                // Shift all rows above downward
                for (int k = i; k > 0; k--)
                    for (int j = 0; j < WIDTH; j++)
                        grid[k][j] = grid[k - 1][j];
                // Clear top row
                for (int j = 0; j < WIDTH; j++)
                    grid[0][j] = EMPTY;
            }
        }
        // Scoring: 1 line = 100, 2 lines = 300, 3 lines = 500, 4 lines = 800
        score += fullLines == 1 ? 100 : fullLines == 2 ? 300 : fullLines == 3 ? 500 : fullLines == 4 ? 800 : 0;
        linesCleared += fullLines;
        if (linesCleared >= level * 10) level++;
    }

    void clearBoard() {
        for (int i = 0; i < HEIGHT; i++)
            for (int j = 0; j < WIDTH; j++)
                grid[i][j] = EMPTY;
        score = 0;
        level = 1;
        linesCleared = 0;
    }
};

class TetrisGame {
public:
    GameBoard board;
    Tetromino *currentPiece;
    bool gameOver;
    bool isPaused;

    TetrisGame() : gameOver(false), isPaused(false) {
        srand(time(0));
        currentPiece = new Tetromino(rand() % 7);
    }

    ~TetrisGame() {
        delete currentPiece;
    }

    void run() {
        while (!gameOver) {
            handleInput();
            if (!isPaused) {
                update();
            }
            board.draw(*currentPiece);
            // Adjust speed based on level; lower sleep time for higher levels
            usleep(500000 / board.level);
        }
        cout << "Game Over!" << endl;
    }

private:
    char getInput() {
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        int bytesAvailable;
        ioctl(STDIN_FILENO, FIONREAD, &bytesAvailable);
        char key = 0;
        if (bytesAvailable > 0) {
            read(STDIN_FILENO, &key, 1);
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return key;
    }

    void handleInput() {
        char key = getInput();
        if (key == 'p' || key == 'P') {
            isPaused = !isPaused;
            if (isPaused) {
                cout << "Game Paused. Press 'p' to resume...\n";
            }
        }
        if (isPaused) return;
        if (key == 'a' || key == 'A') {
            if (board.canMove(*currentPiece, -1, 0))
                currentPiece->x--;
        } else if (key == 'd' || key == 'D') {
            if (board.canMove(*currentPiece, 1, 0))
                currentPiece->x++;
        } else if (key == 's' || key == 'S') {
            if (board.canMove(*currentPiece, 0, 1))
                currentPiece->y++;
        } else if (key == 'w' || key == 'W') {
            currentPiece->rotate();
            // Optional: Check if rotation results in collision; if so, undo.
        } else if (key == ' ') {
            hardDrop();
        } else if (key == 'r' || key == 'R') {
            restartGame();
        } else if (key == 'x' || key == 'X') {
            exit(0);
        }
    }

    void update() {
        if (!board.canMove(*currentPiece, 0, 1)) {
            board.placeTetromino(*currentPiece);
            delete currentPiece;
            // If the new piece immediately collides, game over.
            currentPiece = new Tetromino(rand() % 7);
            if (!board.canMove(*currentPiece, 0, 0)) {
                gameOver = true;
            }
        } else {
            currentPiece->y++;
        }
    }

    void hardDrop() {
        while (board.canMove(*currentPiece, 0, 1)) {
            currentPiece->y++;
            board.score++; // Add a bonus point per row dropped (optional)
        }
        update();
    }

    void restartGame() {
        board.clearBoard();
        delete currentPiece;
        currentPiece = new Tetromino(rand() % 7);
        gameOver = false;
        isPaused = false;
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
