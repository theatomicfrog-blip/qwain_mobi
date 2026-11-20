#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
using namespace std;

static termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    cout << "\033[0m\033[?25h";
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    cout << "\033[?25l";
}

bool keyReady() {
    timeval tv{0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

char readKey() {
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return 0;
}

void clear() { cout << "\033[2J\033[H"; }

struct Alien { int x, y; };

int main(int argc, char** argv) {
    if (argc > 1 && string(argv[1]) != "--egg") return 0;

    enableRawMode();

    const int W = 32, H = 18;
    int px = W / 2, py = H - 2;
    int bx = -1, by = -1;
    int score = 0;
    bool quit = false;

    vector<Alien> aliens;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 8; ++c)
            aliens.push_back({4 + c * 3, 2 + r * 2});

    int dir = 1;
    int tick = 0;

    while (!quit) {
        while (keyReady()) {
            char c = readKey();
            if (c == 'q') quit = true;
            else if (c == 'a' && px > 1) --px;
            else if (c == 'd' && px < W - 2) ++px;
            else if (c == ' ' && bx == -1) { bx = px; by = py - 1; }
        }

        if (++tick % 2 == 0) {
            if (bx != -1) {
                --by;
                if (by <= 0) bx = by = -1;
            }

            for (auto it = aliens.begin(); it != aliens.end(); ) {
                if (bx != -1 && it->x == bx && it->y == by) {
                    it = aliens.erase(it);
                    bx = by = -1;
                    score += 10;
                } else {
                    ++it;
                }
            }

            bool edge = false;
            for (auto &a : aliens) {
                a.x += dir;
                if (a.x <= 1 || a.x >= W - 2) edge = true;
            }
            if (edge) {
                dir = -dir;
                for (auto &a : aliens) a.y++;
            }

            for (auto &a : aliens)
                if (a.y >= py) quit = true;

            if (aliens.empty()) quit = true;
        }

        clear();
        cout << "ASCII Invaders  score:" << score << "   A/D move  SPACE shoot  Q quit\n";

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                char ch = ' ';
                if (y == 0 || y == H - 1 || x == 0 || x == W - 1) ch = '#';
                if (x == px && y == py) ch = '^';
                if (bx == x && by == y) ch = '|';
                for (auto &a : aliens) if (a.x == x && a.y == y) ch = 'M';
                cout << ch;
            }
            cout << '\n';
        }

        usleep(60000);
    }

    clear();
    cout << (aliens.empty() ? "YOU WIN!\n" : "GAME OVER\n");
    cout << "Final score: " << score << "\n";
    return 0;
}
