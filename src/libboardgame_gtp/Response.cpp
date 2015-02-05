//-----------------------------------------------------------------------------
/** @file libboardgame_gtp/Response.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Response.h"

#include <iostream>

namespace libboardgame_gtp {

using namespace std;

//-----------------------------------------------------------------------------

ostringstream Response::s_dummy;

Response::~Response()
{
}

void Response::write(ostream& out, string& buffer) const
{
    buffer = m_stream.str();
    bool was_newline = false;
    for (auto c : buffer)
    {
        bool is_newline = (c == '\n');
        if (is_newline && was_newline)
            out << ' ';
        out << c;
        was_newline = is_newline;
    }
    if (! was_newline)
        out << '\n';
    out << '\n';
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_gtp
