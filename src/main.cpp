/**
 * @file main.cpp
 * @brief Консольный интерфейс игры «Змейка: Сокровищница».
 */

#include "snake_game.h"
#include "terminal.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    bool demo{false};
    bool base{false};
    bool quiet{false};
    bool no_color{false};
    bool help{false};
    bool version{false};
    bool seed_set{false};
    unsigned seed{0U};
    int max_turns{120};
    int delay_ms{0};
};

void print_usage(std::ostream& out) {
    out << "Использование: snake_game [параметры]\n"
        << "  --base              базовая игра без сундуков\n"
        << "  --demo              автоматическая демонстрация без ввода\n"
        << "  --seed N            детерминированное расположение объектов\n"
        << "  --max-turns N       лимит ходов демонстрации (по умолчанию 120)\n"
        << "  --delay MS          пауза между ходами демонстрации\n"
        << "  --quiet             печатать только итог демонстрации\n"
        << "  --no-color          отключить ANSI-цвета\n"
        << "  --help, -h          показать эту справку и правила\n"
        << "  --version           показать версию\n";
}

void print_rules(std::ostream& out, snake::Mode mode) {
    out << "============================================================\n"
        << "                 ЗМЕЙКА: СОКРОВИЩНИЦА\n"
        << "              Домашнее задание №1, вариант 25\n"
        << "============================================================\n\n";

    if (mode == snake::Mode::kTreasure) {
        out << "Цель игры — найти сундук, в котором спрятано золото.\n\n"
            << "На поле 10 × 10 находятся пять закрытых сундуков (C).\n"
            << "Только в одном из них лежит золото. Содержимое сундука\n"
            << "становится известно только после того, как змейка встанет\n"
            << "на его клетку:\n"
            << "  • пустой сундук: −1 очко, игра продолжается;\n"
            << "  • сундук с золотом: +10 очков и немедленная победа.\n\n";
    } else {
        out << "Это базовая версия игры без сундуков.\n"
            << "Цель — собирать еду и как можно дольше не сталкиваться\n"
            << "со стеной или собственным хвостом.\n\n";
    }

    out << "Обычная еда (*) даёт +1 очко и увеличивает длину змейки на 1.\n"
        << "Столкновение со стеной или собственным хвостом завершает игру\n"
        << "поражением. После игры выводится итоговый счёт.\n\n";

    if (mode == snake::Mode::kTreasure) {
        out << "Обозначения поля:\n"
            << "  @ — голова змейки,  o — тело,  * — еда,\n"
            << "  C — закрытый сундук, . — свободная клетка.\n\n";
    } else {
        out << "Обозначения поля:\n"
            << "  @ — голова змейки,  o — тело,  * — еда,\n"
            << "  . — свободная клетка.\n\n";
    }

    out << "Управление:\n"
        << "  W / A / S / D или стрелки — сделать шаг;\n"
        << "  H — открыть справку (игра приостанавливается);\n"
        << "  Q — выйти из игры;\n"
        << "  R — начать новую игру после победы или поражения.\n\n"
        << "Перед началом игры нажмите Enter. Во время игры справка\n"
        << "вызывается клавишей H, как указано в правилах.\n"
        << "============================================================\n";
}

bool parse_int(const std::string& text, int& value) {
    try {
        std::size_t consumed = 0;
        const long parsed = std::stol(text, &consumed);
        if (consumed != text.size() || parsed < 0 || parsed > 1000000L) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_unsigned(const std::string& text, unsigned& value) {
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(text, &consumed);
        if (consumed != text.size() || parsed > 4294967295UL) {
            return false;
        }
        value = static_cast<unsigned>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<snake::Direction> key_to_direction(snake::terminal::Key key) {
    switch (key) {
        case snake::terminal::Key::kUp:
            return snake::Direction::kUp;
        case snake::terminal::Key::kDown:
            return snake::Direction::kDown;
        case snake::terminal::Key::kLeft:
            return snake::Direction::kLeft;
        case snake::terminal::Key::kRight:
            return snake::Direction::kRight;
        default:
            return std::nullopt;
    }
}

std::string cell_symbol(snake::Cell cell) {
    switch (cell) {
        case snake::Cell::kSnakeHead:
            return "@";
        case snake::Cell::kSnakeBody:
            return "o";
        case snake::Cell::kFood:
            return "*";
        case snake::Cell::kChest:
            return "C";
        case snake::Cell::kEmpty:
            return ".";
    }
    return "?";
}

std::string colored_cell(snake::Cell cell, bool color) {
    const std::string symbol = cell_symbol(cell);
    if (!color) {
        return symbol;
    }
    switch (cell) {
        case snake::Cell::kSnakeHead:
            return "\033[1;32m" + symbol + "\033[0m";
        case snake::Cell::kSnakeBody:
            return "\033[32m" + symbol + "\033[0m";
        case snake::Cell::kFood:
            return "\033[1;31m" + symbol + "\033[0m";
        case snake::Cell::kChest:
            return "\033[1;33m" + symbol + "\033[0m";
        case snake::Cell::kEmpty:
            return symbol;
    }
    return symbol;
}

void render(const snake::Game& game, std::ostream& out, bool color) {
    out << "Счёт: " << game.score() << "   Длина: " << game.snake().size();
    if (game.mode() == snake::Mode::kTreasure) {
        out << "   Сундуки: " << game.opened_chests() << "/" << game.chest_count();
    }
    out << "   Ходы: " << game.turns() << "\n";
    out << "Последнее событие: " << game.last_event() << "\n";
    out << "+----------+\n";
    const snake::Board board = game.board();
    for (int y = 0; y < snake::kHeight; ++y) {
        out << "|";
        for (int x = 0; x < snake::kWidth; ++x) {
            out << colored_cell(board[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)],
                                color);
        }
        out << "|\n";
    }
    out << "+----------+\n";
    out << "W/A/S/D или стрелки — шаг, H — справка, Q — выход\n\n";
}

void print_result(const snake::Game& game) {
    if (game.status() == snake::Status::kWon) {
        std::cout << "ПОБЕДА! Золото найдено в сундуке №" << game.gold_chest_number() << ".\n";
    } else if (game.status() == snake::Status::kLost) {
        std::cout << "ПОРАЖЕНИЕ. Причина: " << snake::to_string(game.loss_reason()) << ".\n";
    } else {
        std::cout << "Игра завершена пользователем.\n";
    }
    std::cout << "Итоговый счёт: " << game.score() << ", ходов: " << game.turns();
    if (game.mode() == snake::Mode::kTreasure) {
        std::cout << ", открыто сундуков: " << game.opened_chests() << "/" << game.chest_count();
    }
    std::cout << ".\n";
}

void wait_for_any_key() {
    std::cout << "Нажмите любую клавишу, чтобы продолжить...";
    std::cout.flush();
    (void)snake::terminal::read_key();
}

void run_interactive(const Options& options) {
    const snake::Mode mode = options.base ? snake::Mode::kBase : snake::Mode::kTreasure;
    snake::terminal::RawMode raw_mode;
    print_rules(std::cout, mode);
    std::cout << "Нажмите Enter, чтобы начать, H — показать правила, Q — выйти: ";
    std::cout.flush();

    while (true) {
        const snake::terminal::Key key = snake::terminal::read_key();
        if (key == snake::terminal::Key::kQuit || key == snake::terminal::Key::kEof) {
            std::cout << "\nВыход.\n";
            return;
        }
        if (key == snake::terminal::Key::kHelp) {
            print_rules(std::cout, mode);
            std::cout << "Нажмите Enter, чтобы начать, Q — выйти: ";
            std::cout.flush();
            continue;
        }
        if (key == snake::terminal::Key::kEnter) {
            break;
        }
    }

    snake::Game game(options.seed_set ? options.seed : std::random_device{}(), mode);
    while (true) {
        snake::terminal::clear_screen();
        render(game, std::cout, !options.no_color && snake::terminal::is_tty());

        if (game.status() != snake::Status::kRunning) {
            print_result(game);
            std::cout << "R — новая игра, Q — выход: ";
            std::cout.flush();
            const snake::terminal::Key key = snake::terminal::read_key();
            if (key == snake::terminal::Key::kQuit || key == snake::terminal::Key::kEof) {
                std::cout << "\nВыход.\n";
                return;
            }
            if (key == snake::terminal::Key::kRestart) {
                game.reset(options.seed_set ? options.seed : std::random_device{}(), mode);
            }
            continue;
        }

        const snake::terminal::Key key = snake::terminal::read_key();
        if (key == snake::terminal::Key::kQuit || key == snake::terminal::Key::kEof) {
            std::cout << "\n";
            print_result(game);
            return;
        }
        if (key == snake::terminal::Key::kHelp) {
            print_rules(std::cout, mode);
            wait_for_any_key();
            continue;
        }

        const auto direction = key_to_direction(key);
        if (direction.has_value()) {
            game.set_direction(*direction);
            game.step();
        }
    }
}

/// Проверить, свободна ли клетка для демонстрационного бота.
bool is_blocked_for_bot(const snake::Game& game, snake::Point point, snake::Point target) {
    if (!snake::is_inside(point)) {
        return true;
    }

    const auto& body = game.snake();
    for (std::size_t i = 0; i + 1U < body.size(); ++i) {
        if (body[i] == point) {
            return true;
        }
    }

    for (int i = 0; i < game.chest_count(); ++i) {
        const snake::Chest& chest = game.chests()[static_cast<std::size_t>(i)];
        if (!chest.opened && chest.position != target && chest.position == point) {
            return true;
        }
    }
    return false;
}

std::optional<snake::Direction> first_step_to(const snake::Game& game, snake::Point target) {
    const snake::Point head = game.snake().front();
    if (head == target) {
        return std::nullopt;
    }

    const std::array<snake::Direction, 4> directions = {
        snake::Direction::kUp, snake::Direction::kDown, snake::Direction::kLeft,
        snake::Direction::kRight};

    int first_direction[snake::kHeight][snake::kWidth];
    bool visited[snake::kHeight][snake::kWidth];
    std::fill(&first_direction[0][0], &first_direction[0][0] + snake::kHeight * snake::kWidth, -1);
    std::fill(&visited[0][0], &visited[0][0] + snake::kHeight * snake::kWidth, false);

    std::queue<snake::Point> queue;
    visited[head.y][head.x] = true;
    queue.push(head);

    while (!queue.empty()) {
        const snake::Point current = queue.front();
        queue.pop();
        for (std::size_t i = 0; i < directions.size(); ++i) {
            const snake::Direction direction = directions[i];
            snake::Point next = current;
            switch (direction) {
                case snake::Direction::kUp:
                    --next.y;
                    break;
                case snake::Direction::kDown:
                    ++next.y;
                    break;
                case snake::Direction::kLeft:
                    --next.x;
                    break;
                case snake::Direction::kRight:
                    ++next.x;
                    break;
            }

            if (!snake::is_inside(next) || visited[next.y][next.x] ||
                is_blocked_for_bot(game, next, target)) {
                continue;
            }

            visited[next.y][next.x] = true;
            first_direction[next.y][next.x] =
                (current == head) ? static_cast<int>(i) : first_direction[current.y][current.x];
            if (next == target) {
                return directions[static_cast<std::size_t>(first_direction[next.y][next.x])];
            }
            queue.push(next);
        }
    }
    return std::nullopt;
}

bool is_opposite_direction(snake::Direction lhs, snake::Direction rhs) {
    return (lhs == snake::Direction::kUp && rhs == snake::Direction::kDown) ||
           (lhs == snake::Direction::kDown && rhs == snake::Direction::kUp) ||
           (lhs == snake::Direction::kLeft && rhs == snake::Direction::kRight) ||
           (lhs == snake::Direction::kRight && rhs == snake::Direction::kLeft);
}

std::optional<snake::Direction> bot_direction(const snake::Game& game) {
    const snake::Point head = game.snake().front();
    std::vector<int> chest_order;
    for (int i = 0; i < game.chest_count(); ++i) {
        if (!game.chests()[static_cast<std::size_t>(i)].opened) {
            chest_order.push_back(i);
        }
    }
    std::sort(chest_order.begin(), chest_order.end(), [&](int lhs, int rhs) {
        const snake::Point a = game.chests()[static_cast<std::size_t>(lhs)].position;
        const snake::Point b = game.chests()[static_cast<std::size_t>(rhs)].position;
        const int da = std::abs(a.x - head.x) + std::abs(a.y - head.y);
        const int db = std::abs(b.x - head.x) + std::abs(b.y - head.y);
        return da < db;
    });

    for (int index : chest_order) {
        const auto direction =
            first_step_to(game, game.chests()[static_cast<std::size_t>(index)].position);
        if (direction.has_value() && !is_opposite_direction(game.direction(), *direction)) {
            return direction;
        }
    }

    if (game.has_food()) {
        const auto direction = first_step_to(game, game.food());
        if (direction.has_value() && !is_opposite_direction(game.direction(), *direction)) {
            return direction;
        }
    }

    const std::array<snake::Direction, 4> fallbacks = {
        snake::Direction::kUp, snake::Direction::kDown, snake::Direction::kLeft,
        snake::Direction::kRight};
    for (snake::Direction direction : fallbacks) {
        if (is_opposite_direction(game.direction(), direction)) {
            continue;
        }
        snake::Point next = head;
        switch (direction) {
            case snake::Direction::kUp:
                --next.y;
                break;
            case snake::Direction::kDown:
                ++next.y;
                break;
            case snake::Direction::kLeft:
                --next.x;
                break;
            case snake::Direction::kRight:
                ++next.x;
                break;
        }
        if (!snake::is_inside(next)) {
            continue;
        }
        bool occupied = false;
        const auto& body = game.snake();
        for (std::size_t i = 0; i + 1U < body.size(); ++i) {
            if (body[i] == next) {
                occupied = true;
            }
        }
        if (!occupied) {
            return direction;
        }
    }
    return std::nullopt;
}

void run_demo(const Options& options) {
    const unsigned seed = options.seed_set ? options.seed : std::random_device{}();
    const snake::Mode mode = options.base ? snake::Mode::kBase : snake::Mode::kTreasure;
    snake::Game game(seed, mode);
    const bool color = !options.no_color && snake::terminal::is_tty();

    if (!options.quiet) {
        std::cout << "Демонстрация «Змейка: Сокровищница», seed = " << seed
                  << ", режим: " << (mode == snake::Mode::kTreasure ? "вариант 25" : "базовая игра")
                  << "\n\n";
        render(game, std::cout, color);
    }

    while (game.status() == snake::Status::kRunning && game.turns() < options.max_turns) {
        const auto direction = bot_direction(game);
        if (!direction.has_value()) {
            break;
        }
        if (!game.set_direction(*direction)) {
            break;
        }
        game.step();

        if (!options.quiet && (game.turns() % 5 == 0 || game.status() != snake::Status::kRunning ||
                               game.turns() == options.max_turns)) {
            snake::terminal::clear_screen();
            render(game, std::cout, color);
        }

        if (options.delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(options.delay_ms));
        }
    }

    if (game.status() == snake::Status::kRunning) {
        if (!options.quiet) {
            render(game, std::cout, color);
        }
        std::cout << "Демонстрация остановлена: достигнут лимит ходов (" << options.max_turns
                  << "), счёт: " << game.score() << ".\n";
    } else {
        print_result(game);
    }
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--version") {
            options.version = true;
        } else if (argument == "--demo") {
            options.demo = true;
        } else if (argument == "--base") {
            options.base = true;
        } else if (argument == "--quiet") {
            options.quiet = true;
        } else if (argument == "--no-color") {
            options.no_color = true;
        } else if (argument == "--seed") {
            if (i + 1 >= argc || !parse_unsigned(argv[++i], options.seed)) {
                std::cerr << "Ошибка: для --seed требуется целое число.\n";
                return 2;
            }
            options.seed_set = true;
        } else if (argument == "--max-turns") {
            if (i + 1 >= argc || !parse_int(argv[++i], options.max_turns)) {
                std::cerr << "Ошибка: для --max-turns требуется целое число.\n";
                return 2;
            }
        } else if (argument == "--delay") {
            if (i + 1 >= argc || !parse_int(argv[++i], options.delay_ms)) {
                std::cerr << "Ошибка: для --delay требуется целое число.\n";
                return 2;
            }
        } else {
            std::cerr << "Неизвестный параметр: " << argument << "\n\n";
            print_usage(std::cerr);
            return 2;
        }
    }

    if (options.help) {
        print_usage(std::cout);
        std::cout << "\n";
        print_rules(std::cout, options.base ? snake::Mode::kBase : snake::Mode::kTreasure);
        return 0;
    }
    if (options.version) {
        std::cout << "snake_game 1.0.0 (домашнее задание №1, вариант 25)\n";
        return 0;
    }
    if (options.demo) {
        run_demo(options);
        return 0;
    }
    if (!snake::terminal::is_tty()) {
        std::cerr << "Интерактивный режим требует терминал.\n"
                  << "Для автоматического запуска используйте --demo.\n";
        return 2;
    }
    run_interactive(options);
    return 0;
}
