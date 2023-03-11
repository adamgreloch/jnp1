#ifndef _KVFIFO_H_
#define _KVFIFO_H_

#include <list>
#include <map>
#include <memory>
#include <stdexcept>

template <typename K, typename V> class kvfifo {
  private:
    using K_V_queue_t = std::list<std::pair<K, V>>;
    using K_V_queue_it_t = typename K_V_queue_t::iterator;
    using K_it_lists_t = std::map<K, std::list<K_V_queue_it_t>>;

    struct kv_struct {
        K_V_queue_t K_V_queue;
        K_it_lists_t K_it_lists;
    };

    // "Oczekiwana złożoność czasowa operacji kopiowania przy zapisie
    // wynosi O(n log n), gdzie n oznacza liczbę elementów przechowywanych w
    // kolejce."
    // Skoro złożoność kopiowania nie zależy od liczby modyfikowanych
    // elementów, wystarczy trzymać sprytny wskaźnik na całą
    // strukturę, bez kombinowania ze wskaźnikami na poszczególne elementy.
    using kv_struct_shared = std::shared_ptr<kv_struct>;

    kv_struct_shared p;

    // Wartość true, jeśli przy użyciu metod front/back/first/last
    // udostępniono na zewnątrz referencję do pewnej wartości V, pozwalającą na
    // jej modyfikację. Jeśli kolejka ulega modyfikacji, dokonywane jest
    // deep-copy, więc dotychczasowe referencje ulegają przedawnieniu i
    // poniższa flaga ustawiana jest na false.
    bool v_refs_active = false;

    // Zwraca p sprzed próby deep-copy, jeśli głęboka kopia się udała, bądź
    // nie była potrzebna.
    kv_struct_shared &p_copy_if_shared() {
        if (!p.unique() || v_refs_active)
            return p_deep_copy(p);
        return p;
    }

    kv_struct_shared &p_deep_copy(kv_struct_shared const &from) {
        kv_struct_shared &prev = p;

        p = std::make_shared<kv_struct>(*from);

        // Odbudowa K_it_lists, by zawarte w niej iteratory
        // prowadziły do elementów kopii w *this, a nie w źródle from.
        p->K_it_lists.clear();
        auto it = p->K_V_queue.begin();
        try {
            while (it != p->K_V_queue.end()) {
                p->K_it_lists[it->first].push_back(it);
                it++;
            }
        } catch (...) {
            p = prev;
            throw;
        }

        return prev;
    }

  public:
    // Konstruktory: bezparametrowy tworzący pustą kolejkę, kopiujący i
    // przenoszący. Złożoność O(1).
    constexpr kvfifo() { p = std::make_shared<kv_struct>(kv_struct()); }

    constexpr kvfifo(kvfifo const &other) {
        if (other.v_refs_active) {
            // Jeśli doszło do udostępnienia referencji pozwalającej na edycję
            // wartości V w other, zrób głęboką kopię other.p.
            p_deep_copy(other.p);
            v_refs_active = false;
        } else
            p = other.p;
    }

    constexpr kvfifo(kvfifo &&other) noexcept {
        p = other.p;
        other.p.reset();
    }

    // Operator przypisania przyjmujący argument przez wartość. Złożoność O(1)
    // plus czas niszczenia nadpisywanego obiektu.
    constexpr kvfifo &operator=(kvfifo other) noexcept {
        p = other.p;
        return *this;
    }

    // Metoda push wstawia wartość v na koniec kolejki, nadając jej klucz k.
    // Złożoność O(log n).
    void push(K const &k, V const &v) {
        kv_struct_shared &prev = p_copy_if_shared();

        try {
            std::list<K_V_queue_it_t> &it_list = p->K_it_lists[k];
            try {
                p->K_V_queue.emplace_back(k, v);
            } catch (...) {
                p->K_it_lists.erase(k);
                throw;
            }
            try {
                it_list.push_back(std::prev(p->K_V_queue.end()));
            } catch (...) {
                p->K_V_queue.pop_back();
                p->K_it_lists.erase(k);
                throw;
            }
        } catch (...) {
            p = prev;
            throw;
        }
        v_refs_active = false;
    }

    // Metoda pop() usuwa pierwszy element z kolejki. Jeśli kolejka jest pusta,
    // to podnosi wyjątek std::invalid_argument. Złożoność O(log n).
    void pop() {
        kv_struct_shared &prev = p_copy_if_shared();

        try {
            if (p->K_V_queue.empty()) {
                throw std::invalid_argument("pop() on empty kvfifo");
            }

            K_V_queue_it_t it = p->K_V_queue.begin();
            K k = it->first;

            typename K_it_lists_t::iterator it_list = p->K_it_lists.find(k);

            p->K_V_queue.pop_front();
            it_list->second.pop_front();
            if (it_list->second.empty()) {
                p->K_it_lists.erase(it_list);
            }
        } catch (...) {
            p = prev;
            throw;
        }
        v_refs_active = false;
    }

    // Metoda pop(k) usuwa pierwszy element o podanym kluczu z kolejki. Jeśli
    // podanego klucza nie ma w kolejce, to podnosi wyjątek
    // std::invalid_argument. Złożoność O(log n).
    void pop(K const &k) {
        kv_struct_shared &prev = p_copy_if_shared();

        try {
            typename K_it_lists_t::iterator it_list = p->K_it_lists.find(k);
            if (it_list == p->K_it_lists.end())
                throw std::invalid_argument("pop(k) but no key k in kvfifo");

            K_V_queue_it_t it = it_list->second.front();

            p->K_V_queue.erase(it);
            it_list->second.pop_front();
            if (it_list->second.empty()) {
                p->K_it_lists.erase(it_list);
            }
        } catch (...) {
            p = prev;
            throw;
        }
        v_refs_active = false;
    }

    // Metoda move_to_back przesuwa elementy o kluczu k na koniec kolejki,
    // zachowując ich kolejność względem siebie. Zgłasza wyjątek
    // std::invalid_argument, gdy elementu o podanym kluczu nie ma w kolejce.
    // Złożoność O(m + log n), gdzie m to liczba przesuwanych elementów.
    void move_to_back(K const &k) {
        kv_struct_shared &prev = p_copy_if_shared();

        try {
            typename K_it_lists_t::iterator it_list = p->K_it_lists.find(k);

            if (it_list == p->K_it_lists.end()) {
                p = prev;
                throw std::invalid_argument(
                    "move_to_back(k) but no key k in kvfifo");
            }

            std::list<K_V_queue_it_t> K_list = it_list->second;

            for (auto it = K_list.begin(); it != K_list.end(); ++it)
                p->K_V_queue.splice(p->K_V_queue.end(), p->K_V_queue, *it);
        } catch (...) {
            p = prev;
            throw;
        }
        v_refs_active = false;
    }

    // Metody front i back zwracają parę referencji do klucza i wartości
    // znajdującej się odpowiednio na początku i końcu kolejki. W wersji
    // nie-const zwrócona para powinna umożliwiać modyfikowanie wartości, ale
    // nie klucza. Dowolna operacja modyfikująca kolejkę może unieważnić
    // zwrócone referencje. Jeśli kolejka jest pusta, to podnosi wyjątek
    // std::invalid_argument. Złożoność O(1).
    std::pair<K const &, V const &> front() const {
        if (p->K_V_queue.empty())
            throw std::invalid_argument("front() on empty kvfifo");

        std::pair<K, V> &pair = p->K_V_queue.front();
        return {pair.first, pair.second};
    }

    std::pair<K const &, V &> front() {
        kv_struct_shared &prev = p_copy_if_shared();
        bool prev_state = v_refs_active;

        try {
            if (p->K_V_queue.empty())
                throw std::invalid_argument("front() on empty kvfifo");

            std::pair<K, V> &pair = p->K_V_queue.front();

            v_refs_active = true;
            return {pair.first, pair.second};
        } catch (...) {
            p = prev;
            v_refs_active = prev_state;
            throw;
        }
    }

    std::pair<K const &, V &> back() {
        kv_struct_shared &prev = p_copy_if_shared();
        bool prev_state = v_refs_active;

        try {
            if (p->K_V_queue.empty())
                throw std::invalid_argument("back() on empty kvfifo");

            std::pair<K, V> &pair = p->K_V_queue.back();

            v_refs_active = true;
            return {pair.first, pair.second};
        } catch (...) {
            p = prev;
            v_refs_active = prev_state;
            throw;
        }
    }

    std::pair<K const &, V const &> back() const {
        if (p->K_V_queue.empty())
            throw std::invalid_argument("back() on empty kvfifo");

        std::pair<K, V> &pair = p->K_V_queue.back();
        return {pair.first, pair.second};
    }

    // Metody first i last zwracają odpowiednio pierwszą i ostatnią parę
    // klucz-wartość o danym kluczu, podobnie jak front i back. Jeśli podanego
    // klucza nie ma w kolejce, to podnosi wyjątek std::invalid_argument.
    // Złożoność O(log n).
    std::pair<K const &, V &> first(K const &key) {
        kv_struct_shared &prev = p_copy_if_shared();
        bool prev_state = v_refs_active;

        try {
            if (p->K_it_lists.find(key) == p->K_it_lists.end())
                throw std::invalid_argument("no such key");

            std::pair<K, V> &pair = *p->K_it_lists[key].front();

            v_refs_active = true;
            return {pair.first, pair.second};
        } catch (...) {
            p = prev;
            v_refs_active = prev_state;
            throw;
        }
    }

    std::pair<K const &, V const &> first(K const &key) const {
        if (p->K_it_lists.find(key) == p->K_it_lists.end())
            throw std::invalid_argument("no such key");

        std::pair<K, V> &pair = *p->K_it_lists[key].front();
        return {pair.first, pair.second};
    }

    std::pair<K const &, V &> last(K const &key) {
        kv_struct_shared &prev = p_copy_if_shared();
        bool prev_state = v_refs_active;

        try {
            if (p->K_it_lists.find(key) == p->K_it_lists.end())
                throw std::invalid_argument("no such key");

            std::pair<K, V> &pair = *p->K_it_lists[key].back();

            v_refs_active = true;
            return {pair.first, pair.second};
        } catch (...) {
            p = prev;
            v_refs_active = prev_state;
            throw;
        }
    }

    std::pair<K const &, V const &> last(K const &key) const {
        if (p->K_it_lists.find(key) == p->K_it_lists.end())
            throw std::invalid_argument("no such key");

        std::pair<K, V> &pair = *p->K_it_lists[key].back();
        return {pair.first, pair.second};
    }

    // Metoda size zwraca liczbę elementów w kolejce. Złożoność O(1).
    std::size_t size() const noexcept { return p->K_V_queue.size(); }

    // Metoda empty zwraca true, gdy kolejka jest pusta, a false w przeciwnym
    // przypadku. Złożoność O(1).
    bool empty() const noexcept { return p->K_V_queue.empty(); }

    // Metoda count zwraca liczbę elementów w kolejce o podanym kluczu.
    // Złożoność O(log n).
    std::size_t count(K const &k) const noexcept {
        typename K_it_lists_t::iterator it_list = p->K_it_lists.find(k);

        if (it_list == p->K_it_lists.end())
            return 0;
        else
            return it_list->second.size();
    }

    // Metoda clear usuwa wszystkie elementy z kolejki. Złożoność O(n).
    void clear() {
        p_copy_if_shared();
        p->K_V_queue.clear();
        p->K_it_lists.clear();
    }

    // Iterator k_iterator oraz metody k_begin i k_end, pozwalające przeglądać
    // zbiór kluczy w rosnącej kolejności ich wartości. Iteratory mogą być
    // unieważnione przez dowolną zakończoną powodzeniem operację modyfikującą
    // kolejkę oraz operacje front, back, first i last w wersjach bez const.
    // Iterator musi spełniać koncept std::bidirectional_iterator. Wszelkie
    // operacje w czasie O(log n). Przeglądanie całej kolejki w czasie O(n).
    // Iterator służy jedynie do przeglądania kolejki i za jego pomocą nie można
    // modyfikować kolejki, więc zachowuje się jak const_iterator z biblioteki
    // standardowej.
    class k_iterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = const K;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        k_iterator() = default;

        k_iterator(typename K_it_lists_t::const_iterator _wrapped)
            : wrapped(_wrapped) {}

        reference operator*() const noexcept { return (*wrapped).first; }

        pointer operator->() const noexcept { return *(*wrapped).first; }

        k_iterator &operator++() noexcept { // ++it
            wrapped++;
            return *this;
        }

        k_iterator operator++(int) noexcept { // it++
            k_iterator result(*this);
            operator++();
            return result;
        }

        k_iterator &operator--() noexcept { // --it
            wrapped--;
            return *this;
        }

        k_iterator operator--(int) noexcept { // it--
            k_iterator result(*this);
            operator--();
            return result;
        }

        friend bool operator==(k_iterator const &a,
                               k_iterator const &b) noexcept {
            return a.wrapped == b.wrapped;
        }

        friend bool operator!=(k_iterator const &a,
                               k_iterator const &b) noexcept {
            return !(a == b);
        }

      private:
        typename K_it_lists_t::const_iterator wrapped;
    };

    k_iterator k_begin() const noexcept {
        return k_iterator(p->K_it_lists.cbegin());
    }

    k_iterator k_end() const noexcept {
        return k_iterator(p->K_it_lists.cend());
    }
};

#endif // _KVFIFO_H_
