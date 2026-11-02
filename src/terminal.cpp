/**
 * @file terminal.cpp
 * @brief Реализация ввода с клавиатуры для Linux/WSL и Windows.
 */

#include "terminal.h"

#include <cctype>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace snake::terminal {
namespace {

Key map_character(int character) {
    switch (std::tolower(static_cast<unsigned char>(character))) {
        case 'w':
            return Key::kUp;
        case 's':
            return Key::kDown;
        case 'a':
            return Key::kLeft;
        case 'd':
            return Key::kRight;
        case 'h':
            return Key::kHelp;
        case 'q':
            return Key::kQuit;
        case 'r':
            return Key::kRestart;
        case '\n':
        case '\r':
        case ' ':
            return Key::kEnter;
        default:
            return Key::kOther;
    }
}

#ifndef _WIN32

Key read_arrow_sequence() {
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(STDIN_FILENO, &descriptors);
    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;

    const int ready = select(STDIN_FILENO + 1, &descriptors, nullptr, nullptr, &timeout);
    if (ready <= 0) {
        return Key::kOther;
    }

    unsigned char bracket = 0;
    if (read(STDIN_FILENO, &bracket, 1) != 1 || bracket != '[') {
        return Key::kOther;
    }

    unsigned char code = 0;
    if (read(STDIN_FILENO, &code, 1) != 1) {
        return Key::kOther;
    }

    switch (code) {
        case 'A':
            return Key::kUp;
        case 'B':
            return Key::kDown;
        case 'C':
            return Key::kRight;
        case 'D':
            return Key::kLeft;
        default:
            return Key::kOther;
    }
}

#endif

}  // namespace

bool is_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

#ifndef _WIN32

RawMode::RawMode() {
    if (!is_tty()) {
        return;
    }

    auto* saved = new termios{};
    if (tcgetattr(STDIN_FILENO, saved) != 0) {
        delete saved;
        return;
    }

    termios raw = *saved;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        delete saved;
        return;
    }

    state_ = saved;
    active_ = true;
}

RawMode::~RawMode() {
    if (active_ && state_ != nullptr) {
        tcsetattr(STDIN_FILENO, TCSANOW, static_cast<termios*>(state_));
    }
    delete static_cast<termios*>(state_);
}

Key read_key() {
    unsigned char character = 0;
    ssize_t count = 0;
    do {
        count = read(STDIN_FILENO, &character, 1);
    } while (count < 0 && errno == EINTR);

    if (count <= 0) {
        return Key::kEof;
    }
    if (character == 27) {
        return read_arrow_sequence();
    }
    return map_character(character);
}

void clear_screen() {
    if (is_tty()) {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }
}

#else

RawMode::RawMode() = default;
RawMode::~RawMode() = default;

Key read_key() {
    const int character = _getch();
    if (character == 0 || character == 224) {
        const int code = _getch();
        switch (code) {
            case 72:
                return Key::kUp;
            case 80:
                return Key::kDown;
            case 75:
                return Key::kLeft;
            case 77:
                return Key::kRight;
            default:
                return Key::kOther;
        }
    }
    if (character == 3) {
        return Key::kQuit;
    }
    return map_character(character);
}

void clear_screen() {
    std::system("cls");
}

#endif

}  // namespace snake::terminal
