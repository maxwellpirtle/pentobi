//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/Search.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_MCTS_SEARCH_H
#define LIBPENTOBI_MCTS_SEARCH_H

#include "State.h"
#include "libboardgame_mcts/Search.h"
#include "libpentobi_base/GameStateHistory.h"

namespace libpentobi_mcts {

using libboardgame_util::Timer;
using libboardgame_util::TimeSource;
using libpentobi_base::GameStateHistory;
using libpentobi_base::Setup;

//-----------------------------------------------------------------------------

/** Monte-Carlo tree search implementation for Blokus.
    @note @ref libboardgame_avoid_stack_allocation */
class Search
    : public libboardgame_mcts::Search<State, Move, 4>
{
public:
    Search(Variant initial_variant, size_t memory = 0);

    ~Search() throw();


    /** @name Pure virtual functions of libboardgame_mcts::Search */
    // @{

    string get_move_string(Move mv) const;

    unsigned int get_nu_players() const;

    unsigned int get_player() const;

    // @} // @name


    /** @name Overriden virtual functions of libboardgame_mcts::Search */
    // @{

    bool check_followup(vector<Move>& sequence);

    void write_info(ostream& out) const;

    // @} // @name


    /** @name Parameters */
    // @{

    Float get_score_modification() const;

    void set_score_modification(Float value);

    bool get_detect_symmetry() const;

    void set_detect_symmetry(bool enable);

    bool get_avoid_symmetric_draw() const;

    void set_avoid_symmetric_draw(bool enable);

    /** Automatically set some user-changeable parameters that have different
        optimal values for different game variants whenever the game variant
        changes.
        Default is true. */
    bool get_auto_param() const;

    void set_auto_param(bool enable);

    // @} // @name


    bool search(Move& mv, const Board& bd, Color to_play, Float max_count,
                size_t min_simulations, double max_time,
                TimeSource& time_source);

    /** Get color to play at root node of the last search. */
    Color get_to_play() const;

    const GameStateHistory& get_last_state() const;

    /** Get board position of last search at root node as setup.
        @param[out] variant
        @param[out] setup */
    void get_root_position(Variant& variant, Setup& setup) const;

protected:
    void on_start_search();

private:
    typedef libboardgame_mcts::Search<State, Move, 4> ParentClass;

    /** Automatically set default parameters for the game variant if
        the game variant changes. */
    bool m_auto_param;

    /** Game variant of last search. */
    Variant m_variant;

    Color m_to_play;

    SharedConst m_shared_const;

    /** Local variable reused for efficiency. */
    GameStateHistory m_state;

    GameStateHistory m_last_state;

    const Board& get_board() const;

    void set_default_param(Variant variant);
};

inline bool Search::get_auto_param() const
{
    return m_auto_param;
}

inline bool Search::get_avoid_symmetric_draw() const
{
    return m_shared_const.avoid_symmetric_draw;
}

inline const Board& Search::get_board() const
{
    return *m_shared_const.board;
}

inline bool Search::get_detect_symmetry() const
{
    return m_shared_const.detect_symmetry;
}

inline const GameStateHistory& Search::get_last_state() const
{
    return m_last_state;
}

inline unsigned int Search::get_nu_players() const
{
    return get_board().get_nu_colors();
}

inline unsigned int Search::get_player() const
{
    return m_to_play.to_int();
}

inline Float Search::get_score_modification() const
{
    return m_shared_const.score_modification;
}

inline Color Search::get_to_play() const
{
    return m_to_play;
}

inline void Search::set_auto_param(bool enable)
{
    m_auto_param = enable;
}

inline void Search::set_avoid_symmetric_draw(bool enable)
{
    m_shared_const.avoid_symmetric_draw = enable;
}

inline void Search::set_detect_symmetry(bool enable)
{
    m_shared_const.detect_symmetry = enable;
}

inline void Search::set_score_modification(Float value)
{
    m_shared_const.score_modification = value;
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts

#endif // LIBPENTOBI_MCTS_SEARCH_H
