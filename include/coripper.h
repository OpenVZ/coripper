/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __CORIPPER_H__
#define __CORIPPER_H__

#include <core_segments.h>

namespace CoRipper
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Core

struct Core
{
	bool read(const char*);

private:
	friend std::ostream& operator<<(std::ostream& sout, const Core& c);

	void clear();

	Builder::data_t m_data;
	Reader::ptr_t m_reader;
};

std::ostream& operator<<(std::ostream& o_, const Core& c_);

} //namespace CoRipper

#endif //__CORIPPER_H__
