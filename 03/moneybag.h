#ifndef MONEYBAG_H
#define MONEYBAG_H

#include <compare>
#include <sstream>
#include <stdexcept>

class Moneybag {
public:
    using coin_number_t = uint64_t;

private:
    coin_number_t livre;
    coin_number_t solidus;
    coin_number_t denier;

public:
    constexpr Moneybag(coin_number_t livre, coin_number_t solidus,
                       coin_number_t denier)
            : livre(livre), solidus(solidus), denier(denier) {}

    [[nodiscard]] constexpr coin_number_t livre_number() const { return livre; }

    [[nodiscard]] constexpr coin_number_t solidus_number() const {
        return solidus;
    }

    [[nodiscard]] constexpr coin_number_t denier_number() const {
        return denier;
    }

    constexpr Moneybag &operator+=(const Moneybag &moneybag) {
        if (livre + moneybag.livre < livre ||
            solidus + moneybag.solidus < solidus ||
            denier + moneybag.denier < denier)
            throw std::out_of_range("Wyjście poza zakres arytmetyki");
        livre += moneybag.livre;
        solidus += moneybag.solidus;
        denier += moneybag.denier;
        return *this;
    }

    constexpr Moneybag &operator-=(const Moneybag &moneybag) {
        if (livre - moneybag.livre > livre ||
            solidus - moneybag.solidus > solidus ||
            denier - moneybag.denier > denier)
            throw std::out_of_range("Wyjście poza zakres arytmetyki");
        livre -= moneybag.livre;
        solidus -= moneybag.solidus;
        denier -= moneybag.denier;
        return *this;
    }

    constexpr Moneybag &operator*=(const size_t &n) {
        if (n != 0 &&
            ((livre * n) / n != livre || (solidus * n) / n != solidus ||
             (denier * n) / n != denier))
            throw std::out_of_range("Wyjście poza zakres arytmetyki");
        livre *= n;
        solidus *= n;
        denier *= n;
        return *this;
    }

    constexpr Moneybag operator+(const Moneybag &moneybag) const {
        return Moneybag(*this) += moneybag;
    }

    constexpr Moneybag operator-(const Moneybag &moneybag) const {
        return Moneybag(*this) -= moneybag;
    }

    constexpr Moneybag operator*(const size_t &n) const { return Moneybag(*this) *= n; }

    constexpr explicit operator bool() const {
        return livre > 0 || solidus > 0 || denier > 0;
    }

    constexpr bool operator==(const Moneybag &moneybag) const {
        return livre == moneybag.livre && solidus == moneybag.solidus &&
               denier == moneybag.denier;
    }

    constexpr std::partial_ordering operator<=>(
            const Moneybag &moneybag) const {
        if (livre == moneybag.livre && solidus == moneybag.solidus &&
            denier == moneybag.denier)
            return std::partial_ordering::equivalent;
        if (livre <= moneybag.livre && solidus <= moneybag.solidus &&
            denier <= moneybag.denier)
            return std::partial_ordering::less;
        if (livre >= moneybag.livre && solidus >= moneybag.solidus &&
            denier >= moneybag.denier)
            return std::partial_ordering::greater;
        return std::partial_ordering::unordered;
    }
};

static std::ostream &operator<<(std::ostream &stream,
                                const Moneybag &moneybag) {
    stream << "(" << moneybag.livre_number() << " ";
    if (moneybag.livre_number() == 1)
        stream << "livr";
    else
        stream << "livres";
    stream << ", " << moneybag.solidus_number() << " ";
    if (moneybag.solidus_number() == 1)
        stream << "solidus";
    else
        stream << "soliduses";
    stream << ", " << moneybag.denier_number() << " ";
    if (moneybag.denier_number() == 1)
        stream << "denier";
    else
        stream << "deniers";
    stream << ")";
    return stream;
}

static constexpr Moneybag operator*(const size_t n, const Moneybag &moneybag) {
    return moneybag * n;
}

static constexpr Moneybag Livre(1, 0, 0);
static constexpr Moneybag Solidus(0, 1, 0);
static constexpr Moneybag Denier(0, 0, 1);

class Value {
private:
    static const uint8_t DENIERS_PER_SOLIDUS = 12;
    static const uint8_t DENIERS_PER_LIVRE = 240;
    using value_t = __uint128_t;
    value_t val;

public:
    consteval Value() : val(0) {};

    constexpr explicit Value(const Moneybag &b)
            : val(value_t(b.livre_number()) * DENIERS_PER_LIVRE +
                  value_t(b.solidus_number()) * DENIERS_PER_SOLIDUS +
                  value_t(b.denier_number())) {};

    constexpr explicit Value(const size_t &deniers) : val(deniers) {};

    Value &operator=(const size_t &deniers) {
        val = deniers;
        return *this;
    }

    constexpr Value(const Value &v) = default;

    constexpr bool operator==(const Value &v) const {
        return this->val == v.val;
    }

    constexpr std::strong_ordering operator<=>(const Value &v) const = default;

    constexpr bool operator==(const size_t &deniers) const {
        return this->val == deniers;
    }

    constexpr std::strong_ordering operator<=>(const size_t &deniers) const {
        return this->val <=> deniers;
    }

    explicit operator std::string() const {
        if (val == 0) return "0";

        std::string res;
        value_t x = val;
        uint8_t d;

        while (x > 0) {
            d = x % 10;
            res += std::to_string(d);
            x = (x - d) / 10;
        }

        std::reverse(res.begin(), res.end());
        return res;
    }
};

#endif
