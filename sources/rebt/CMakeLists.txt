# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

# Low voltage circuit calculations (Spanish REBT). Plain C++17, no Qt:
# it builds and is tested without the rest of QElectroTech.
add_library(qetrebt STATIC
	rebttypes.h
	rebtrules.h
	rebtrules.cpp
	rebtcalc.h
	rebtcalc.cpp
	rebtampacity.h
	rebtampacity.cpp
	rebtprotection.h
	rebtprotection.cpp
	rebtshortcircuit.h
	rebtshortcircuit.cpp
	rebtdwelling.h
	rebtdwelling.cpp
	rebtindirect.h
	rebtindirect.cpp
)
target_include_directories(qetrebt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(qetrebt PUBLIC cxx_std_17)
