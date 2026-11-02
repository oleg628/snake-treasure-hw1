/**
 * @file game.cpp
 * @brief Реализация игровых правил «Змейки» и варианта 25.
 */

#include "snake_game.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace snake {
namespace {

bool is_opposite(Direction lhs, Direction rhs) {
    return (lhs == Direction::kUp && rhs == Direction::kDown) ||
           (lhs == Direction::kDown && rhs == Direction::kUp) ||
           (lhs == Direction::kLeft && rhs == Direction::kRight) ||
           (lhs == Direction::kRight && rhs == Direction::kLeft);
}

Point next_point(Point point, Direction direction) {
    switch (direction) {
        case Direction::kUp:
            return {point.x, point.y - 1};
        case Direction::kDown:
            return {point.x, point.y + 1};
        case Direction::kLeft:
            return {point.x - 1, point.y};
        case Direction::kRight:
            return {point.x + 1, point.y};
    }
    return point;
}

}  // namespace

bool is_inside(Point point) {
    return point.x >= 0 && point.x < kWidth && point.y >= 0 && point.y < kHeight;
}

Game::Game() {
    std::random_device random;
    reset(random(), Mode::kTreasure);
}

Game::Game(unsigned seed, Mode mode) {
    reset(seed, mode);
}

Game Game::from_state(std::deque<Point> snake, Direction direction, Point food,
                      std::array<Chest, kChestCount> chests, int score, unsigned seed) {
    if (snake.empty()) {
        throw std::invalid_argument("Змейка не может быть пустой");
    }

    for (std::size_t i = 0; i < snake.size(); ++i) {
        if (!is_inside(snake[i])) {
            throw std::invalid_argument("Сегмент змейки вне поля");
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (snake[i] == snake[j]) {
                throw std::invalid_argument("Сегменты змейки совпадают");
            }
        }
    }

    int gold_count = 0;
    for (std::size_t i = 0; i < chests.size(); ++i) {
        if (!is_inside(chests[i].position)) {
            throw std::invalid_argument("Сундук находится вне поля");
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (chests[i].position == chests[j].position) {
                throw std::invalid_argument("Два сундука стоят в одной клетке");
            }
        }
        if (chests[i].has_gold) {
            ++gold_count;
        }
        if (!chests[i].opened &&
            std::find(snake.begin(), snake.end(), chests[i].position) != snake.end()) {
            throw std::invalid_argument("Закрытый сундук стоит на змейке");
        }
    }

    if (gold_count != 1) {
        throw std::invalid_argument("На поле должен быть ровно один золотой сундук");
    }
    if (!is_inside(food)) {
        throw std::invalid_argument("Еда находится вне поля");
    }
    if (std::find(snake.begin(), snake.end(), food) != snake.end()) {
        throw std::invalid_argument("Еда стоит на змейке");
    }
    for (const Chest& chest : chests) {
        if (!chest.opened && chest.position == food) {
            throw std::invalid_argument("Еда стоит на закрытом сундуке");
        }
    }

    Game game(seed, Mode::kTreasure);
    game.mode_ = Mode::kTreasure;
    game.chest_count_ = kChestCount;
    game.snake_ = std::move(snake);
    game.chests_ = chests;
    game.food_ = food;
    game.has_food_ = true;
    game.direction_ = direction;
    game.score_ = score;
    game.turns_ = 0;
    game.status_ = Status::kRunning;
    game.loss_reason_ = LossReason::kNone;
    game.gold_chest_number_ = 0;
    game.last_event_ = "Состояние восстановлено";
    return game;
}

void Game::reset() {
    std::random_device random;
    reset(random(), Mode::kTreasure);
}

void Game::reset(unsigned seed, Mode mode) {
    mode_ = mode;
    chest_count_ = (mode_ == Mode::kTreasure) ? kChestCount : 0;
    rng_.seed(seed);
    snake_.clear();
    snake_.push_back({kWidth / 2, kHeight / 2});
    direction_ = Direction::kRight;
    status_ = Status::kRunning;
    loss_reason_ = LossReason::kNone;
    score_ = 0;
    turns_ = 0;
    gold_chest_number_ = 0;
    last_event_ = "Игра началась";
    place_random_objects(seed);
}

void Game::place_random_objects(unsigned /*seed*/) {
    std::vector<Point> free_cells;
    free_cells.reserve(kWidth * kHeight);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const Point point{x, y};
            if (!occupied_by_snake(point)) {
                free_cells.push_back(point);
            }
        }
    }

    std::shuffle(free_cells.begin(), free_cells.end(), rng_);
    if (free_cells.size() < static_cast<std::size_t>(chest_count_) + 1U) {
        throw std::runtime_error("Недостаточно свободных клеток для сундуков и еды");
    }

    chests_.fill(Chest{});
    for (int i = 0; i < chest_count_; ++i) {
        chests_[static_cast<std::size_t>(i)].position = free_cells[static_cast<std::size_t>(i)];
    }

    if (chest_count_ > 0) {
        std::uniform_int_distribution<int> gold_distribution(0, chest_count_ - 1);
        const int gold_index = gold_distribution(rng_);
        chests_[static_cast<std::size_t>(gold_index)].has_gold = true;
    }

    food_ = free_cells[static_cast<std::size_t>(chest_count_)];
    has_food_ = true;
}

void Game::spawn_food() {
    std::vector<Point> free_cells;
    free_cells.reserve(kWidth * kHeight);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const Point point{x, y};
            if (!occupied_by_snake(point) && !occupied_by_chest(point)) {
                free_cells.push_back(point);
            }
        }
    }

    if (free_cells.empty()) {
        has_food_ = false;
        food_ = {};
        return;
    }

    std::uniform_int_distribution<std::size_t> distribution(0U, free_cells.size() - 1U);
    food_ = free_cells[distribution(rng_)];
    has_food_ = true;
}

bool Game::set_direction(Direction direction) {
    if (is_opposite(direction_, direction)) {
        return false;
    }
    direction_ = direction;
    return true;
}

Status Game::step() {
    if (status_ != Status::kRunning) {
        return status_;
    }

    ++turns_;
    const Point next = next_point(snake_.front(), direction_);

    if (!is_inside(next)) {
        finish_loss(LossReason::kWall);
        last_event_ = "Столкновение со стеной";
        return status_;
    }

    const bool grows = has_food_ && next == food_;
    const int chest_index = find_unopened_chest(next);

    for (std::size_t i = 0; i < snake_.size(); ++i) {
        const bool is_tail = (i + 1U == snake_.size());
        if (is_tail && !grows) {
            continue;
        }
        if (snake_[i] == next) {
            finish_loss(LossReason::kSelf);
            last_event_ = "Столкновение с хвостом";
            return status_;
        }
    }

    snake_.push_front(next);
    if (!grows) {
        snake_.pop_back();
    }

    if (grows) {
        ++score_;
        has_food_ = false;
        last_event_ = "Съедена еда: +1 очко";
        spawn_food();
    }

    if (chest_index >= 0) {
        Chest& chest = chests_[static_cast<std::size_t>(chest_index)];
        chest.opened = true;
        if (chest.has_gold) {
            score_ += 10;
            status_ = Status::kWon;
            loss_reason_ = LossReason::kNone;
            gold_chest_number_ = chest_index + 1;
            last_event_ = "Найдено золото: +10 очков, победа!";
        } else {
            --score_;
            last_event_ = "Пустой сундук: -1 очко";
        }
    }

    return status_;
}

void Game::finish_loss(LossReason reason) {
    status_ = Status::kLost;
    loss_reason_ = reason;
}

int Game::find_unopened_chest(Point point) const {
    for (int i = 0; i < chest_count_; ++i) {
        const Chest& chest = chests_[static_cast<std::size_t>(i)];
        if (!chest.opened && chest.position == point) {
            return i;
        }
    }
    return -1;
}

bool Game::occupied_by_snake(Point point) const {
    return std::find(snake_.begin(), snake_.end(), point) != snake_.end();
}

bool Game::occupied_by_chest(Point point) const {
    for (int i = 0; i < chest_count_; ++i) {
        const Chest& chest = chests_[static_cast<std::size_t>(i)];
        if (!chest.opened && chest.position == point) {
            return true;
        }
    }
    return false;
}

int Game::opened_chests() const {
    int count = 0;
    for (int i = 0; i < chest_count_; ++i) {
        if (chests_[static_cast<std::size_t>(i)].opened) {
            ++count;
        }
    }
    return count;
}

int Game::remaining_chests() const {
    return chest_count_ - opened_chests();
}

Board Game::board() const {
    Board result{};
    for (auto& row : result) {
        row.fill(Cell::kEmpty);
    }

    for (int i = 0; i < chest_count_; ++i) {
        const Chest& chest = chests_[static_cast<std::size_t>(i)];
        if (!chest.opened) {
            result[static_cast<std::size_t>(chest.position.y)]
                  [static_cast<std::size_t>(chest.position.x)] = Cell::kChest;
        }
    }

    if (has_food_) {
        result[static_cast<std::size_t>(food_.y)][static_cast<std::size_t>(food_.x)] = Cell::kFood;
    }

    for (std::size_t i = 0; i < snake_.size(); ++i) {
        const Point point = snake_[i];
        result[static_cast<std::size_t>(point.y)][static_cast<std::size_t>(point.x)] =
            (i == 0U) ? Cell::kSnakeHead : Cell::kSnakeBody;
    }

    return result;
}

std::string to_string(Direction direction) {
    switch (direction) {
        case Direction::kUp:
            return "вверх";
        case Direction::kDown:
            return "вниз";
        case Direction::kLeft:
            return "влево";
        case Direction::kRight:
            return "вправо";
    }
    return "неизвестно";
}

std::string to_string(Status status) {
    switch (status) {
        case Status::kRunning:
            return "идёт";
        case Status::kWon:
            return "победа";
        case Status::kLost:
            return "поражение";
    }
    return "неизвестно";
}

std::string to_string(LossReason reason) {
    switch (reason) {
        case LossReason::kNone:
            return "нет";
        case LossReason::kWall:
            return "стена";
        case LossReason::kSelf:
            return "хвост";
        case LossReason::kQuit:
            return "выход";
    }
    return "неизвестно";
}

}  // namespace snake
