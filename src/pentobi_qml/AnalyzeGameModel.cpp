//-----------------------------------------------------------------------------
/** @file pentobi_qml/AnalyzeGameModel.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#include "AnalyzeGameModel.h"

#include <QtConcurrent>
#include "GameModel.h"
#include "PlayerModel.h"
#include "libboardgame_sgf/SgfUtil.h"
#include "libboardgame_util/Abort.h"

using libboardgame_sgf::util::is_main_variation;
using libboardgame_sgf::util::find_root;
using libboardgame_util::clear_abort;
using libboardgame_util::ArrayList;
using libboardgame_util::set_abort;
using libpentobi_base::ColorMove;

//-----------------------------------------------------------------------------

AnalyzeGameElement::AnalyzeGameElement(QObject* parent, int moveColor,
                                       double value)
    : QObject(parent),
      m_moveColor(moveColor),
      m_value(value)
{
}

//-----------------------------------------------------------------------------

AnalyzeGameModel::AnalyzeGameModel(QObject* parent)
    : QObject(parent)
{
    connect(&m_watcher, SIGNAL(finished()), SLOT(runFinished()));
}

AnalyzeGameModel::~AnalyzeGameModel()
{
    cancel();
}

void AnalyzeGameModel::asyncRun(const Game* game, Search* search)
{
    size_t nuSimulations = 3000;
    auto progressCallback =
        [&](unsigned movesAnalyzed, unsigned totalMoves)
        {
            int progress = 100 * movesAnalyzed / max(totalMoves, 1u);
            // Use invokeMethod() because callback runs in different thread
            QMetaObject::invokeMethod(this, "setProgress",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, progress));
            QMetaObject::invokeMethod(this, "updateElements",
                                      Qt::BlockingQueuedConnection);
        };
    m_analyzeGame.run(*game, *search, nuSimulations, progressCallback);
}

void AnalyzeGameModel::cancel()
{
    if (! m_isRunning)
        return;
    set_abort();
    m_watcher.waitForFinished();
    setIsRunning(false);
}

void AnalyzeGameModel::clear()
{
    if (! m_elements.empty())
    {
        m_markMoveNumber = -1;
        m_elements.clear();
        emit elementsChanged();
    }
}

QQmlListProperty<AnalyzeGameElement> AnalyzeGameModel::elements()
{
    return QQmlListProperty<AnalyzeGameElement>(this, m_elements);
}

void AnalyzeGameModel::gotoMove(GameModel* gameModel, int moveNumber)
{
    if (moveNumber < 0
            || moveNumber >= static_cast<int>(m_analyzeGame.get_nu_moves()))
        return;
    auto& game = gameModel->getGame();
    if (game.get_variant() != m_analyzeGame.get_variant())
        return;
    auto& tree = game.get_tree();
    auto node = &tree.get_root();
    if (tree.has_move_ignore_invalid(*node))
    {
        // Move in root node not supported.
        setMarkMoveNumber(-1);
        return;
    }
    for (int i = 0; i < moveNumber; ++i)
    {
        auto mv = m_analyzeGame.get_move(i);
        bool found = false;
        for (auto& i : node->get_children())
            if (tree.get_move(i) == mv)
            {
                found = true;
                node = &i;
                break;
            }
        if (! found)
        {
            setMarkMoveNumber(-1);
            return;
        }
    }
    gameModel->gotoNode(*node);
    setMarkMoveNumber(moveNumber);
}

void AnalyzeGameModel::markCurrentMove(GameModel* gameModel)
{
    auto& game = gameModel->getGame();
    auto& node = game.get_current();
    int moveNumber = -1;
    if (is_main_variation(node))
    {
        ArrayList<ColorMove, Board::max_game_moves> moves;
        auto& tree = game.get_tree();
        auto current = &find_root(node);
        while (current)
        {
            auto mv = tree.get_move(*current);
            if (! mv.is_null() && moves.size() < Board::max_game_moves)
                moves.push_back(mv);
            if (current == &node)
                break;
            current = current->get_first_child_or_null();
        }
        if (moves.size() <= m_analyzeGame.get_nu_moves())
        {
            for (unsigned i = 0; i < moves.size(); ++i)
                if (moves[i] != m_analyzeGame.get_move(i))
                    return;
            moveNumber = moves.size();
        }
    }
    setMarkMoveNumber(moveNumber);
}

void AnalyzeGameModel::runFinished()
{
    setIsRunning(false);
    updateElements();
}

void AnalyzeGameModel::setIsRunning(bool isRunning)
{
    if (m_isRunning == isRunning)
        return;
    m_isRunning = isRunning;
    emit isRunningChanged();
}

void AnalyzeGameModel::setMarkMoveNumber(int markMoveNumber)
{
    if (m_markMoveNumber == markMoveNumber)
        return;
    m_markMoveNumber = markMoveNumber;
    emit markMoveNumberChanged();
}

void AnalyzeGameModel::setProgress(int progress)
{
    if (m_progress == progress)
        return;
    m_progress = progress;
    emit progressChanged();
}

void AnalyzeGameModel::start(GameModel* gameModel, PlayerModel* playerModel)
{
    cancel();
    clear_abort();
    auto future = QtConcurrent::run(this, &AnalyzeGameModel::asyncRun,
                                    &gameModel->getGame(),
                                    &playerModel->getSearch());
    m_watcher.setFuture(future);
    setIsRunning(true);
}

void AnalyzeGameModel::updateElements()
{
    m_elements.clear();
    for (unsigned i = 0; i < m_analyzeGame.get_nu_moves(); ++i)
    {
        auto moveColor = m_analyzeGame.get_move(i).color.to_int();
        // Values of search are supposed to be win/loss probabilities but can
        // be slighly outside [0..1] (see libpentobi_mcts::State).
        auto value = max(0., min(1., m_analyzeGame.get_value(i)));
        m_elements.append(new AnalyzeGameElement(this, moveColor, value));
    }
    emit elementsChanged();
}

//-----------------------------------------------------------------------------