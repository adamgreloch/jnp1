#ifndef WORLDCUP2022_H
#define WORLDCUP2022_H

#include "board.h"
#include "player.h"
#include "square.h"
#include "worldcup.h"
#include <memory>
#include <vector>

// Wyjątek zwracany, gdy do gry zostało dodanych zbyt wiele kostek.
class TooManyDiceException : public std::exception {};

// Wyjątek zwracany, gdy do gry zostało dodanych za mało kostek.
class TooFewDiceException : public std::exception {};

// Wyjątek zwracany, gdy do gry zostało dodanych zbyt wiele graczy.
class TooManyPlayersException : public std::exception {};

// Wyjątek zwracany, gdy do gry zostało dodanych za mało graczy.
class TooFewPlayersException : public std::exception {};

// Klasa WorldCup2022 dziedzicząca po WorldCup.
// Jej zadaniem jest przeprowadzanie kolejnych tur gry. (High Cohesion)
// Klasa Worldcup2022 przechowuje wszystkie obiekty potrzebne
// do przeprowadzenia rozgrywki.
class WorldCup2022 : public WorldCup {
  private:
    static constexpr size_t N_MIN_DICE = 2;
    static constexpr size_t N_MAX_DICE = 2;
    static constexpr size_t N_MIN_PLAYERS = 2;
    static constexpr size_t N_MAX_PLAYERS = 11;

    std::vector<std::shared_ptr<Die>> dies;
    std::vector<std::shared_ptr<Player>> players;
    std::shared_ptr<ScoreBoard> scoreBoard;
    std::shared_ptr<Board> board;

    const unsigned int initial_money = 1000;
    const unsigned int initial_position = 0;

    // Rozgrywa pojedynczą rundę i przekazuje informacje o wynikach
    // do obiektu klasy Scoreboard. Zwraca ilość pozostałych graczy.
    uint64_t playRound(unsigned int round_number) {
        scoreBoard->onRound(round_number);
        uint64_t players_left = 0;
        uint64_t steps;

        for (auto &player : players)
            if (player->getMoney() > 0) {
                if (player->waitTurnIfNeeded()) {
                    steps = 0;
                    for (auto &die : dies)
                        steps += die->roll();
                    std::vector<std::shared_ptr<Square>> path =
                        board->walk(player->getPosition(), steps);
                    players_left += player->makeMove(path, board->size());
                } else
                    players_left++;
                scoreBoard->onTurn(player->getName(), player->status(),
                                   board->getSquareName(player->getPosition()),
                                   player->getMoney());
            }

        return players_left;
    }

  public:
    WorldCup2022() : board(std::make_shared<Board2022>()){};

    ~WorldCup2022() override = default;

    void addDie(std::shared_ptr<Die> die) override {
        if (die == nullptr)
            return;
        dies.push_back(die);
    }

    void addPlayer(const std::string &name) override {
        players.push_back(
            std::make_shared<Player>(name, initial_money, initial_position));
    }

    void setScoreBoard(std::shared_ptr<ScoreBoard> scoreboard) override {
        scoreBoard = scoreboard;
    }

    void play(unsigned int rounds) override {
        if (dies.size() > N_MAX_DICE)
            throw TooManyDiceException();
        if (dies.size() < N_MIN_DICE)
            throw TooFewDiceException();
        if (players.size() > N_MAX_PLAYERS)
            throw TooManyPlayersException();
        if (players.size() < N_MIN_PLAYERS)
            throw TooFewPlayersException();

        // rozegranie rund
        unsigned int round_number = 0;
        while (playRound(round_number++) > 1 && round_number < rounds) {
        }

        // wyłonienie zwycięzcy
        auto &winner = players.front();
        for (auto &player : players)
            if (player->getMoney() > winner->getMoney())
                winner = player;

        scoreBoard->onWin(winner->getName());
    }
};

#endif // WORLDCUP2022_H
