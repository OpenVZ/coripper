/*
 * Copyright (c) 2015-2017 Parallels IP Holdings GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <iostream>
#include <coripper.h>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Usage: "
			<< argv[0]
			<< " <source path> [> <dest path>]"
			<< std::endl;
		return -1;
	}

	CoRipper::Core core;

	if (!core.read(argv[1])) {
		std::cerr << "Coredump file read failed." << std::endl;
		return -1;
	}

	if (!(std::cout << core)) {
		std::cerr << "Failed to write output." << std::endl;
		return -1;
	}

	return 0;
}
