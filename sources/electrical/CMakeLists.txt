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

# The electrical model of a project (boards, circuits) and its XML.
# Qt Core and Xml only: no widget, testable without the application.
add_library(qetelectrical STATIC
	electricalmodel.h
	electricalmodel.cpp
	electricalevaluation.h
	electricalevaluation.cpp
	ampacityfile.h
	ampacityfile.cpp
	devicecatalogue.h
	devicecatalogue.cpp
)
target_include_directories(qetelectrical PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(qetelectrical PUBLIC qetrebt Qt::Core Qt::Xml)
target_compile_features(qetelectrical PUBLIC cxx_std_17)
