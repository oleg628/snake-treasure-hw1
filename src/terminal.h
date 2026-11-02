/**
 * @file terminal.h
 * @brief Кроссплатформенный ввод с клавиатуры и управление экраном.
 */

#ifndef SNAKE_TERMINAL_H_
#define SNAKE_TERMINAL_H_

namespace snake::terminal {

/// Клавиши, которые понимает игра.
enum class Key { kUp, kDown, kLeft, kRight, kHelp, kQuit, kRestart, kEnter, kOther, kEof };

/// true, если стандартный ввод подключён к терминалу.
bool is_tty();

/// Переключение терминала в неканонический режим без эха.
class RawMode {
   public:
    RawMode();
    ~RawMode();
    RawMode(const RawMode&) = delete;
    RawMode& operator=(const RawMode&) = delete;

   private:
    bool active_{false};
    void* state_{nullptr};
};

/// Прочитать одну клавишу (для стрелок — escape-последовательность).
Key read_key();

/// Очистить экран, если вывод идёт в терминал.
void clear_screen();

}  // namespace snake::terminal

#endif  // SNAKE_TERMINAL_H_
