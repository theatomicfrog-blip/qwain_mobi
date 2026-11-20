// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef PREFIX_PARSER_H
#define PREFIX_PARSER_H

#include <QString>
#include <QRegularExpression>

inline QString guessIsoFromCallsign(QString call) {
    call = call.toUpper();
    // Strip modifiers
    int slash = call.indexOf('/');
    if (slash != -1) {
        // e.g. IL7/IZ5IUY -> IL7 (prefix usually indicates country operated from)
        // e.g. GB3MHZ/B -> GB3MHZ (/B is beacon)
        QString before = call.left(slash);
        QString after = call.mid(slash + 1);
        
        // If after is length >= 3 and has no digits, it's likely a callsign (e.g. W1AW/KL7) - wait no KL7 has digits.
        // Actually, usually the SHORTER one without numbers is the country prefix (like F/W1AW)
        // Or if it's 1-2 chars, it's a modifier (/P, /M, /MM, /B)
        
        if (after == "B" || after == "P" || after == "M" || after == "MM" || after == "AM" || after == "QRP") {
            call = before;
        } else if (before.length() < after.length() && before.length() <= 3) {
            call = before; // F/W1AW -> F
        } else if (after.length() <= 3) {
            call = after; // W1AW/F -> F
        } else {
            // Assume before 
            call = before;
        }
    }
    
    if (call.startsWith("K") || call.startsWith("W") || call.startsWith("N") || 
        (call.startsWith("A") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'L')) return "us";
        
    if (call.startsWith("V") && call.length() >= 2 && (call[1] == 'A' || call[1] == 'E' || call[1] == 'O' || call[1] == 'Y' || call[1] == 'X')) return "ca";
    
    if (call.startsWith("G") || call.startsWith("M") || call.startsWith("2")) return "gb";
    
    if (call.startsWith("J") && call.length() >= 2 && call[1] != 'T' && call[1] != 'X') return "jp";
    if (call.startsWith("7K") || call.startsWith("7L") || call.startsWith("7M") || call.startsWith("7N") || call.startsWith("8J")) return "jp";
    
    if (call.startsWith("I")) return "it";
    if (call.startsWith("D") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'R') return "de";
    if (call.startsWith("R") || (call.startsWith("U") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'I')) return "ru";
    
    if (call.startsWith("E") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'H') return "es";
    if (call.startsWith("C") && call.length() >= 2 && call[1] == 'T') return "pt";
    if (call.startsWith("C") && call.length() >= 2 && call[1] == 'U') return "pt"; // Azores
    
    if (call.startsWith("P") && call.length() >= 2 && call[1] >= 'P' && call[1] <= 'Y') return "br";
    if (call.startsWith("L") && call.length() >= 2 && call[1] >= 'U' && call[1] <= 'W') return "ar";
    if (call.startsWith("H") && call.length() >= 2 && call[1] == 'L') return "kr";
    
    if (call.startsWith("B")) return "cn";
    if (call.startsWith("V") && call.length() >= 2 && call[1] == 'K') return "au";
    if (call.startsWith("Z") && call.length() >= 2 && call[1] == 'L') return "nz";
    if (call.startsWith("Z") && call.length() >= 2 && call[1] == 'S') return "za";
    
    if (call.startsWith("F")) return "fr";
    if (call.startsWith("H") && call.length() >= 2 && (call[1] == 'A' || call[1] == 'G')) return "hu";
    if (call.startsWith("P") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'I') return "nl";
    if (call.startsWith("O") && call.length() >= 2 && call[1] >= 'N' && call[1] <= 'T') return "be";
    if (call.startsWith("H") && call.length() >= 2 && call[1] >= 'J' && call[1] <= 'K') return "co";
    if (call.startsWith("9") && call.length() >= 2 && call[1] == 'K') return "kw";
    if (call.startsWith("9") && call.length() >= 2 && call[1] == 'A') return "hr";
    if (call.startsWith("4") && call.length() >= 2 && call[1] == 'X') return "il";
    if (call.startsWith("H") && call.length() >= 2 && call[1] == 'B') return "ch";
    
    if (call.startsWith("S") && call.length() >= 2 && call[1] >= 'M' && call[1] <= 'Q') return "se";
    if (call.startsWith("L") && call.length() >= 2 && call[1] >= 'A' && call[1] <= 'N') return "no";
    if (call.startsWith("O") && call.length() >= 2 && call[1] >= 'U' && call[1] <= 'Z') return "dk";
    if (call.startsWith("O") && call.length() >= 2 && call[1] >= 'F' && call[1] <= 'J') return "fi";
    if (call.startsWith("S") && call.length() >= 2 && call[1] >= 'N' && call[1] <= 'R') return "pl";
    if (call.startsWith("U") && call.length() >= 2 && call[1] >= 'R' && call[1] <= 'Z') return "ua";
    if (call.startsWith("E") && call.length() >= 2 && call[1] >= 'M' && call[1] <= 'O') return "ua";
    if (call.startsWith("Y") && call.length() >= 2 && call[1] >= 'O' && call[1] <= 'R') return "ro";
    if (call.startsWith("V") && call.length() >= 2 && call[1] == 'U') return "in";
    if (call.startsWith("Y") && call.length() >= 2 && call[1] >= 'B' && call[1] <= 'H') return "id";
    if (call.startsWith("D") && call.length() >= 2 && call[1] >= 'U' && call[1] <= 'Z') return "ph";
    if (call.startsWith("E") && call.length() >= 2 && call[1] == 'I') return "ie";
    
    return "";
}

#endif
