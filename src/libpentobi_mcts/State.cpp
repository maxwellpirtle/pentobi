//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/State.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "State.h"

#include "libboardgame_util/FmtSaver.h"
#include "libpentobi_base/BoardUtil.h"

namespace libpentobi_mcts {

using namespace std;
using libboardgame_util::FmtSaver;
using libpentobi_base::BoardType;
using libpentobi_base::ColorIterator;
using libpentobi_base::PointState;

//-----------------------------------------------------------------------------

inline Float sigmoid(Float steepness, Float x)
{
    return -1.f + 2.f / (1.f + exp(-steepness * x));
}

//-----------------------------------------------------------------------------

SharedConst::SharedConst(const Color& to_play)
    : board(nullptr),
      to_play(to_play),
      avoid_symmetric_draw(true)
{
    // Game variant and position dependent variables are initialized in
    // libpentobi_mcts::Search::start_search()
}

//-----------------------------------------------------------------------------

State::State(Variant initial_variant, const SharedConst& shared_const)
  : m_shared_const(shared_const),
    m_bd(initial_variant)
{
}

inline void State::add_move(MoveList& moves, Move mv, double gamma)
{
    m_total_gamma += gamma;
    m_cumulative_gamma[moves.size()] = m_total_gamma;
    moves.push_back(mv);
}

inline void State::add_moves(Point p, Color c,
                             const Board::PiecesLeftList& pieces_considered)
{
    auto& moves = m_moves[c];
    auto& marker = m_marker[c];
    auto adj_status = m_bd.get_adj_status(p, c);
    auto& is_forbidden = m_bd.is_forbidden(c);
    double gamma;
    for (Piece piece : pieces_considered)
        for (Move mv : get_moves(c, piece, p, adj_status))
            if (! marker[mv]
                && check_move(is_forbidden, get_move_info(mv), gamma))
            {
                marker.set(mv);
                add_move(moves, mv, gamma);
            }
    m_moves_added_at[c].set(p);
}

inline void State::add_moves(Point p, Color c, Piece piece,
                             unsigned adj_status)
{
    auto& moves = m_moves[c];
    auto& marker = m_marker[c];
    auto& is_forbidden = m_bd.is_forbidden(c);
    double gamma;
    for (Move mv : get_moves(c, piece, p, adj_status))
        if (! marker[mv] && check_move(is_forbidden, get_move_info(mv), gamma))
        {
            marker.set(mv);
            add_move(moves, mv, gamma);
        }
}

void State::add_starting_moves(Color c,
                               const Board::PiecesLeftList& pieces_considered,
                               bool with_gamma)
{
    // Using only one starting point (if game variant has more than one) not
    // only reduces the branching factor but is also necessary because
    // update_moves() assumes that a move stays legal if the forbidden
    // status for all of its points does not change.
    Point p = find_best_starting_point(c);
    if (p.is_null())
        return;
    auto& moves = m_moves[c];
    auto& marker = m_marker[c];
    auto& is_forbidden = m_bd.is_forbidden(c);
    for (Piece piece : pieces_considered)
    {
        for (Move mv : get_moves(c, piece, p, 0))
        {
            LIBBOARDGAME_ASSERT(! marker[mv]);
            if (check_move_without_gamma(is_forbidden, mv))
            {
                marker.set(mv);
                if (with_gamma)
                    add_move(moves, mv, m_gamma_piece[piece]);
                else
                    moves.push_back(mv);
            }
        }
    }
}

/** Check if move is not forbidden and compute/handle its local value in the
    same loop. */
bool State::check_move(const Grid<bool>& is_forbidden, const MoveInfo& info,
                       double& gamma)
{
    auto i = info.begin();
    if (is_forbidden[*i])
        return false;
    LocalValue::Compute local(*i, m_local_value);
    auto end = info.end();
    while (++i != end)
    {
        if (is_forbidden[*i])
            return false;
        local.add_move_point(*i, m_local_value);
    }
    gamma = m_gamma_piece[info.get_piece()];
    if (local.has_local())
    {
        gamma *= m_gamma_nu_attach[local.get_nu_attach()];
        if (local.has_adj_attach())
            gamma *= 1e5;
    }
    return true;
}

bool State::check_move_without_gamma(const Grid<bool>& is_forbidden, Move mv)
{
    auto& info = get_move_info(mv);
    auto i = info.begin();
    if (is_forbidden[*i])
        return false;
    auto end = info.end();
    while (++i != end)
        if (is_forbidden[*i])
            return false;
    return true;
}

void State::dump(ostream& out) const
{
    out << "pentobi_mcts::State:\n";
    libpentobi_base::boardutil::dump(m_bd, out);
}

/** Get the game result for each color.
    The result is 0,0.5,1 for loss/tie/win in 2-player variants. If there
    are n \> 2 players, this is generalized in the following way: The scores
    are sorted in ascending order. Each rank r_i (i in 0..n-1) is assigned
    a result value of r_i/(n-1). If multiple players have the same score,
    the result value is the average of all ranks with this score. So being
    the single winner still gives the result 1 and having the lowest score
    gives the result 0. Being the single winner is better than sharing the
    best place, which is better than getting the second place, etc.

    Bonuses are added to the result to encorage wins with larger scores and
    shorter game length. */
array<Float, 4> State::evaluate_playout()
{
    // Always evaluate symmetric positions as a draw in the playouts. This
    // will encourage the first player to break the symmetry and the second
    // player to preserve it.
    if (! m_is_symmetry_broken
        && m_bd.get_nu_onboard_pieces() >= m_symmetry_min_nu_pieces)
    {
        if (log_simulations)
            log("Result: 0.5 (symmetry)");
        array<Float, 4> result;
        fill(result.begin(), result.end(), 0.5);
        return result;
    }

    auto nu_players = m_bd.get_nu_players();
    ColorMap<Float> points;
    ColorMap<Float> score;
    for (ColorIterator i(m_nu_colors); i; ++i)
        points[*i] = Float(m_bd.get_points_with_bonus(*i));
    for (Color::IntType i = 0; i < nu_players; ++i)
        score[Color(i)] = Float(m_bd.get_score(Color(i)));
    if (m_nu_colors > nu_players)
    {
        LIBBOARDGAME_ASSERT(m_nu_colors == 4);
        score[Color(2)] = score[Color(0)];
        score[Color(3)] = score[Color(1)];
    }
    array<Float, Color::range> sorted_points;
    if (nu_players > 2)
    {
        for (ColorIterator i(m_nu_colors); i; ++i)
            sorted_points[(*i).to_int()] = points[*i];
        sort(sorted_points.begin(), sorted_points.begin() + m_nu_colors);
    }
    array<Float, 4> result_array;
    for (Color::IntType i = 0; i < nu_players; ++i)
    {
        Color c(i);
        auto s = score[c];
        Float game_result;
        if (nu_players == 2)
        {
            if (i == 1)
            {
                result_array[1] = 1.f - result_array[0];
                break;
            }
            if (s > 0)
                game_result = 1;
            else if (s < 0)
                game_result = 0;
            else
                game_result = 0.5;
        }
        else
        {
            game_result = 0;
            Float n = 0;
            for (Color::IntType j = 0; j < m_nu_colors; ++j)
                if (sorted_points[j] == points[c])
                {
                    game_result += Float(j) / Float(m_nu_colors - 1);
                    ++n;
                }
            game_result /= n;
        }

        Float res = game_result;
        {
            auto& stat = m_stat_score[c];
            stat.add(s);
            Float dev = stat.get_deviation();
            if (dev > 0)
                res += 0.2f * sigmoid(2.0f, (s - stat.get_mean()) / dev);
        }
        {
            Float l = static_cast<Float>(m_bd.get_nu_moves());
            auto& stat = m_stat_len;
            stat.add(l);
            Float dev = stat.get_deviation();
            if (dev > 0)
            {
                if (game_result == 1)
                    res -= 0.12f * sigmoid(2.0f, (l - stat.get_mean()) / dev);
                else if (game_result == 0)
                    res += 0.12f * sigmoid(2.0f, (l - stat.get_mean()) / dev);
            }
        }
        result_array[i] = res;
        if (log_simulations)
            log() << "Result color " << c << ": sco=" << s << " game_res="
                  << game_result << " res=" << res << '\n';
    }
    if (m_nu_colors > nu_players)
    {
        LIBBOARDGAME_ASSERT(m_nu_colors == 4);
        result_array[2] = result_array[0];
        result_array[3] = result_array[1];
    }
    return result_array;
}

Point State::find_best_starting_point(Color c) const
{
    // We use the starting point that maximizes the distance to occupied
    // starting points, especially to the ones occupied by the player (their
    // distance is weighted with a factor of 2)
    Point best = Point::null();
    float max_distance = -1;
    bool is_trigon = (m_bd.get_board_type() == BoardType::trigon
                      || m_bd.get_board_type() == BoardType::trigon_3);
    float ratio = (is_trigon ? 1.732f : 1);
    auto width = m_bd.get_geometry().get_width();
    for (Point p : m_bd.get_starting_points(c))
    {
        if (m_bd.is_forbidden(p, c))
            continue;
        float px = static_cast<float>(p.get_x(width));
        float py = static_cast<float>(p.get_y(width));
        float d = 0;
        for (ColorIterator i(m_nu_colors); i; ++i)
        {
            for (Point pp : m_bd.get_starting_points(*i))
            {
                PointState s = m_bd.get_point_state(pp);
                if (! s.is_empty())
                {
                    float ppx = static_cast<float>(pp.get_x(width));
                    float ppy = static_cast<float>(pp.get_y(width));
                    float dx = ppx - px;
                    float dy = ratio * (ppy - py);
                    float weight = 1;
                    if (s == c || s == m_bd.get_second_color(c))
                        weight = 2;
                    d += weight * sqrt(dx * dx + dy * dy);
                }
            }
        }
        if (d > max_distance)
        {
            best = p;
            max_distance = d;
        }
    }
    return best;
}

bool State::gen_playout_move(Move lgr1, Move lgr2, Move& mv)
{
    if (m_nu_passes == m_nu_colors)
        return false;

    if (! m_is_symmetry_broken
        && m_bd.get_nu_onboard_pieces() >= m_symmetry_min_nu_pieces)
    {
        // See also the comment in evaluate_playout()
        if (log_simulations)
            log("Terminate playout. Symmetry not broken.");
        return false;
    }

    ++m_nu_playout_moves;
    if (lgr2.is_regular() && m_bd.is_legal_nonpass(lgr2))
    {
        if (log_simulations)
            log("Playing last good reply 2");
        ++m_nu_last_good_reply_moves;
        mv = lgr2;
        return true;
    }
    if (lgr1.is_regular() && m_bd.is_legal_nonpass(lgr1))
    {
        if (log_simulations)
            log("Playing last good reply 1");
        ++m_nu_last_good_reply_moves;
        mv = lgr1;
        return true;
    }

    Color to_play;
    while (true)
    {
        to_play = m_bd.get_to_play();
        if (! m_is_move_list_initialized[to_play])
            init_moves_with_gamma(to_play);
        else if (m_has_moves[to_play])
            update_moves(to_play);
        if ((m_has_moves[to_play] = ! m_moves[to_play].empty()))
            break;
        if (m_nu_passes + 1 == m_nu_colors)
            return false;
        if (m_check_terminate_early && m_bd.get_score(to_play) < 0
            && ! m_has_moves[m_bd.get_second_color(to_play)])
        {
            if (log_simulations)
                log("Terminate early (no moves and negative score)");
            return false;
        }
        ++m_nu_passes;
        m_bd.set_to_play(m_bd.get_next(to_play));
        // Don't try to handle symmetry after pass moves
        m_is_symmetry_broken = true;
    }

    auto& moves = m_moves[to_play];
    if (log_simulations)
        log() << "Moves: " << moves.size() << ", total gamma: "
              << m_total_gamma << "\n";
    auto begin = m_cumulative_gamma.begin();
    auto end = begin + moves.size();
    auto random = m_total_gamma * m_random.generate_double();
    auto pos = lower_bound(begin, end, random);
    LIBBOARDGAME_ASSERT(pos != end);
    mv = moves[static_cast<unsigned>(pos - begin)];
    return true;
}

bool State::gen_and_play_playout_move(Move lgr1, Move lgr2)
{
    Move mv;
    if (gen_playout_move(lgr1, lgr2, mv))
    {
        play_playout(mv);
        return true;
    }
    return false;
}

inline const PieceMap<bool>& State::get_pieces_considered() const
{
    // Use number of on-board pieces for move number to handle the case where
    // there are more pieces on the board than moves (setup positions)
    unsigned nu_moves = m_bd.get_nu_onboard_pieces();
    if (nu_moves >= m_shared_const.min_move_all_considered
        || m_force_consider_all_pieces)
        return m_shared_const.is_piece_considered_all;
    else
        return *m_shared_const.is_piece_considered[nu_moves];
}

void State::init_moves_with_gamma(Color c)
{
    m_is_piece_considered[c] = &get_pieces_considered();
    m_local_value.init(m_bd);
    m_total_gamma = 0;
    auto& marker = m_marker[c];
    auto& moves = m_moves[c];
    marker.clear_all_set_known(moves);
    moves.clear();
    Board::PiecesLeftList pieces_considered;
    for (Piece piece : m_bd.get_pieces_left(c))
        if ((*m_is_piece_considered[c])[piece])
            pieces_considered.push_back(piece);
    if (m_bd.is_first_piece(c))
        add_starting_moves(c, pieces_considered, true);
    else
        for (Point p : m_bd.get_attach_points(c))
            if (! m_bd.is_forbidden(p, c))
                add_moves(p, c, pieces_considered);
    m_is_move_list_initialized[c] = true;
    m_new_moves[c].clear();
    if (moves.empty() && ! m_force_consider_all_pieces)
    {
        m_force_consider_all_pieces = true;
        init_moves_with_gamma(c);
    }
}

void State::init_moves_without_gamma(Color c)
{
    m_is_piece_considered[c] = &get_pieces_considered();
    auto& marker = m_marker[c];
    auto& moves = m_moves[c];
    marker.clear_all_set_known(moves);
    moves.clear();
    Board::PiecesLeftList pieces_considered;
    for (Piece piece : m_bd.get_pieces_left(c))
        if ((*m_is_piece_considered[c])[piece])
            pieces_considered.push_back(piece);
    auto& is_forbidden = m_bd.is_forbidden(c);
    if (m_bd.is_first_piece(c))
        add_starting_moves(c, pieces_considered, false);
    else
        for (Point p : m_bd.get_attach_points(c))
            if (! is_forbidden[p])
            {
                auto adj_status = m_bd.get_adj_status(p, c);
                for (Piece piece : pieces_considered)
                    for (Move mv : get_moves(c, piece, p, adj_status))
                        if (! marker[mv]
                            && check_move_without_gamma(is_forbidden, mv))
                        {
                            marker.set(mv);
                            moves.push_back(mv);
                        }
                m_moves_added_at[c].set(p);
            }
    m_is_move_list_initialized[c] = true;
    m_new_moves[c].clear();
    if (moves.empty() && ! m_force_consider_all_pieces)
    {
        m_force_consider_all_pieces = true;
        init_moves_without_gamma(c);
    }
}

void State::play_expanded_child(Move mv)
{
    if (log_simulations)
        log("Playing expanded child");
    if (! mv.is_pass())
        play_playout(mv);
    else
    {
        m_bd.play_pass();
        ++m_nu_passes;
        // Don't try to handle pass moves: a pass move either breaks symmetry
        // or both players have passed and it's the end of the game and we need
        // symmetry detection only as a heuristic ((playouts and move value
        // initialization)
        m_is_symmetry_broken = true;
        if (log_simulations)
            log() << m_bd;
    }
}

void State::play_playout(Move mv)
{
    LIBBOARDGAME_ASSERT(m_bd.is_legal_nonpass(mv));
    m_new_moves[m_bd.get_to_play()].push_back(mv);
    m_bd.play_nonpass(mv);
    m_nu_passes = 0;
    if (! m_is_symmetry_broken)
        update_symmetry_broken(mv);
    if (log_simulations)
        log() << m_bd;
}

void State::start_search()
{
    auto& bd = *m_shared_const.board;
    m_bd.copy_from(bd);
    m_bd.set_to_play(m_shared_const.to_play);
    m_bd.take_snapshot();
    m_bc = &m_bd.get_board_const();
    m_nu_colors = bd.get_nu_colors();
    m_move_info_array = m_bc->get_move_info_array();
    m_move_info_ext_array = m_bc->get_move_info_ext_array();
    m_nu_moves_initial = bd.get_nu_moves();
    m_check_terminate_early =
        (m_nu_moves_initial < 10u * m_nu_colors
         && m_bd.get_nu_players() == 2);
    m_nu_simulations = 0;
    m_nu_playout_moves = 0;
    m_nu_last_good_reply_moves = 0;
    auto variant = bd.get_variant();
    m_check_symmetric_draw =
        ((variant == Variant::duo || variant == Variant::junior
          || variant == Variant::trigon_2)
         && ! ((m_shared_const.to_play == Color(1)
                || m_shared_const.to_play == Color(3))
               && m_shared_const.avoid_symmetric_draw)
         && ! check_symmetry_broken(bd, m_shared_const.symmetric_points));
    if (variant == Variant::trigon_2)
        m_symmetry_min_nu_pieces = 5;
    else
        m_symmetry_min_nu_pieces = 3; // Only used in Duo

    m_prior_knowledge.start_search(bd);
    for (ColorIterator i(m_nu_colors); i; ++i)
        m_stat_score[*i].clear();
    m_stat_len.clear();

    // Init precomputed gamma values
    double gamma_size_factor = 1;
    double gamma_nu_attach_factor = 1;
    switch (bd.get_board_type())
    {
    case BoardType::classic:
        gamma_size_factor = 5;
        break;
    case BoardType::duo:
        gamma_size_factor = 3;
        gamma_nu_attach_factor = 1.8;
        break;
    case BoardType::trigon:
    case BoardType::trigon_3:
        gamma_size_factor = 5;
        break;
    }
    for (Piece::IntType i = 0; i < m_bc->get_nu_pieces(); ++i)
    {
        Piece piece(i);
        auto piece_size = m_bc->get_piece_info(piece).get_size();
        LIBBOARDGAME_ASSERT(piece_size > 0);
        auto piece_nu_attach = m_bc->get_nu_attach_points(piece);
        LIBBOARDGAME_ASSERT(piece_nu_attach > 0);
        m_gamma_piece[piece] =
            pow(gamma_size_factor, piece_size - 1)
            * pow(gamma_nu_attach_factor, piece_nu_attach - 1);
    }
    for (unsigned i = 0; i < PieceInfo::max_size + 1; ++i)
        m_gamma_nu_attach[i] = pow(1e10, i);
}

void State::start_simulation(size_t n)
{
    if (log_simulations)
        log() << "=========================================================\n"
              << "Simulation " << n << "\n"
              << "=========================================================\n";
    ++m_nu_simulations;
    m_bd.restore_snapshot();
    m_force_consider_all_pieces = false;
    for (ColorIterator i(m_nu_colors); i; ++i)
    {
        m_has_moves[*i] = true;
        m_is_move_list_initialized[*i] = false;
        m_new_moves[*i].clear();
        m_moves_added_at[*i].clear();
    }
    m_nu_passes = 0;
    for (unsigned i = m_bd.get_nu_moves(); i > 0; --i)
    // Should m_nu_passes be initialized without assuming alternating
    // colors in the board's move history?
    {
        if (! m_bd.get_move(i - 1).move.is_pass())
            break;
        ++m_nu_passes;
    }
}

void State::update_moves(Color c)
{
    m_local_value.init(m_bd);
    m_total_gamma = 0;
    auto& marker = m_marker[c];

    // Find old moves that are still legal
    PieceMap<bool> is_piece_left(false);
    for (Piece piece : m_bd.get_pieces_left(c))
        is_piece_left[piece] = true;
    auto& is_forbidden = m_bd.is_forbidden(c);
    auto& moves = m_moves[c];
    auto old_size = moves.size();
    moves.clear();
    double gamma;
    for (unsigned i = 0; i < old_size; ++i)
    {
        LIBBOARDGAME_ASSERT(i >= moves.size());
        Move mv = moves.get_unchecked(i);
        auto& info = get_move_info(mv);
        if (is_piece_left[info.get_piece()]
            && check_move(is_forbidden, info, gamma))
            add_move(moves, mv, gamma);
        else
            marker.clear(mv);
    }

    // Find new legal moves because of new pieces played by this color
    Board::PiecesLeftList pieces_considered;
    for (Piece piece : m_bd.get_pieces_left(c))
        if ((*m_is_piece_considered[c])[piece])
            pieces_considered.push_back(piece);
    for (Move mv : m_new_moves[c])
    {
        auto& info_ext = get_move_info_ext(mv);
        auto i = info_ext.begin_attach();
        auto end = info_ext.end_attach();
        do
            if (! is_forbidden[*i] && ! m_moves_added_at[c][*i])
                add_moves(*i, c, pieces_considered);
        while (++i != end);
    }
    m_new_moves[c].clear();

    // Generate moves for pieces not considered in the last position
    auto& is_piece_considered = *m_is_piece_considered[c];
    if (&is_piece_considered != &m_shared_const.is_piece_considered_all)
    {
        if (moves.empty())
            m_force_consider_all_pieces = true;
        auto& is_piece_considered_new = get_pieces_considered();
        if (&is_piece_considered !=  &is_piece_considered_new)
        {
            pieces_considered.clear();
            for (Piece piece : m_bd.get_pieces_left(c))
                if (! is_piece_considered[piece]
                    && is_piece_considered_new[piece])
                    pieces_considered.push_back(piece);
            for (Point p : m_bd.get_attach_points(c))
                if (! is_forbidden[p])
                {
                    auto adj_status = m_bd.get_adj_status(p, c);
                    for (Piece piece : pieces_considered)
                        add_moves(p, c, piece, adj_status);
                }
            m_is_piece_considered[c] = &is_piece_considered_new;
        }
    }
}

void State::update_symmetry_broken(Move mv)
{
    LIBBOARDGAME_ASSERT(! mv.is_pass());
    Color to_play = m_bd.get_to_play();
    Color second_color = m_bd.get_second_color(to_play);
    auto& info = get_move_info(mv);
    auto i = info.begin();
    auto end = info.end();
    if (to_play == Color(0) || to_play == Color(2))
    {
        // First player to play: Check that all symmetric points of the last
        // move of the second player are occupied by the first player
        do
        {
            Point symm_p = m_shared_const.symmetric_points[*i];
            if (m_bd.get_point_state(symm_p) != second_color)
            {
                m_is_symmetry_broken = true;
                return;
            }
        }
        while (++i != end);
    }
    else
    {
        // Second player to play: Check that all symmetric points of the last
        // move of the first player are empty (i.e. the second player can play
        // there to preserve the symmetry)
        do
        {
            Point symm_p = m_shared_const.symmetric_points[*i];
            if (! m_bd.get_point_state(symm_p).is_empty())
            {
                m_is_symmetry_broken = true;
                return;
            }
        }
        while (++i != end);
    }
}

void State::write_info(ostream& out) const
{
    if (m_nu_playout_moves > 0)
    {
        FmtSaver saver(log());
        out << "LGR: " << fixed << setprecision(1)
            << (100.0 * static_cast<double>(m_nu_last_good_reply_moves)
                / static_cast<double>(m_nu_playout_moves))
            << "%, ";
    }
    if (m_bd.get_nu_players() == 2)
    {
        out << "Sco: ";
        m_stat_score[Color(0)].write(out, true, 1);
    }
    out << '\n';
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts
