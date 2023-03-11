#include "hash.h"
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <vector>

#ifndef NDEBUG
bool const debug = true;
#else
bool const debug = false;
#endif

using std::unordered_set, std::unordered_map, std::vector, std::pair,
    std::size_t, std::cerr, std::endl, std::string, std::to_string;

namespace jnp1 {
  using id_t = unsigned long;
  using key_t = vector<uint64_t>;

  struct custom_hash {
    hash_function_t hash_function;

    size_t operator()(const key_t &k) const {
      return hash_function(k.data(), k.size());
    }

    explicit custom_hash(hash_function_t hf) : hash_function(hf) {}
  };

  using hashset_t = unordered_set<key_t, custom_hash>;
  using hashsets_t = unordered_map<id_t, hashset_t>;

  int const INITIAL_SIZE = 16;

  namespace {
    hashsets_t &get_hashsets() {
      static hashsets_t hashsets;
      return hashsets;
    }

    string seq_rep(uint64_t const *seq, size_t size) {
      if (!seq) return "NULL";

      string res = "\"";
      for (size_t i = 0; i < size; i++) {
        if (i > 0) res += " ";
        res += to_string(seq[i]);
      }
      res += "\"";
      return res;
    }

    string seq_rep(key_t &v) {
      return seq_rep(v.data(), v.size());
    }

    bool check_table_exists(const string &func_name, unsigned long id) {
      if (!get_hashsets().count(id)) {
        if (debug)
          cerr << func_name << ": hash table #" << id << " does not exist"
               << endl;
        return false;
      }
      return true;
    }

    void cerr_seq_state(const string &func_name, unsigned long id, key_t &v,
                        const string &state) {
      cerr << func_name << ": hash table #" << id
           << ", sequence " << seq_rep(v) << " " << state << endl;
    }

    bool
    assert_not_present(const string &func_name, unsigned long id, key_t &v) {
      if (get_hashsets().at(id).count(v) == 0) return true;
      if (debug) cerr_seq_state(func_name, id, v, "was present");
      return false;
    }

    bool
    assert_is_present(const string &func_name, unsigned long id, key_t &v) {
      if (get_hashsets().at(id).count(v) > 0) return true;
      if (debug) cerr_seq_state(func_name, id, v, "was not present");
      return false;
    }

    bool
    check_args (const string &func_name, uint64_t const *seq, size_t size) {
      bool passed = true;

      if (!seq) {
        if (debug)
          cerr << func_name << ": invalid pointer (" << seq_rep(seq, size)
               << ")" << endl;
        passed = false;
      }

      if (!size) {
        if (debug)
          cerr << func_name << ": invalid size (" << size << ")" << endl;
        passed = false;
      }

      return passed;
    }

    void print_fun_call(const string &func_name, unsigned long id, uint64_t
    const *seq, size_t size) {
      if (debug)
        cerr << func_name << "(" << id << ", " << seq_rep(seq, size) <<
             ", " << size << ")" << endl;
    }
  }

  unsigned long hash_create(hash_function_t hash_function) {
    if (debug) cerr << __func__ << "(" << (void*) hash_function << ")" << endl;
    hashset_t new_hashset(INITIAL_SIZE, custom_hash(hash_function));

    static id_t last_id = 0;
    get_hashsets().emplace(last_id, new_hashset);

    if (debug)
      cerr << __func__ << ": hash table #" << last_id << " created"
           << endl;

    return last_id++;
  }

  bool hash_insert(unsigned long id, uint64_t const *seq, size_t size) {
    print_fun_call(__func__, id, seq, size);

    if (!check_args(__func__, seq, size)) return false;
    if (!check_table_exists(__func__, id)) return false;

    key_t v(size);
    copy(seq, seq + size, v.begin());

    if (!assert_not_present(__func__, id, v)) return false;

    if (get_hashsets().at(id).emplace(v).second) {
      if (debug) cerr_seq_state(__func__, id, v, "inserted");
      return true;
    }
    return false;
  }

  void hash_delete(unsigned long id) {
    if (debug) cerr << __func__ << "(" << id << ")" << endl;

    if (check_table_exists(__func__, id)) {
      get_hashsets().erase(id);
      if (debug)
        cerr << __func__
             << ": hash table #" << id << " deleted" << endl;
    }
  }

  size_t hash_size(unsigned long id) {
    if (debug) cerr << __func__ << "(" << id << ")" << endl;
    if (!check_table_exists(__func__, id)) return 0;

    size_t count = get_hashsets().at(id).size();

    if (debug)
      cerr << __func__
           << ": hash table #" << id << " contains " << count << " element(s)"
           << endl;

    return count;
  }

  bool hash_remove(unsigned long id, uint64_t const *seq, std::size_t size) {
    print_fun_call(__func__, id, seq, size);

    if (!check_args(__func__, seq, size)) return false;
    if (!check_table_exists(__func__, id)) return false;

    key_t v(size);
    copy(seq, seq + size, v.begin());

    if (assert_is_present(__func__, id, v)) {
      if (debug) cerr_seq_state(__func__, id, v, "removed");
      return get_hashsets().at(id).erase(v) == 1;
    } else
      return false;
  }

  void hash_clear(unsigned long id) {
    if (debug) cerr << __func__ << "(" << id << ")" << endl;
    if (!check_table_exists(__func__, id)) return;

    bool empty;
    if (!(empty = get_hashsets().at(id).empty()))
      get_hashsets().at(id).clear();

    if (debug)
      cerr << __func__ << ": hash table #" << id
           << (empty ? " was empty" : " cleared") << endl;
  }

  bool hash_test(unsigned long id, uint64_t const *seq, std::size_t size) {
    print_fun_call(__func__, id, seq, size);

    if (!check_args(__func__, seq, size)) return false;
    if (!check_table_exists(__func__, id)) return false;

    key_t v(size);
    copy(seq, seq + size, v.begin());

    bool present;

    present = get_hashsets().at(id).count(v);
    if (debug)
      cerr_seq_state(__func__, id, v,
                     present ? "is present" : "is not present");
    return present;
  }
}