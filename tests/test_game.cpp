/**
 * @file test_game.cpp
 * @brief Модульные тесты игровых правил «Змейки: Сокровищница».
 */

#include "snake_game.h"

#include <array>
#include <deque>
#include <ostream>
#include <string>

#include <gtest/gtest.h>

namespace snake {
std::ostream& operator<<(std::ostream& out, Point point) {
    return out << "(" << point.x << ", " << point.y << ")";
}
}  // namespace snake

namespace {

using snake::Chest;
using snake::Direction;
using snake::Game;
using snake::LossReason;
using snake::Mode;
using snake::Point;
using snake::Status;

const std::array<Point, 5> kFarChests = {Point{0, 0}, Point{9, 0}, Point{0, 9}, Point{9, 9},
                                         Point{8, 8}};

std::array<Chest, snake::kChestCount> far_chests(int gold_index = 0) {
    std::array<Chest, snake::kChestCount> chests{};
    for (int i = 0; i < snake::kChestCount; ++i) {
        chests[static_cast<std::size_t>(i)].position = kFarChests[static_cast<std::size_t>(i)];
        chests[static_cast<std::size_t>(i)].has_gold = (i == gold_index);
    }
    return chests;
}

std::array<Chest, snake::kChestCount> chests_with_target(Point target, bool has_gold) {
    std::array<Chest, snake::kChestCount> chests = far_chests(has_gold ? 0 : 1);
    chests[0].position = target;
    chests[0].has_gold = has_gold;
    if (has_gold) {
        chests[1].has_gold = false;
    }
    return chests;
}

Game make_game(std::deque<Point> body, Direction direction, Point food,
               std::array<Chest, snake::kChestCount> chests, int score = 0) {
    return Game::from_state(std::move(body), direction, food, chests, score);
}

Game centered_right_game(std::array<Chest, snake::kChestCount> chests, Point food = Point{9, 5}) {
    return make_game({{5, 5}}, Direction::kRight, food, std::move(chests));
}

TEST(GameTest, InitialStateHasValidField) {
    const Game game(25U);

    EXPECT_EQ(game.status(), Status::kRunning);
    EXPECT_EQ(game.score(), 0);
    EXPECT_EQ(game.turns(), 0);
    ASSERT_EQ(game.snake().size(), 1U);
    EXPECT_EQ(game.snake().front(), (Point{5, 5}));
    EXPECT_EQ(game.direction(), Direction::kRight);
    EXPECT_TRUE(game.has_food());
    EXPECT_EQ(game.opened_chests(), 0);
    EXPECT_EQ(game.remaining_chests(), snake::kChestCount);
}

TEST(GameTest, InitialObjectsAreUniqueAndDoNotOverlap) {
    const Game game(25U);
    int gold_count = 0;
    for (std::size_t i = 0; i < game.chests().size(); ++i) {
        const Chest& chest = game.chests()[i];
        EXPECT_TRUE(snake::is_inside(chest.position));
        EXPECT_NE(chest.position, (Point{5, 5}));
        EXPECT_NE(chest.position, game.food());
        if (chest.has_gold) {
            ++gold_count;
        }
        for (std::size_t j = 0; j < i; ++j) {
            EXPECT_NE(chest.position, game.chests()[j].position);
        }
    }
    EXPECT_EQ(gold_count, 1);
    EXPECT_FALSE(game.snake().front() == game.food());
}

TEST(GameTest, BaseModeHasNoChests) {
    const Game game(25U, Mode::kBase);
    EXPECT_EQ(game.mode(), Mode::kBase);
    EXPECT_EQ(game.chest_count(), 0);
    EXPECT_EQ(game.opened_chests(), 0);
    EXPECT_EQ(game.remaining_chests(), 0);

    const auto board = game.board();
    int chests = 0;
    int food = 0;
    for (const auto& row : board) {
        for (snake::Cell cell : row) {
            if (cell == snake::Cell::kChest) {
                ++chests;
            } else if (cell == snake::Cell::kFood) {
                ++food;
            }
        }
    }
    EXPECT_EQ(chests, 0);
    EXPECT_EQ(food, 1);
}

TEST(GameTest, SeedProducesSameLayout) {
    const Game first(25U);
    const Game second(25U);

    EXPECT_EQ(first.food(), second.food());
    EXPECT_EQ(first.board(), second.board());
    for (std::size_t i = 0; i < first.chests().size(); ++i) {
        EXPECT_EQ(first.chests()[i].position, second.chests()[i].position);
        EXPECT_EQ(first.chests()[i].has_gold, second.chests()[i].has_gold);
    }
}

TEST(GameTest, BoardContainsExactlyOneHeadFiveChestsAndOneFood) {
    const Game game(25U);
    const auto board = game.board();
    int heads = 0;
    int bodies = 0;
    int chests = 0;
    int food = 0;
    for (const auto& row : board) {
        for (snake::Cell cell : row) {
            if (cell == snake::Cell::kSnakeHead) {
                ++heads;
            } else if (cell == snake::Cell::kSnakeBody) {
                ++bodies;
            } else if (cell == snake::Cell::kChest) {
                ++chests;
            } else if (cell == snake::Cell::kFood) {
                ++food;
            }
        }
    }
    EXPECT_EQ(heads, 1);
    EXPECT_EQ(bodies, 0);
    EXPECT_EQ(chests, snake::kChestCount);
    EXPECT_EQ(food, 1);
}

TEST(GameTest, SnakeMovesOneCell) {
    Game game = centered_right_game(far_chests());
    game.step();

    ASSERT_EQ(game.snake().size(), 1U);
    EXPECT_EQ(game.snake().front(), (Point{6, 5}));
    EXPECT_EQ(game.turns(), 1);
    EXPECT_EQ(game.status(), Status::kRunning);
}

TEST(GameTest, ReverseDirectionIsIgnored) {
    Game game = centered_right_game(far_chests());
    EXPECT_FALSE(game.set_direction(Direction::kLeft));
    game.step();

    EXPECT_EQ(game.snake().front(), (Point{6, 5}));
    EXPECT_EQ(game.direction(), Direction::kRight);
}

TEST(GameTest, FoodIncreasesScoreAndLength) {
    Game game = centered_right_game(far_chests(), Point{6, 5});
    game.step();

    EXPECT_EQ(game.snake().front(), (Point{6, 5}));
    EXPECT_EQ(game.snake().size(), 2U);
    EXPECT_EQ(game.score(), 1);
    EXPECT_EQ(game.turns(), 1);
}

TEST(GameTest, EmptyChestCostsOnePoint) {
    Game game = centered_right_game(chests_with_target(Point{6, 5}, false));
    game.step();

    EXPECT_EQ(game.snake().front(), (Point{6, 5}));
    EXPECT_EQ(game.score(), -1);
    EXPECT_EQ(game.opened_chests(), 1);
    EXPECT_EQ(game.remaining_chests(), snake::kChestCount - 1);
    EXPECT_TRUE(game.chests()[0].opened);
    EXPECT_EQ(game.status(), Status::kRunning);
}

TEST(GameTest, GoldChestGivesTenPointsAndVictory) {
    Game game = centered_right_game(chests_with_target(Point{6, 5}, true));
    game.step();

    EXPECT_EQ(game.score(), 10);
    EXPECT_EQ(game.status(), Status::kWon);
    EXPECT_EQ(game.gold_chest_number(), 1);
    EXPECT_EQ(game.opened_chests(), 1);
    EXPECT_TRUE(game.last_event().find("золото") != std::string::npos);
}

TEST(GameTest, OpenedChestDoesNotPenalizeAgain) {
    Game game = centered_right_game(chests_with_target(Point{6, 5}, false));
    game.step();
    EXPECT_EQ(game.score(), -1);

    game.set_direction(Direction::kDown);
    game.step();
    game.set_direction(Direction::kLeft);
    game.step();
    game.set_direction(Direction::kUp);
    game.step();
    game.set_direction(Direction::kRight);
    game.step();

    EXPECT_EQ(game.snake().front(), (Point{6, 5}));
    EXPECT_EQ(game.score(), -1);
    EXPECT_EQ(game.turns(), 5);
    EXPECT_EQ(game.opened_chests(), 1);
}

TEST(GameTest, WallCollisionEndsGame) {
    Game game = make_game({{5, 5}}, Direction::kLeft, Point{9, 5}, far_chests());
    for (int i = 0; i < 5; ++i) {
        game.step();
    }
    EXPECT_EQ(game.snake().front(), (Point{0, 5}));
    EXPECT_EQ(game.status(), Status::kRunning);

    game.step();
    EXPECT_EQ(game.status(), Status::kLost);
    EXPECT_EQ(game.loss_reason(), LossReason::kWall);
    EXPECT_EQ(game.turns(), 6);
}

TEST(GameTest, SelfCollisionEndsGame) {
    Game game =
        make_game({{5, 5}, {4, 5}, {4, 4}, {5, 4}}, Direction::kLeft, Point{9, 5}, far_chests());
    game.step();

    EXPECT_EQ(game.status(), Status::kLost);
    EXPECT_EQ(game.loss_reason(), LossReason::kSelf);
    EXPECT_EQ(game.turns(), 1);
}

TEST(GameTest, VictoryStopsFurtherMoves) {
    Game game = centered_right_game(chests_with_target(Point{6, 5}, true));
    game.step();
    const Point head = game.snake().front();
    const int turns = game.turns();

    game.step();
    EXPECT_EQ(game.snake().front(), head);
    EXPECT_EQ(game.turns(), turns);
    EXPECT_EQ(game.status(), Status::kWon);
}

TEST(GameTest, ResetRestoresInitialState) {
    Game game(25U);
    game.step();
    game.reset(25U);
    const Game fresh(25U);

    EXPECT_EQ(game.status(), Status::kRunning);
    EXPECT_EQ(game.score(), 0);
    EXPECT_EQ(game.turns(), 0);
    ASSERT_EQ(game.snake().size(), 1U);
    EXPECT_EQ(game.snake().front(), (Point{5, 5}));
    EXPECT_EQ(game.food(), fresh.food());
    for (std::size_t i = 0; i < game.chests().size(); ++i) {
        EXPECT_EQ(game.chests()[i].position, fresh.chests()[i].position);
        EXPECT_EQ(game.chests()[i].has_gold, fresh.chests()[i].has_gold);
    }
}

TEST(GameTest, BoardAndChestCountersAreConsistent) {
    Game game = centered_right_game(chests_with_target(Point{6, 5}, false));
    game.step();

    const auto board = game.board();
    int chests = 0;
    int heads = 0;
    for (const auto& row : board) {
        for (snake::Cell cell : row) {
            if (cell == snake::Cell::kChest) {
                ++chests;
            } else if (cell == snake::Cell::kSnakeHead) {
                ++heads;
            }
        }
    }
    EXPECT_EQ(heads, 1);
    EXPECT_EQ(chests, game.remaining_chests());
    EXPECT_EQ(game.opened_chests(), 1);
}

}  // namespace
