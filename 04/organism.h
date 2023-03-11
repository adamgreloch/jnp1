#ifndef _ORGANISM_H_
#define _ORGANISM_H_

#include <iostream>
#include <optional>
#include <cstdint>
#include <tuple>

template<typename species_t, bool can_eat_meat, bool can_eat_plants>
requires std::equality_comparable<species_t>
class Organism {
private:
    uint64_t vitality;
    species_t const &species;

public:
    constexpr Organism(species_t const &species, uint64_t const &vitality) :
            species(species), vitality(vitality) {};

    constexpr uint64_t get_vitality() const {
        return vitality;
    }

    constexpr void add_vitality(uint64_t const &v) {
        vitality += v;
    }

    constexpr void die() {
        vitality = 0;
    }

    constexpr bool is_dead() const {
        return vitality <= 0;
    }

    constexpr species_t &get_species() const {
        return species;
    }

    template<bool sp_eats_m, bool sp_eats_p>
    constexpr bool
    eats(Organism<species_t, sp_eats_m, sp_eats_p> const &other) {

        if ((is_carnivore() && !other.is_plant()) ||
            (is_omnivore() || (is_herbivore() && other.is_plant())))
            return true;
        else return false;
    }

    constexpr static bool is_omnivore() {
        return can_eat_meat && can_eat_plants;
    }

    constexpr static bool is_herbivore() {
        return !can_eat_meat && can_eat_plants;
    }

    constexpr static bool is_carnivore() {
        return can_eat_meat && !can_eat_plants;
    }

    constexpr static bool is_plant() {
        return !can_eat_meat && !can_eat_plants;
    }
};

template<typename species_t>
using Carnivore = class Organism<species_t const, true, false>;

template<typename species_t>
using Omnivore = class Organism<species_t const, true, true>;

template<typename species_t>
using Herbivore = class Organism<species_t const, false, true>;

template<typename species_t>
using Plant = class Organism<species_t const, false, false>;

template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
        bool sp2_eats_m, bool sp2_eats_p>
constexpr std::tuple<Organism<species_t, sp1_eats_m, sp1_eats_p>,
        Organism<species_t, sp2_eats_m, sp2_eats_p>,
        std::optional<Organism<species_t, sp1_eats_m, sp1_eats_p>>>
encounter(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
          Organism<species_t, sp2_eats_m, sp2_eats_p> organism2);

// Namespace for hiding implementation details.
namespace detail {
    /*
     * One of organisms is a plant and the other can eat it.
     */
    template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
            bool sp2_eats_m, bool sp2_eats_p>
    constexpr std::tuple<Organism<species_t, sp1_eats_m, sp1_eats_p>,
            Organism<species_t, sp2_eats_m, sp2_eats_p>,
            std::optional<Organism<species_t, sp1_eats_m, sp1_eats_p>>>
    eat_plant(Organism<species_t, sp1_eats_m, sp1_eats_p> o1,
              Organism<species_t, sp2_eats_m, sp2_eats_p> o2) {

        if (o1.eats(o2)) {
            o1.add_vitality(o2.get_vitality());
            o2.die();
        } else {
            o2.add_vitality(o1.get_vitality());
            o1.die();
        }
        return {o1, o2, std::nullopt};
    }

    /*
     * None of organisms is a plant and one possibly can eat the other.
     */
    template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
            bool sp2_eats_m, bool sp2_eats_p>
    constexpr std::tuple<Organism<species_t, sp1_eats_m, sp1_eats_p>,
            Organism<species_t, sp2_eats_m, sp2_eats_p>,
            std::optional<Organism<species_t, sp1_eats_m, sp1_eats_p>>>
    eat_animal(Organism<species_t, sp1_eats_m, sp1_eats_p> o1,
               Organism<species_t, sp2_eats_m, sp2_eats_p> o2) {

        if (o1.eats(o2)) {
            if (o1.get_vitality() > o2.get_vitality()) {
                o1.add_vitality(o2.get_vitality() / 2);
                o2.die();
            } else if (o2.eats(o1)) {
                if (o1.get_vitality() == o2.get_vitality()) {
                    o1.die();
                    o2.die();
                } else {
                    o2.add_vitality(o1.get_vitality() / 2);
                    o1.die();
                }
            }
        } else /* o2.eats(o1) && !o1.eats(o2) */ {
            if (o1.get_vitality() > o2.get_vitality()) {
                o2.add_vitality(o1.get_vitality() / 2);
                o1.die();
            }
        }
        return {o1, o2, std::nullopt};
    }

    template<typename species_t, bool sp1_eats_m, bool sp1_eats_p>
    constexpr Organism<species_t, sp1_eats_m, sp1_eats_p>
    encounter_series_helper(
            Organism<species_t, sp1_eats_m, sp1_eats_p> organism1) {

        return organism1;
    }

    template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
            bool sp2_eats_m, bool sp2_eats_p, typename ... Args>
    constexpr Organism<species_t, sp1_eats_m, sp1_eats_p>
    encounter_series_helper(
            Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
            Organism<species_t, sp2_eats_m, sp2_eats_p> organism2,
            Args ... args) {

        return encounter_series_helper(
                std::get<0>(encounter(organism1, organism2)),
                args...);
    }
}

/*
 * Returns modified copies of organism1 and organism2 and, if applicable,
 * their child in a way specified by encounter rules.
 */
template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
        bool sp2_eats_m, bool sp2_eats_p>
constexpr std::tuple<Organism<species_t, sp1_eats_m, sp1_eats_p>,
        Organism<species_t, sp2_eats_m, sp2_eats_p>,
        std::optional<Organism<species_t, sp1_eats_m, sp1_eats_p>>>
encounter(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
          Organism<species_t, sp2_eats_m, sp2_eats_p> organism2) {

    using namespace detail;

    // 2. two plants cannot meet
    static_assert(!organism1.is_plant() || !organism2.is_plant());

    if (organism1.is_dead() || organism2.is_dead()
        || (!organism1.eats(organism2) && !organism2.eats(organism1))) {
        // 3. dead organism does nothing
        // or 5. two organisms - neither can eat another - do nothing
    } else if (std::is_same_v<decltype(organism1), decltype(organism2)>) {
        // 4. couple of the same species produces a child
        return {organism1, organism2, Organism<species_t,
                sp1_eats_m, sp1_eats_p>(organism1.get_species
                (), (organism1.get_vitality() + organism2.get_vitality()) / 2)};
    } else if (organism1.is_plant() || organism2.is_plant()) {
        // 7. plant and plant-eating animal - animal eats plant
        return eat_plant(organism1, organism2);
    } else {
        // 6. two animals - both can eat another - fight
        // or 8. herbivore and meat-eating animal
        return eat_animal(organism1, organism2);
    }

    return {organism1, organism2, std::nullopt};
}


template<typename species_t, bool sp1_eats_m, bool sp1_eats_p,
        typename ... Args>
constexpr Organism<species_t, sp1_eats_m, sp1_eats_p>
encounter_series(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1, Args
... args) {

    return detail::encounter_series_helper(organism1, args...);
}

#endif // _ORGANISM_H_
