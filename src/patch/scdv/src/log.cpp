/*
 * Copyright (c) 2020 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <e32debug.h>
#include <e32std.h>
#include <scdv/log.h>
#include <scdv/panic.h>

namespace Scdv {
    class TDesOverflowHandler : public TDes8Overflow {
        virtual void Overflow(TDes8 &) {
            Panic(Scdv::EPanicLogFailure);
        }
    };

    void Log(const char *aFormat, ...) {
        TPtrC8 baseFormat(reinterpret_cast<const TUint8 *>(aFormat));
        HBufC8 *newString = HBufC8::NewL(baseFormat.Length() * 2);

        VA_LIST list;
        va_start(list, aFormat);

        TDesOverflowHandler handler;

        TPtr8 stringDes = newString->Des();
        stringDes.AppendFormatList(baseFormat, list, &handler);

        RDebug::Printf("%S", &stringDes);
        User::Free(newString);
    }
};
