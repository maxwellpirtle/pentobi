//-----------------------------------------------------------------------------
/** @file libboardgame_sgf/Writer.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Writer.h"

#include <sstream>

namespace libboardgame_sgf {

//-----------------------------------------------------------------------------

Writer::Writer(ostream& out)
    : m_out(out)
{ }

void Writer::begin_node()
{
    m_is_first_prop = true;
    write_indent();
    m_out << ';';
}

void Writer::begin_tree()
{
    write_indent();
    m_out << '(';
    // Don't indent the first level
    if (m_level > 0)
        m_current_indent += m_indent;
    ++m_level;
    if (m_indent >= 0)
        m_out << '\n';
}

void Writer::end_node()
{
    if (! m_one_prop_per_line && m_indent >= 0)
        m_out << '\n';
}

void Writer::end_tree()
{
    --m_level;
    if (m_level > 0)
        m_current_indent -= m_indent;
    write_indent();
    m_out << ')';
    if (m_indent >= 0)
        m_out << '\n';
}

string Writer::get_escaped(const string& s)
{
    ostringstream buffer;
    for (char c : s)
    {
        if (c == ']' || c == '\\')
            buffer << '\\' << c;
        else if (c == '\t' || c == '\f' || c == '\v')
            // Replace whitespace as required by the SGF standard.
            buffer << ' ';
        else
            buffer << c;
    }
    return buffer.str();
}

void Writer::write_indent()
{
    if (m_indent >= 0)
        for (int i = 0; i < m_current_indent; ++i)
            m_out << ' ';
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_sgf
