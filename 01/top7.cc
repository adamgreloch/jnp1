#include <iostream>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

/// Wejście, wyjście
using std::string, std::to_string;
using std::getline;
using std::cout, std::cin, std::cerr, std::endl;

/// Struktury danych.
using std::vector, std::unordered_map, std::unordered_set, std::pair,
      std::tuple;

/// Narzędzia
using std::sort, std::min, std::stoi,
      std::make_tuple, std::istringstream;

/// Linie odczytu i jej numeracja.
using line_t = string;
using line_num_t = size_t;

/// Zbiór utworów, które wypadły z głosowania.
using track_id_t = int_least32_t;
using dropped_tracks_t = unordered_set<track_id_t>;

/// Wektor zbiorów głosów zebranych w pojedynczej liniach.
using vote_set_t = unordered_set<track_id_t>;

/** Notowanie (podsumowanie) utworów.
 * $1 - numer utworu;
 * $2 - ilość głosów (punktów) zdobyta przez ten utwór.
 */
using count_t = size_t;
using count_per_track_t = unordered_map<track_id_t, count_t>;

/** Rozkład miejsc w poprzednim notowaniu (podsumowaniu) utworów.
 * $1 - numer utworu;
 * $2 - pozycja w poprzednim new7 (top7).
 */
using track_rank_t = int_least32_t;
using unordered_ranks_t = unordered_map<track_id_t, track_rank_t>;

/// Posortowane przeboje. Rozmiar niekoniecznie musi być 7. Wartością są punkty.
using top7_pair = pair<track_id_t, count_t>;
using top7_t = vector<top7_pair>;

/// Posortowane utwory. Rozmiar niekoniecznie musi być 7. Wartością są
/// pozycje w rankingu.
using sorted_rank_pair = pair<track_id_t, track_rank_t>;
using ordered_ranks_t = vector<sorted_rank_pair>;

  /** Przechowywane struktury danych.
   * $0 - zebrane punkty przez poszczególne utwory;
   * $1 - rozkład miejsc w poprzednim podsumowaniu;
   * $2 - zbiór utworów, które wypadły z losowania;
   * $3 - rozkład miejsc w poprzednim notowaniu;
   * $4 - wektor zbiorów głosów zebranych w pojedynczych liniach.
   */
using hit_list_t = tuple<count_per_track_t, unordered_ranks_t,
  dropped_tracks_t, unordered_ranks_t, count_per_track_t>;

namespace {

  /// Rodzaje poleceń.
  enum cmd_t {
    Vote, New, Top, Empty
  };

  /// Klucz w strukturze danych poprzedniego notowania do bieżącego MAX.
  int const MAX_VOTE_ID = 100000000;

  /// Liczba przebojów pojedynczego notowania (podsumowania).
  unsigned const TOP_COUNT = 7;

  /** Przechowywane struktury danych.
   * $0 - zebrane punkty przez poszczególne utwory;
   * $1 - rozkład miejsc w poprzednim podsumowaniu;
   * $2 - zbiór utworów, które wypadły z losowania;
   * $3 - rozkład miejsc w poprzednim notowaniu;
   * $4 - wektor zbiorów głosów zebranych w pojedynczych liniach.
   */
  int const POINTS = 0;
  int const PREVIOUS_OVERALL = 1;
  int const DROPPED = 2;
  int const PREVIOUS_LISTING = 3;
  int const POLL = 4;

  /// Funkcje przeznaczone do parsowania.
  namespace IO {

    using std::regex;

    string const DELIM(" ");
    string const NEW("NEW");
    string const TOP("TOP");
    int const NUM_LEN = 8;

    string const whitespace_str(" \t\f\v\n\r");
    regex const whitespace_expr("[[:space:]]+");
    regex const num_prefix0_expr(DELIM + "0");
    regex const num_len_expr("[0-9]{" + to_string(NUM_LEN + 1) + "}");
    regex const only_digits_expr("[0-9]+");
    regex const digits_spaces_expr("[0-9[:space:]]+");

    /** Obcina z linii białe znaki na początku i na końcu, a wszystkie w
     * środku zamienia na napis DELIM.
     * @param[in, out] line - linia wejścia do przycięcia.
     */
    void trim_spaces(line_t &line) {
      line.erase(line.find_last_not_of(whitespace_str) + 1);
      line.erase(0, line.find_first_not_of(whitespace_str));
      line = regex_replace(line, whitespace_expr, DELIM);
    }

    /** Funkcja pomocnicza dla parse_command().
     * @param[in, out] line - pojedyncza linia wejścia. Jeżeli polecenie
     * zostało rozpoznane, napis jest o to polecenie obcięty;
     * @param[in] cmd - polecenie rozpoczynające linię.
     * @return Wartość true, jeżeli line rozpoczyna się napisem cmd,
     * false w przeciwnym przypadku.
     */
    bool match_command(line_t &line, string const &cmd) {
      if (line.rfind(cmd, 0) == 0) {
        line.erase(0, cmd.length() + DELIM.length()); // Usuń następujący separator.
        return true;
      }
      return false;
    }

    /** Sprawdza polecenie, które jest wyznaczone przez pierwsze słowo wejścia.
     * @param[in, out] line - linia wejścia z wyrazami oddzielonymi o napis
     * DELIM, którą rozpoczyna polecenie. Po wykonaniu funkcji napis jest
     * obcięty o polecenie.
     * @return Zwraca polecenie określone na wejściu.
     */
    cmd_t parse_command(line_t &line) {
      cmd_t cmd = Vote;
      if (line.empty())
        cmd = Empty;
      else if (match_command(line, NEW))
        cmd = New;
      else if (match_command(line, TOP))
        cmd = Top;
      return cmd;
    }

    /** Sprawdza, czy zapis liczb jest poprawny.
     * Liczba nie może zaczynać się od zera, jeżeli zerem nie jest, ani
     * przekraczać długości NUM_LEN.
     * @param[in, out] line - napis przedstawiający liczby oddzielone
     * napisem DELIM.
     * @return True, jeżeli nie ma błędnej liczby, false w przeciwnym razie.
     */
    bool validate_numbers(line_t const &line) {
      return
          !regex_search(DELIM + line, num_prefix0_expr) &&
          !regex_search(line, num_len_expr);
    }

    /** Sprawdza, czy parametry polecenia są poprawne.
     * @param[in] line - napis następujący polecenie,
     * @param[in] cmd - polecenie.
     * @return True, jeżeli parametry spełniają specyfikacje polecenia,
     * false w przeciwnym razie albo wtedy, gdy polecenie nie jest rozpoznane.
     */
    bool validate_parameters(line_t const &line, cmd_t const cmd) {
      switch (cmd) {
        case Vote:
          return regex_match(line, digits_spaces_expr) &&
                 validate_numbers(line);
        case New:
          return regex_match(line, only_digits_expr) &&
                 validate_numbers(line);
        case Top:
          return line.empty();
        case Empty:
          return true;
        default:
          return false;
      }
    }
  }

  /// Funkcje przeznaczone do notowań.
  namespace hit_list::list {

    /** Wyrzuca utwory z głosowania.
     * @param previous - poprzednie notowanie;
     * @param current - obecne notowanie;
     * @param dropped - utwory, które wypadły.
     */
    void drop_tracks(unordered_ranks_t const &previous,
                     unordered_ranks_t const &current,
                     dropped_tracks_t &dropped) {

      for (auto const &[id, rank]: previous) {
        if (current.find(id) == current.end())
          dropped.insert(id);
      }
    }

    /** Resetuje głosy, aktualizuje max i zapisuje notowanie.
     * @param previous - poprzednie notowanie;
     * @param current - obecne notowanie;
     * @param poll - głosy;
     * @param max - maksymalny indeks dostępnych utworów.
     */
    void initialize_listing(unordered_ranks_t &previous,
                            unordered_ranks_t &current,
                            count_per_track_t &poll,
                            track_id_t const &max) {
      previous.swap(current);
      previous[MAX_VOTE_ID] = max;

      poll = count_per_track_t();
    }
  }

  /// Funkcje przeznaczone do głosowań.
  namespace hit_list::poll {

    /** Pobiera z napisu głosy i sprawdza, czy są poprawne.
     * @param[in, out] votes - głosy na daną linijkę;
     * @param[in] dropped - utwory, które wypadły z głosowania;
     * @param[in] line - linijka zawierająca głosy;
     * @param[in] max - maksymalny indeks utworu.
     * @return Wartość true, jeżeli dane są poprawne, wartość false w
     * przeciwnym razie. (Niepoprawne, jeżeli utwór przepadł lub głos
     * jest oddany ponad max).
     */
    bool fetch_votes(vote_set_t &votes, dropped_tracks_t const &dropped,
                     line_t const &line, track_id_t const &max) {

      bool valid_arguments = true;
      track_id_t track_id;
      istringstream stream(line);

      while (stream >> track_id && valid_arguments) {
        if (track_id > max || dropped.count(track_id) || votes.count(track_id))
          valid_arguments = false;

        votes.insert(track_id);
      }
      return valid_arguments;
    }

    /** Aktualizuje liczbę głosów na każdy utwór.
     * @param poll - aktualne głosy w bieżącym głosowaniu;
     * @param votes - głosy w danej linijce.
     */
    void update_poll(count_per_track_t &poll,
                     vote_set_t const &votes) {

      for (auto const &vote: votes)
        poll[vote]++;
    }
  }

  /// Funkcje przeznaczone do podsumowań.
  namespace hit_list::top {

    string const DELIM = " ";
    string const RANK_NO_CHANGE = "-";

    /**
     * Porównuje punktację dwóch utworów.
     * @param a - para zawierająca numer pierwszego utworu oraz liczbę jego
     * punktów.
     * @param b - para zawierająca numer drugiego utworu oraz liczbę jego
     * punktów.
     * @return @p true, jeśli pierwszy utwór jest wyżej w rankingu, @p false
     * w przeciwnym wypadku.
     */
    bool compare_points(top7_pair const &a, top7_pair const &b) {
      if (a.second == b.second)
        return a.first < b.first;
      else
        return a.second > b.second;
    }

    /** Tworzy ranking siedmiu najlepszych utworów na podstawie zdobytych
     * przez nie punktów.
     * @param[in] points - mapa zawierająca numery i związane z nimi liczby
     * punktów.
     * @return Zwraca parę zawierającą odpowiednio ranking dostępny po
     * numerze utworu (bez zachowania porządku), oraz ranking dostępny po
     * pozycji utworu.
     */
    pair<unordered_ranks_t, ordered_ranks_t>
    fetch_ranking(count_per_track_t const &points) {

      top7_t ranking(points.begin(), points.end());
      sort(ranking.begin(), ranking.end(), compare_points);

      track_id_t id;
      unordered_ranks_t updated_ranking;
      ordered_ranks_t sorted_updated_ranking;

      for (size_t rank = 1;
           rank <= min((size_t) TOP_COUNT, ranking.size()); rank++) {

        tie(id, std::ignore) = ranking[rank - 1];
        sorted_updated_ranking.push_back({id, rank});
        updated_ranking.insert({id, rank});
      }

      return {updated_ranking, sorted_updated_ranking};
    }

    /** Zwraca ranking siedmiu najlepszych utworów na wyjście standardowe,
     * według liczby punktów/głosów na poszczególne utwory, zawartych w @p
     * current_ranking. W rankingu, obok numeru utworu, obliczana jest też
     * różnica obecnej pozycji względem tej w poprzednim rankingu, określonej w
     * @p previous_ranking.
     * @param[in] previous_ranking - siedem najlepszych utworów w poprzednim rankingu
     * @param[in] current_ranking - siedem najlepszych utworów w obecnym rankingu.
     * @param[in] current_ordered_ranking - siedem najlepszych utworów w
     * obecnym rankingu, lecz w strukturze utrzymującej uporządkowanie
     * malejąco względem liczby punktów/głosów.
     */
    void print_top7(unordered_ranks_t const &previous_ranking,
                    unordered_ranks_t const &current_ranking,
                    ordered_ranks_t const &current_ordered_ranking) {

      for (auto &pair: current_ordered_ranking) {
        cout << pair.first << DELIM;
        if (!previous_ranking.contains(pair.first))
          cout << RANK_NO_CHANGE;
        else
          cout <<
               previous_ranking.at(pair.first) - current_ranking.at(pair.first);
        cout << endl;
      }
    }

    /** Funkcja przyznaje punkty siedmiu pierwszym utworom na podstawie ich
     * pozycji.
     * @param[in] points - mapa z generalną klasyfikacją punktową;
     * @param[in] listing - nowe notowanie, zawierające siedem najlepszych
     * utworów.
     */
    void grant_points(count_per_track_t &points,
                      unordered_ranks_t const &listing) {

      for (const pair<const int, track_rank_t> &pair: listing)
        points[pair.first] += TOP_COUNT + 1 - pair.second;
    }
  }

  /// Główne funkcje hit_list, odpalane z run().
  namespace hit_list::run {

    /** Wykonuje NEW MAX.
     * @param[in, out] previous - poprzednie notowanie;
     * @param[in, out] dropped - utwory, które wypadły z głosowania;
     * @param[in, out] poll - głosy w obecnym notowaniu;
     * @param[in, out] ranking - obecny ranking punktów;
     * @param[in] max - nowy MAX w NEW MAX.
     * @return Wartość true, jeżeli dane są poprawne, wartość false w
     * przeciwnym razie. (Niepoprawne, jeżeli nowy max < stary max).
     */
    bool run_new(unordered_ranks_t &previous_listing,
                 dropped_tracks_t &dropped,
                 count_per_track_t &poll, count_per_track_t &ranking,
                 track_id_t const &max) {

      using namespace list;
      using top::fetch_ranking;
      using top::print_top7;
      using top::grant_points;

      if (max >= previous_listing[MAX_VOTE_ID]) {
        unordered_ranks_t current_listing;
        ordered_ranks_t listing_order;

        tie(current_listing, listing_order) = fetch_ranking(poll);
        grant_points(ranking, current_listing);
        print_top7(previous_listing, current_listing, listing_order);
        drop_tracks(previous_listing, current_listing, dropped);

        initialize_listing(previous_listing, current_listing, poll, max);
        return true;
      }

      return false;
    }

    /** Wykonuje głosy.
     * @param[in] poll - aktualne głosy w obecnym notowaniu;
     * @param[in, out] dropped - utwory, które wypadły z głosowania;
     * @param[in] line - utwory, na które zagłosowano;
     * @param[in] max - maksymalny indeks utworu do zagłosowania.
     * @return Wartość true, jeżeli dane są poprawne, wartość false w
     * przeciwnym razie.
     */
    bool run_vote(count_per_track_t &poll,
                  dropped_tracks_t const &dropped,
                  string const &line, int const &max) {

      using namespace poll;
      vote_set_t vote_set;

      if (fetch_votes(vote_set, dropped, line, max))
        update_poll(poll, vote_set);
      else
        return false;

      return true;
    }

    /**
     * Wykonuje polecenie TOP. Aktualizuje ranking ogólny i umieszcza
     * go na STDOUT. Zastępuje @p previous_overall zaktualizowanym rankingiem.
     * @param[in] points - struktura przechowująca łączną liczbę zdobytych
     * punktów dla każdego utworu;
     * @param[in,out] previous_overall - struktura przechowująca pozycje siedmiu
     * najlepszych utworów w poprzednim rankingu ogólnym;
     * @return
     */
    bool run_top(count_per_track_t const &points,
                 unordered_ranks_t &previous_overall) {

      pair<unordered_ranks_t, ordered_ranks_t> ranking_pair = top::fetch_ranking
          (points);
      top::print_top7(previous_overall,
                      ranking_pair.first, ranking_pair.second);

      previous_overall.swap(ranking_pair.first);
      return true;
    }
  }

  /** Wypisuje komunikat o błędzie w linii na wejściu diagnostycznym.
   * @param[in] num - numer linii;
   * @param[in] line - linia na której jest błąd.
   */
  void error_write(line_num_t const &num, line_t const &line) {
    cerr << "Error in line " << num << ": " << line << endl;
  }

  /** Oczyszcza linię wejścia ze zbędnych danych, rozpoznaje polecenie, a
   * także sprawdza jej poprawność. Dane są oddzielone napisem DELIM.
   * @param[in, out] line - skanowany napis;
   * @param[in, out] cmd - rodzaj polecenia.
   * @return Wartość true, jeżeli dane są poprawne; wartość false w
   * przeciwnym razie.
   */
  bool parse_line(cmd_t &cmd, line_t &line) {
    using namespace IO;
    trim_spaces(line);
    cmd = parse_command(line);
    return validate_parameters(line, cmd);
  }

  /** Uruchamia listę przebojów. Jeżeli kończy działanie wartością false,
   * to nie zachodzą żadne zmiany w strukturach danych.
   * @param[in] cmd - typ polecenia;
   * @param[in] line - parametry polecenia.
   * @return Wartość true, jeżeli dane są poprawne, false w przeciwnym razie.
   */
  bool run(hit_list_t &data, cmd_t const &cmd, line_t const &line) {
    using namespace hit_list::run;
    switch (cmd) {
      case New:
        return run_new(get<PREVIOUS_LISTING>(data), get<DROPPED>(data),
                       get<POLL>(data), get<POINTS>(data), stoi(line));
      case Vote:
        return run_vote(get<POLL>(data), get<DROPPED>(data),
                        line, get<PREVIOUS_LISTING>(data)[MAX_VOTE_ID]);
      case Top:
        return run_top(get<POINTS>(data),
                       get<PREVIOUS_OVERALL>(data));
      default:
        return true; // tu będzie Empty
    }
  }
}

int main() {
  line_num_t line_num = 0;
  cmd_t cmd;
  line_t line;
  hit_list_t data = make_tuple(count_per_track_t(), unordered_ranks_t(),
                               dropped_tracks_t(), unordered_ranks_t(),
                               count_per_track_t());

  get<PREVIOUS_LISTING>(data).insert({MAX_VOTE_ID, 0}); // MAX=0 przed notowaniem.

  while (getline(cin, line)) {
    line_num++;
    line_t line_orig(line);

    if (!parse_line(cmd, line) || !run(data, cmd, line))
      error_write(line_num, line_orig);
  }
  return 0;
}
