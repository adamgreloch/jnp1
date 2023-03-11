#ifndef BOARD_H
#define BOARD_H

#include "player.h"
#include "square.h"
#include <memory>
#include <utility>
#include <vector>

// Reprezentuje cykliczną planszę do gry WorldCup.
// Jest odpowiedzialna za ustalanie ciągu pól po których mają przemieścić
// się gracze w zależności od pozycji i liczby kroków.
// Klasy dziedziczące mają mieć konstruktor bezparametrowy tworzący pola i
// zestawiający je w planszę.
class Board {
  public:
    virtual ~Board() = default;

    // Przechodzi po polach planszy wychodząc z pozycji start.
    // Zwraca wektor, którego pierwszy element to pozycja startowa,
    // ostatni element to pole, na którym gracz zostaje, a wszystkie pomiędzy
    // to pola przechodzone.
    virtual std::vector<std::shared_ptr<Square>> walk(uint64_t start,
                                                      uint64_t steps) const = 0;

    // Zwraca nazwę pola na danej pozycji.
    virtual std::string getSquareName(uint64_t position) const = 0;

    // Zwraca liczbę pól składających się na planszę.
    virtual uint64_t size() const = 0;
};

class Board2022 : public Board {
  private:
    static constexpr int64_t SZCZESNY_RANSOM = 180;
    static constexpr uint64_t GOAL_BONUS = 120;
    static constexpr uint64_t BOOKMAKER_COST = 100;
    static constexpr uint64_t BOOKMAKER_MOD = 3;
    static constexpr uint64_t YELLOW_CARD_DURATION = 3;
    static constexpr uint64_t SEASON_BEGINNING = 50;

    static constexpr double FRIENDLY_WEIGHT = 1.0;
    static constexpr double POINTS_WEIGHT = 2.5;
    static constexpr double FINAL_WEIGHT = 4.0;

    std::vector<std::shared_ptr<Square>> squares;

  public:
    Board2022()
        : squares({std::make_shared<SeasonBeginningSquare>("Początek sezonu",
                                                           SEASON_BEGINNING),
                   std::make_shared<GameSquare>("Mecz z San Marino",
                                                FRIENDLY_WEIGHT, 160),
                   std::make_shared<FreeTimeSquare>("Dzień wolny od treningu"),
                   std::make_shared<GameSquare>("Mecz z Lichtensteinem",
                                                FRIENDLY_WEIGHT, 220),
                   std::make_shared<YellowCardSquare>("Żółta kartka",
                                                      YELLOW_CARD_DURATION),
                   std::make_shared<GameSquare>("Mecz z Meksykiem",
                                                POINTS_WEIGHT, 300),
                   std::make_shared<GameSquare>("Mecz z Arabią Saudyjską",
                                                POINTS_WEIGHT, 280),
                   std::make_shared<BookmakerSquare>(
                       "Bukmacher", BOOKMAKER_COST, BOOKMAKER_MOD),
                   std::make_shared<GameSquare>("Mecz z Argentyną",
                                                POINTS_WEIGHT, 250),
                   std::make_shared<GoalSquare>("Gol", GOAL_BONUS),
                   std::make_shared<GameSquare>("Mecz z Francją", FINAL_WEIGHT,
                                                400),
                   std::make_shared<PenaltySquare>("Rzut karny",
                                                   SZCZESNY_RANSOM)}) {}

    ~Board2022() override = default;

    std::vector<std::shared_ptr<Square>> walk(uint64_t start,
                                              uint64_t steps) const override {
        std::vector<std::shared_ptr<Square>> res;
        if (steps == 0)
            res.emplace_back(squares[start]);
        else {
            unsigned long board_size = squares.size();

            for (uint64_t i = 0; i < steps; i++)
                res.emplace_back(squares[(start + 1 + i) % board_size]);
        }
        return res;
    };

    std::string getSquareName(uint64_t position) const override {
        return squares[position]->getName();
    }

    uint64_t size() const override { return squares.size(); }
};

#endif // BOARD_H
