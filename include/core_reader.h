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

#ifndef __CORE_READER_H__
#define __CORE_READER_H__

#include <vector>
#include <libelf.h>
#include <gelf.h>
#include <link.h>
#include <sys/procfs.h>
#include <boost/shared_ptr.hpp>

namespace CoRipper
{

typedef struct r_debug rdebug_t;
typedef struct link_map linkmap_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Reader

struct Reader
{
	typedef boost::shared_ptr<Reader> ptr_t;

	~Reader();

	GElf_Ehdr* getEhdr(GElf_Ehdr& dst_);

	GElf_Phdr* findNotePhdr(GElf_Phdr& dst_);
	Elf_Data* getNoteData(const GElf_Phdr& phdr_);

	Elf_Data* getAuxvData(const GElf_Phdr& phdr_, Elf_Data* noteData_);
	GElf_auxv_t* findAuxvByType(Elf_Data *auxvData_, unsigned type_, GElf_auxv_t& dst_);

	GElf_Phdr* findPhdrByVaddr(GElf_Addr vaddr_, GElf_Phdr& dst_);
	off_t findOffsetByVaddr(GElf_Addr vaddr_);

	Elf_Data* getDynData(GElf_Phdr& phdr_);
	GElf_Dyn* findDynByTag(Elf_Data* data_, GElf_Sxword tag_, GElf_Dyn& dst_);

	Elf_Data* getExecPhdrData(Elf_Data* auxvData_);
	GElf_Phdr* findExecPhdrByType(Elf_Data *phdrData_, unsigned type_, GElf_Phdr& dst_);

	bool getString(GElf_Addr vaddr_, std::vector<char>& buf_);
	rdebug_t* getRDebug(GElf_Dyn& dyn_, rdebug_t& rdebug_);
	linkmap_t* getLinkmap(GElf_Addr vaddr_, linkmap_t& lmap_);

	size_t getNextPrStatus(Elf_Data* notes_, size_t pos_, prstatus_t& prs_);
	Elf_Data* getStackData(const prstatus_t& prs_, GElf_Addr& vaddr_);

	static ptr_t openCoreFile(const char* fname_);

private:
	Reader(int fd_, Elf* e_)
	: m_fd(fd_), m_core(e_)
	{
	}

	ssize_t readCoreData(void* buff_, size_t size_, off_t offset_);
	GElf_Addr getStack(const prstatus_t& prs_);

	int m_fd;
	Elf* m_core;
};

} //namespace CoRipper

#endif //__CORE_READER_H__

