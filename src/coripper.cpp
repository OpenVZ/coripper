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

#include <coripper.h>
#include <iostream>

namespace CoRipper
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Core

bool Core::read(const char* fname)
{
	clear();

	if(!(m_reader = Reader::openCoreFile(fname)))
	{
		std::cerr << "ERROR: Unable to open file " << fname << std::endl;
		return false;
	}

	Builder b(*m_reader);
	if (!b.readNote())
	{
		std::cerr << "ERROR: Unable to read core notes" << std::endl;
		return false;
	}
	if (!b.readDynamic())
	{
		std::cerr << "ERROR: Unable to read dynamic section" << std::endl;
		return false;
	}
	if (!b.readRDebug())
	{
		std::cerr << "ERROR: Unable to read rdebug structure" << std::endl;
		return false;
	}
	if (!b.readLinkmaps())
	{
		std::cerr << "ERROR: Unable to read linkmap" << std::endl;
		return false;
	}
	if (!b.readStacks())
	{
		std::cerr << "ERROR: Unable to read stacks" << std::endl;
		return false;
	}
	if (!b.getResult(m_data))
	{
		std::cerr << "ERROR: Unable to read elf header" << std::endl;
		return false;
	}
	return true;
}

void Core::clear()
{
	m_data.second.clear();
}

std::ostream& operator<<(std::ostream& s_, const Core& c_)
{
	return s_ << c_.m_data;
}

} //namespace CoRipper
