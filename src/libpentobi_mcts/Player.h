//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/Player.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_MCTS_PLAYER_H
#define LIBPENTOBI_MCTS_PLAYER_H

#include <boost/filesystem.hpp>
#include "Search.h"
#include "libpentobi_base/Book.h"
#include "libpentobi_base/Player.h"

namespace libpentobi_mcts {

using boost::filesystem::path;
using libpentobi_base::Book;
using libpentobi_base::GameVariant;

//-----------------------------------------------------------------------------

class Player
    : public libpentobi_base::Player
{
public:
    /** Constructor.
        @param bd
        @param application_dir_path Directory of the main executable (potential
        location of book files). */
    Player(const Board& bd, GameVariant game_variant,
           const path& application_dir_path = path());

    ~Player() throw();

    Move genmove(Color c);

    ValueType get_fixed_simulations() const;

    double get_fixed_time() const;

    /** Use a fixed number of simulations in the search.
        If set to a value greater than zero, this value will enforce a
        fixed number of simulations per search independent of the playing
        level. */
    void set_fixed_simulations(ValueType n);

    /** Use a fixed time limit per move.
        If set to a value greater than zero, this value will set a fixed
        (maximum) time per search independent of the playing level. */
    void set_fixed_time(double seconds);

    bool get_use_book() const;

    void set_use_book(bool enable);

    int get_level() const;

    void set_level(int level);

    Search& get_search();

    void load_book(istream& in);

protected:
    bool m_is_book_loaded;

    bool m_use_book;

    path m_application_dir_path;

    int m_level;

    array<float, Board::max_player_moves> weight_max_count_classic;

    array<float, Board::max_player_moves> weight_max_count_duo;

    ValueType m_fixed_simulations;

    double m_fixed_time;

    Search m_search;

    Book m_book;

    void load_book(const string& filename);

    bool try_load_book(const path& filepath);
};

inline ValueType Player::get_fixed_simulations() const
{
    return m_fixed_simulations;
}

inline double Player::get_fixed_time() const
{
    return m_fixed_time;
}

inline int Player::get_level() const
{
    return m_level;
}

inline bool Player::get_use_book() const
{
    return m_use_book;
}

inline Search& Player::get_search()
{
    return m_search;
}

inline void Player::set_fixed_simulations(ValueType n)
{
    m_fixed_simulations = n;
    m_fixed_time = 0;
}

inline void Player::set_fixed_time(double seconds)
{
    m_fixed_time = seconds;
    m_fixed_simulations = 0;
}

inline void Player::set_level(int level)
{
    m_level = level;
    m_fixed_simulations = 0;
    m_fixed_time = 0;
}

inline void Player::set_use_book(bool enable)
{
    m_use_book = enable;
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts

#endif // LIBPENTOBI_MCTS_PLAYER_H
