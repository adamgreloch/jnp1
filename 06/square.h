#ifndef SQUARE_H
#define SQUARE_H

#include <iostream>
#include <string>

// Reprezentuje pole w grze.
// obiekty klasy square są odpowiedzialne za przechowywanie
// i przekazywanie informacji o konsekwencjach przejścia lub zatrzymania się
// na danym polu.
class Square {
  private:
    std::string name;

  public:
    explicit Square(std::string name) : name(std::move(name)) {}
    virtual ~Square() = default;

    // Zwraca liczbę tur, które trzeba poczekać, po zatrzymaniu się na tym
    // polu.
    virtual uint64_t turnsToWait() const = 0;

    // Zwraca liczbę zdzisławów zdobytych/straconych po przejściu przez to pole.
    virtual int64_t afterPassing() const = 0;

    // Zwraca liczbę zdzisławów zdobytych/straconych po zatrzymaniu się na
    // tym polu.
    virtual int64_t afterStopping() = 0;

    // Sygnalizuje przejście gracza przez to pole. Zakłada, że może
    // legalnie to zrobić, np. pokryć całkowity koszt przejścia.
    virtual void passThrough(){};

    // Sygnalizuje zatrzymanie się gracza na tym polu. Zakłada, że może
    // legalnie to zrobić, np. pokryć całkowity koszt zatrzymania.
    virtual void stopOn(){};

    // Zwraca nazwę pola.
    virtual const std::string &getName() const final { return name; }
};

// poniższe klasy reprezentują konkretne rodzaje pól dostępnyh w grze

// początek sezonu – przy przejściu lub zatrzymaniu się na tym polu gracz
// dostaje 50 zdzisławów;
class SeasonBeginningSquare : public Square {
  private:
    const uint64_t SeasonBeginningReward;

  public:
    explicit SeasonBeginningSquare(const std::string &name, uint64_t reward_)
        : Square(name), SeasonBeginningReward(reward_) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return SeasonBeginningReward; }

    int64_t afterStopping() override { return SeasonBeginningReward; }
};

// gol – przy zatrzymaniu na tym polu gracz dostaje premię za gola;
class GoalSquare : public Square {
  private:
    const int64_t goal_bonus;

  public:
    explicit GoalSquare(const std::string &name, int64_t _goal_bonus)
        : Square(name), goal_bonus(_goal_bonus) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return 0; }

    int64_t afterStopping() override { return goal_bonus; }
};

// rzut karny – przy zatrzymaniu się na tym polu gracz musi zapłacić Szczęsnemu
// za obronę rzutu karnego;
class PenaltySquare : public Square {
  private:
    const int64_t save_cost;

  public:
    explicit PenaltySquare(const std::string &name, int64_t _save_cost)
        : Square(name), save_cost(_save_cost) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return 0; }

    int64_t afterStopping() override { return -save_cost; }
};

// bukmacher – przy zatrzymaniu się na tym polu co trzeci gracz wygrywa zakład
// w zdzisławach, a pozostali przegrywają; seria zaczyna się od wygranego
// zakładu u bukmachera;
class BookmakerSquare : public Square {
  private:
    uint64_t stop_counter = 0;
    const int64_t cost;
    const uint64_t mod;

  public:
    explicit BookmakerSquare(const std::string &name, int64_t _cost,
                             uint64_t _mod)
        : Square(name), cost(_cost), mod(_mod) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return 0; }

    int64_t afterStopping() override {
        bool win = stop_counter == 0;
        return win ? cost : -cost;
    }

    void stopOn() override { stop_counter = (stop_counter + 1) % mod; }
};

// żółta kartka – gracz czeka określoną liczbę kolejek;
class YellowCardSquare : public Square {
  private:
    const uint64_t duration;

  public:
    explicit YellowCardSquare(const std::string &name, uint64_t _duration)
        : Square(name), duration(_duration) {}

    uint64_t turnsToWait() const override { return duration; }

    int64_t afterPassing() const override { return 0; }

    int64_t afterStopping() override { return 0; }
};

// mecz – przy przejściu przez to pole gracz musi rozegrać mecz i uiścić opłatę
// za rozegranie; gracz, który zatrzyma się na tym polu, zgarnia wszystkie
// pobrane opłaty za rozegrane mecze pomnożone przez wagę meczu wg FIFA.
class GameSquare : public Square {
  private:
    double weight;
    int64_t cost;
    int64_t accumulated = 0;

  public:
    GameSquare(const std::string &name, double _weight, unsigned int _cost)
        : Square(name), weight(_weight), cost(_cost) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return -cost; }

    void passThrough() override { accumulated += cost; }

    int64_t afterStopping() override {
        return static_cast<int64_t>(weight * accumulated);
    }

    void stopOn() override { accumulated = 0; }
};

// dzień wolny
class FreeTimeSquare : public Square {
  public:
    explicit FreeTimeSquare(const std::string &name) : Square(name) {}

    uint64_t turnsToWait() const override { return 0; }

    int64_t afterPassing() const override { return 0; }

    int64_t afterStopping() override { return 0; }
};

#endif // SQUARE_H
