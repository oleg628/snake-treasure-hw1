/**
 * @file snake_game.h
 * @brief Игровая логика «Змейки» и варианта 25 «Сокровищница».
 *
 * Модуль не зависит от консоли: он предоставляет детерминированное игровое
 * состояние, которое можно проверить модульными тестами и отрисовать любым
 * способом. Это разделение позволяет тестировать правила без ввода-вывода.
 */

#ifndef SNAKE_GAME_H_
#define SNAKE_GAME_H_

#include <array>
#include <cstddef>
#include <deque>
#include <random>
#include <string>

namespace snake {

/// Ширина игрового поля в клетках.
inline constexpr int kWidth = 10;
/// Высота игрового поля в клетках.
inline constexpr int kHeight = 10;
/// Количество сундуков в варианте 25.
inline constexpr int kChestCount = 5;

/**
 * @brief Координата клетки поля.
 *
 * x — столбец (0…9), y — строка (0…9). Начало координат находится в левом
 * верхнем углу поля.
 */
struct Point {
    int x{0};  ///< Столбец.
    int y{0};  ///< Строка.
};

/// Проверка совпадения двух координат.
inline bool operator==(Point lhs, Point rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/// Проверка различия двух координат.
inline bool operator!=(Point lhs, Point rhs) {
    return !(lhs == rhs);
}

/// Режим игры: базовая версия или вариант 25.
enum class Mode { kBase, kTreasure };

/// Направление движения змейки.
enum class Direction { kUp, kDown, kLeft, kRight };

/// Тип клетки, используемый только при отрисовке поля.
enum class Cell { kEmpty, kSnakeHead, kSnakeBody, kFood, kChest };

/// Текущее состояние игры.
enum class Status { kRunning, kWon, kLost };

/// Причина завершения игры.
enum class LossReason { kNone, kWall, kSelf, kQuit };

/**
 * @brief Сундук на поле.
 *
 * Содержимое сундука скрыто от игрока до момента открытия. Ровно у одного
 * сундука has_gold == true.
 */
struct Chest {
    Point position{};      ///< Координата сундука.
    bool has_gold{false};  ///< Признак золота (не показывается игроку).
    bool opened{false};    ///< Был ли сундук уже открыт.
};

/// Снимок поля для отрисовки.
using Board = std::array<std::array<Cell, kWidth>, kHeight>;

/**
 * @brief Игра «Змейка» с вариантом 25 «Сокровищница».
 *
 * Базовая механика: змейка стартует в центре поля, ест еду, растёт и
 * проигрывает при столкновении со стеной или собственным хвостом. В варианте
 * 25 на поле дополнительно находятся пять сундуков, только в одном из которых
 * лежит золото. Пустой сундук отнимает одно очко, сундук с золотом даёт десять
 * очков и немедленно завершает игру победой.
 */
class Game {
   public:
    /**
     * @brief Создать игру со случайным начальным положением.
     *
     * @throws std::bad_alloc Если не удалось выделить память под контейнеры.
     */
    Game();

    /**
     * @brief Создать игру с детерминированным генератором случайных чисел.
     * @param seed Начальное значение генератора.
     * @param mode Режим: базовая игра или вариант 25.
     * @throws std::bad_alloc Если не удалось выделить память под контейнеры.
     */
    explicit Game(unsigned seed, Mode mode = Mode::kTreasure);

    /**
     * @brief Восстановить игру из заданного состояния.
     *
     * Метод используется тестами и может применяться для загрузки сохранённой
     * игры. Координаты змейки, еды и сундуков должны быть корректны.
     *
     * @param snake Тело змейки, начиная с головы.
     * @param direction Текущее направление.
     * @param food Координата еды.
     * @param chests Массив из пяти сундуков.
     * @param score Текущий счёт.
     * @param seed Начальное значение генератора для будущих ходов.
     * @return Новое игровое состояние.
     * @throws std::invalid_argument Если состояние нарушает правила.
     */
    static Game from_state(std::deque<Point> snake, Direction direction, Point food,
                           std::array<Chest, kChestCount> chests, int score, unsigned seed = 0U);

    /**
     * @brief Начать игру заново со случайным расположением объектов.
     * @throws std::bad_alloc Если не удалось выделить память под контейнеры.
     */
    void reset();

    /**
     * @brief Начать игру заново с детерминированным генератором.
     * @param seed Начальное значение генератора.
     * @param mode Режим: базовая игра или вариант 25.
     * @throws std::bad_alloc Если не удалось выделить память под контейнеры.
     */
    void reset(unsigned seed, Mode mode = Mode::kTreasure);

    /**
     * @brief Задать направление следующего хода.
     *
     * Разворот на 180 градусов запрещён: попытка развернуться игнорируется.
     *
     * @param direction Желаемое направление.
     * @return true, если направление принято; false, если это разворот.
     */
    bool set_direction(Direction direction);

    /**
     * @brief Выполнить один ход.
     *
     * Один вызов перемещает голову на одну клетку, обрабатывает еду, сундук,
     * столкновение со стеной или хвостом. После завершения игры вызовы step()
     * ничего не меняют.
     *
     * @return Состояние игры после хода.
     */
    Status step();

    /// Текущий режим игры.
    Mode mode() const {
        return mode_;
    }

    /// Количество сундуков в текущем режиме (0 для базовой игры).
    int chest_count() const {
        return chest_count_;
    }

    /// Текущее состояние игры.
    Status status() const {
        return status_;
    }

    /// Причина завершения игры (или LossReason::kNone).
    LossReason loss_reason() const {
        return loss_reason_;
    }

    /// Текущий счёт. Пустые сундуки могут сделать его отрицательным.
    int score() const {
        return score_;
    }

    /// Количество выполненных ходов.
    int turns() const {
        return turns_;
    }

    /// Текущее направление движения.
    Direction direction() const {
        return direction_;
    }

    /// Тело змейки: первый элемент — голова, последний — хвост.
    const std::deque<Point>& snake() const {
        return snake_;
    }

    /// Массив сундуков (содержимое доступно тестам и отрисовке).
    const std::array<Chest, kChestCount>& chests() const {
        return chests_;
    }

    /// Признак наличия еды на поле.
    bool has_food() const {
        return has_food_;
    }

    /// Координата еды; имеет смысл только при has_food() == true.
    Point food() const {
        return food_;
    }

    /// Количество открытых сундуков.
    int opened_chests() const;

    /// Количество ещё не открытых сундуков.
    int remaining_chests() const;

    /// Номер сундука с золотом после победы (1…5) или 0.
    int gold_chest_number() const {
        return gold_chest_number_;
    }

    /// Последнее игровое событие для строки состояния.
    const std::string& last_event() const {
        return last_event_;
    }

    /**
     * @brief Построить снимок поля для отрисовки.
     * @return Поле 10 × 10 с типами клеток.
     */
    Board board() const;

   private:
    void place_random_objects(unsigned seed);
    void spawn_food();
    void finish_loss(LossReason reason);
    int find_unopened_chest(Point point) const;
    bool occupied_by_snake(Point point) const;
    bool occupied_by_chest(Point point) const;

    std::mt19937 rng_;
    Mode mode_{Mode::kTreasure};
    int chest_count_{kChestCount};
    std::deque<Point> snake_;
    std::array<Chest, kChestCount> chests_{};
    Point food_{};
    bool has_food_{false};
    Direction direction_{Direction::kRight};
    Status status_{Status::kRunning};
    LossReason loss_reason_{LossReason::kNone};
    int score_{0};
    int turns_{0};
    int gold_chest_number_{0};
    std::string last_event_;
};

/// Строковое представление направления (для отладки и отчётов).
std::string to_string(Direction direction);

/// Строковое представление состояния игры.
std::string to_string(Status status);

/// Строковое представление причины завершения.
std::string to_string(LossReason reason);

/**
 * @brief Проверить, находится ли точка внутри поля.
 * @param point Проверяемая координата.
 * @return true, если координата принадлежит полю 10 × 10.
 */
bool is_inside(Point point);

}  // namespace snake

#endif  // SNAKE_GAME_H_
