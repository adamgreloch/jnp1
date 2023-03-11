#ifndef PLAYER_H
#define PLAYER_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "board.h"
#include "square.h"

// Reprezentuje gracza gry WorldCup.
// Jedyną odpowiedzialnością Player jest realizowanie ciągów akcji spowodowanych
// ruchem po planszy, mających wpływ na jego pozycję na planszy, czas
// oczekiwania oraz liczbę posiadanych zdzisławów.
class Player {
  private:
    static constexpr char bankrupt_str[] = "bankrut";
    static constexpr char wait_str[] = "czekanie: ";
    static constexpr char in_game_str[] = "w grze";

    const std::string name;

    int64_t money;
    uint64_t waiting = 0;
    uint64_t position;

  protected:
    // Przechodzi przez dane pole, wykonując związaną z nim akcję.
    virtual void passThrough(Square &sq) {
        if (money + sq.afterPassing() >= 0) {
            money += sq.afterPassing();
            sq.passThrough();
        } else
            money = -1;
    }

    // Zatrzymuje się w danym polu, wykonując związaną z nim akcję.
    virtual void stopIn(Square &sq) {
        if (money + sq.afterStopping() >= 0) {
            money += sq.afterStopping();
            waiting += sq.turnsToWait();
            sq.stopOn();
        } else
            money = -1;
    }

    virtual std::string stars(const std::string &s) const {
        return "*** " + s + " ***";
    }

  public:
    Player(std::string _name, int64_t _money, uint64_t _position)
        : name(std::move(_name)), money(_money), position(_position) {}

    virtual ~Player() = default;

    // Przechodzi się po polach z zadanej ścieżki, aktywując powiązane z nimi
    // akcje. Przez pierwsze path.size()-1 pól wykonuje akcję
    // przechodzenia, zaś na ostatnim polu wykonuje akcję zatrzymania się
    // na polu. Zwraca false, jeśli doszło do bankructwa w którymkolwiek
    // momencie przechodzenia przez ścieżkę.
    virtual bool makeMove(const std::vector<std::shared_ptr<Square>> &path,
                          uint64_t board_size) {
        if (money < 0)
            return false;

        uint64_t size = path.size();

        for (uint64_t i = 0; i < size - 1 && money >= 0; i++)
            passThrough(*path[i]);

        stopIn(*path[size - 1]);

        position += size;
        position %= board_size;

        if (money < 0) {
            money = 0;
            return false;
        }

        return money >= 0;
    }

    // Jeśli gracz musi poczekać, to czeka turę. Zwraca true, jeśli gracz
    // odczekał wszystko i może grać dalej, false w przeciwnym przypadku.
    virtual bool waitTurnIfNeeded() {
        if (waiting > 0)
            waiting--;
        return waiting == 0;
    }

    virtual int64_t getMoney() const { return money; }

    virtual const std::string &getName() const { return name; }

    virtual uint64_t getPosition() const { return position; }

    // Zwraca obecny status gracza w postaci napisu.
    virtual std::string status() const {
        if (money == 0)
            return stars(bankrupt_str);
        else if (waiting > 0)
            return stars(wait_str + std::to_string(waiting));
        else
            return in_game_str;
    }
};

#endif // PLAYER_H
