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

#include <core_segments.h>
#include <cstring>
#include <boost/foreach.hpp>

namespace CoRipper
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Segment

Segment::~Segment()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Builder

bool Builder::getResult(Builder::data_t& dst_)
{
	GElf_Ehdr h;
	if (!m_reader->getEhdr(h))
		return false;
	h.e_phnum = m_segments.size();
	h.e_phoff = sizeof(h);
	dst_.first = Header(h);
	dst_.second = m_segments;
	return true;
}

bool Builder::readNote()
{
	if (m_note)
		return true;

	GElf_Phdr notePhdr;
	if (!m_reader->findNotePhdr(notePhdr))
		return false;

	Elf_Data* noteData;
	if(NULL == (noteData = m_reader->getNoteData(notePhdr)))
		return false;

	m_note.reset(new Note(notePhdr, noteData));
	m_segments.push_back(m_note);
	return true;
}

bool Builder::readDynamic()
{
	if (m_dynamic)
		return true;

	if (!m_note && !readNote())
		return false;

	Elf_Data* auxvData = m_reader->getAuxvData(m_note->getHeader(), m_note->getData());
	Elf_Data* execPhdrData = m_reader->getExecPhdrData(auxvData);

	if (NULL == auxvData || NULL == execPhdrData)
		return false;

	GElf_Phdr dynPhdr;
	if (!m_reader->findExecPhdrByType(execPhdrData, PT_DYNAMIC, dynPhdr))
		return false;

	// If an executable is a shared object (ELF type == DYN) then DYNAMIC segment virtual address has a bias.
	// We can check this by compare PT_PHDR program header's virtual address with AUXV phoff value.
	// For static executable PT_PHDR virtual addsters is equal AUXV phoff value.
	{
		GElf_Phdr phdr;
		GElf_auxv_t phoff;

		if (!m_reader->findExecPhdrByType(execPhdrData, PT_PHDR, phdr))
			return false;

		if (!m_reader->findAuxvByType(auxvData, AT_PHDR, phoff))
			return false;

		if ( phoff.a_un.a_val !=  phdr.p_vaddr) {
			if (!m_reader->findPhdrByVaddr(phoff.a_un.a_val, phdr))
				return false;

			dynPhdr.p_vaddr += phdr.p_vaddr;
		}
	}

	Elf_Data* dynData;
	if (NULL == (dynData = m_reader->getDynData(dynPhdr)))
		return false;

	m_dynamic.reset(new Dynamic(dynPhdr, dynData));
	m_segments.push_back(m_dynamic);
	return true;
}

bool Builder::readRDebug()
{
	if (m_rdebug)
		return true;

	if (!m_dynamic && !readDynamic())
		return false;

	GElf_Dyn debugDyn;
	if (!m_reader->findDynByTag(m_dynamic->getData(), DT_DEBUG, debugDyn))
		return false;

	rdebug_t rdebug;
	if (!m_reader->getRDebug(debugDyn, rdebug))
		return false;

	m_rdebug.reset(new RDebug(debugDyn, rdebug));
	m_segments.push_back(m_rdebug);
	return true;
}

// Read all stack segments
bool Builder::readStacks()
{
	size_t pos = 0;
	prstatus_t prs;
	Stack::list_t stacks;

	if (!m_note && !readNote())
		return false;

	// iterate through all prstatus notes and save stack data.
	while((pos = m_reader->getNextPrStatus(m_note->getData(), pos, prs)) > 0) {
		GElf_Addr vaddr;
		Elf_Data* stackData = m_reader->getStackData(prs, vaddr);
		if (NULL == stackData)
			return false;

		stacks.push_back(Stack::ptr_t(new Stack(vaddr, stackData)));
	}
	m_segments.insert(m_segments.end(), stacks.begin(), stacks.end());
	return true;
}

// Read all linkmap structures and linkmap name strings
bool Builder::readLinkmaps()
{
	if (!m_rdebug && !readRDebug())
		return false;

	Segment::list_t linkmapList;
	GElf_Addr vaddr = (GElf_Addr) m_rdebug->getData().r_map;
	while (vaddr != 0) {
		linkmap_t lmap;
		if (!m_reader->getLinkmap(vaddr, lmap))
			return false;

		Linkmap::ptr_t linkmapPtr(new Linkmap(vaddr, lmap));
		linkmapList.push_back(linkmapPtr);

		// get link name
		vaddr = linkmapPtr->getName();
		std::vector<char> buf;
		if (!m_reader->getString(vaddr, buf))
			return false;

		if (NULL != strstr(&buf[0], "/libpthread.so"))
			strrchr(&buf[0], '/')[4] = 'a';
		linkmapList.push_back(String::ptr_t(new String(vaddr, buf)));

		// next linkmap address
		vaddr = linkmapPtr->getNext();
	}
	m_segments.insert(m_segments.end(), linkmapList.begin(), linkmapList.end());
	return true;
}

std::ostream& operator<<(std::ostream& o_, const Builder::data_t& d_)
{
	off_t offset = d_.first.getOffset();

	// write ELF header
	if(!o_.write(d_.first.getData(), d_.first.getSize()))
		return o_;

	// write all program headers
	BOOST_FOREACH(const Segment::ptr_t& s, d_.second)
	{
		Segment::Header h = s->getHeader(offset);
		if (!o_.write(reinterpret_cast<const char*>(&h), d_.first.getHeaderSize()))
			return o_;
		offset += s->getSize();
	}

	// write segments data
	BOOST_FOREACH(const Segment::ptr_t& s, d_.second)
	{
		if (!o_.write(s->getBuffer(), s->getSize()))
			return o_;
	}

	return o_;
}

} //namespace CoRipper
