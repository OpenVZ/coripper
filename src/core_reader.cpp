/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <core_reader.h>

enum {
	MIN_PATH_BUFF_SIZE = 16,
	MAX_PATH_BUFF_SIZE = 4096
};

namespace CoRipper
{

typedef struct user_regs_struct regs_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Reader

// Reader factory method - opens file and creates ELF descriptor
Reader::ptr_t Reader::openCoreFile(const char* fname_)
{
	int fd;
	Elf* core;

	if ((fd = open(fname_, O_RDONLY, 0)) < 0) {
		std::cerr << "ERROR: Failed to open file: "
			<< fname_
			<< ". Reason:"
			<< strerror(errno)
			<< std::endl;
		return ptr_t();
	}

	if (elf_version(EV_CURRENT) == EV_NONE
		|| NULL == (core = elf_begin(fd, ELF_C_READ, NULL)))
	{
		std::cerr << "ERROR: Failed to start work with elf"
			<< std::endl;

		close(fd);
		return ptr_t();
	}

	return ptr_t(new Reader(fd, core));
}

Reader::~Reader()
{
	elf_end(m_core);
	close(m_fd);
}

// Read ELF headr
GElf_Ehdr* Reader::getEhdr(GElf_Ehdr& dst_)
{
	return gelf_getehdr(m_core, &dst_);
}

// Locate and return NOTE program header
GElf_Phdr* Reader::findNotePhdr(GElf_Phdr& dst_)
{
	size_t phnum, ndx;

	if (0 != elf_getphdrnum(m_core, &phnum))
		return NULL;

	for (ndx = 0; ndx < phnum; ndx++) {
		if (gelf_getphdr(m_core, ndx, &dst_) != &dst_)
			return NULL;

		if (dst_.p_type == PT_NOTE)
			return &dst_;
	}

	return NULL;
}

// Read NOTE data
Elf_Data* Reader::getNoteData(const GElf_Phdr& phdr_)
{
	return elf_getdata_rawchunk(m_core, phdr_.p_offset, phdr_.p_filesz, ELF_T_NHDR);
}

// Read AUXV data
Elf_Data* Reader::getAuxvData(const GElf_Phdr& phdr_, Elf_Data* noteData_)
{
	GElf_Nhdr nhdr;
	size_t name_pos, desc_pos;
	size_t pos = 0;

	while ((pos = gelf_getnote(noteData_, pos, &nhdr,
	                           &name_pos, &desc_pos)) > 0)
	{
		if (nhdr.n_type == NT_AUXV)
			return elf_getdata_rawchunk(m_core,
					phdr_.p_offset+ desc_pos,
					nhdr.n_descsz,
					ELF_T_AUXV);
	}
	return NULL;
}

// Locate and return AUXV with given type
GElf_auxv_t* Reader::findAuxvByType(Elf_Data *auxvData_, unsigned type_, GElf_auxv_t& dst_)
{
	size_t ndx = 0;

	while(gelf_getauxv(auxvData_, ndx++, &dst_)) {
		if (dst_.a_type == type_)
			return &dst_;
	}

	return NULL;
}

// Locate and return program header corresponding given virtual address
GElf_Phdr* Reader::findPhdrByVaddr(GElf_Addr vaddr_, GElf_Phdr& dst_)
{
	size_t phnum, ndx;

	if (0 != elf_getphdrnum(m_core, &phnum))
		return NULL;

	for (ndx = 0; ndx < phnum; ndx++) {
		GElf_Phdr phdr;

		if (gelf_getphdr(m_core, ndx, &phdr) != &phdr)
			return NULL;

		if (vaddr_ >= phdr.p_vaddr && vaddr_ < phdr.p_vaddr + phdr.p_filesz) {
			memcpy(&dst_, &phdr, sizeof(GElf_Phdr));
			return &dst_;
		}
	}

	return NULL;
}

// Retrun offset in file corresponding given virtual address
off_t Reader::findOffsetByVaddr(GElf_Addr vaddr_)
{
	GElf_Phdr phdr;

	if (!findPhdrByVaddr(vaddr_, phdr))
		return -1;

	return phdr.p_offset + (vaddr_ - phdr.p_vaddr);
}

// Read .dynamic segment data
Elf_Data* Reader::getDynData(GElf_Phdr& phdr_)
{
	off_t offset = findOffsetByVaddr(phdr_.p_vaddr);
	return elf_getdata_rawchunk(m_core, offset, phdr_.p_filesz, ELF_T_DYN);
}

// Locate and return dynamic unit with given type
GElf_Dyn* Reader::findDynByTag(Elf_Data* data_, GElf_Sxword tag_, GElf_Dyn& dst_)
{
	size_t ndx = 0;
	while(gelf_getdyn(data_, ndx++, &dst_)) {
		if (dst_.d_tag == tag_)
			return &dst_;
	}
	return NULL;
}

// Return program headers data from executable segment
Elf_Data* Reader::getExecPhdrData(Elf_Data* auxvData_)
{
	GElf_auxv_t phnum, phoff, phsize;

	off_t offset;
	size_t size;

	if (!findAuxvByType(auxvData_, AT_PHNUM, phnum))
		return NULL;
	if (!findAuxvByType(auxvData_, AT_PHDR, phoff))
		return NULL;
	if (!findAuxvByType(auxvData_, AT_PHENT, phsize))
		return NULL;

	offset = findOffsetByVaddr(phoff.a_un.a_val);
	size = phnum.a_un.a_val * phsize.a_un.a_val;

	return elf_getdata_rawchunk(m_core, offset, size, ELF_T_PHDR);
}

namespace
{

template <class Src, class Dst>
void copyPhdr(const Src& src_, Dst& dst_)
{
	dst_.p_type = src_.p_type;
	dst_.p_offset = src_.p_offset;
	dst_.p_vaddr = src_.p_vaddr;
	dst_.p_paddr = src_.p_paddr;
	dst_.p_filesz = src_.p_filesz;
	dst_.p_memsz = src_.p_memsz;
	dst_.p_flags = src_.p_flags;
	dst_.p_align = src_.p_align;
}

template <class T>
GElf_Phdr* doFindExecPhderByType(Elf_Data *phdrData_, unsigned type_, GElf_Phdr& dst_)
{
	T* phdrArray = (T*) phdrData_->d_buf;
	size_t phnum = phdrData_->d_size / sizeof(T);
	for (size_t ndx = 0; ndx < phnum; ndx++) {
		if (phdrArray[ndx].p_type == type_) {
			copyPhdr(phdrArray[ndx], dst_);
			return &dst_;
		}
	}
	return NULL;
}

} // namespace

// Locate executable's program header in given data with given type 
GElf_Phdr* Reader::findExecPhdrByType(Elf_Data *phdrData_, unsigned type_, GElf_Phdr& dst_)
{
	char* id = elf_getident(m_core, NULL);

	if (id && id[EI_CLASS] == ELFCLASS32)
		return doFindExecPhderByType<Elf32_Phdr>(phdrData_, type_, dst_);
	else
		return doFindExecPhderByType<Elf64_Phdr>(phdrData_, type_, dst_);
}

// Read C string
bool Reader::getString(GElf_Addr vaddr_, std::vector<char>& buf_)
{
	off_t offset = findOffsetByVaddr(vaddr_);
	size_t len;
	if (offset < 0) {
		buf_.resize(1);
		buf_[0] = '\0';
		return true;
	}

	buf_.resize(MAX_PATH_BUFF_SIZE);
	ssize_t size = 0;
	if ((size = readCoreData(&buf_[0], buf_.size(), offset)) > 0
		&& (len = strnlen(&buf_[0], size)) != (size_t)size)
	{
		buf_.resize(len + 1);
	}
	else
		buf_.resize(0);

	return buf_.size() > 0;
}

// Read rdebug structure
rdebug_t* Reader::getRDebug(GElf_Dyn& dyn_, rdebug_t& rdebug_)
{
	off_t offset;

	if ((offset = findOffsetByVaddr(dyn_.d_un.d_ptr)) < 0)
		return NULL;

	if (readCoreData(&rdebug_, sizeof(rdebug_t), offset) == sizeof(rdebug_t))
		return &rdebug_;

	return NULL;
}

// Read linkmap structure
linkmap_t* Reader::getLinkmap(GElf_Addr vaddr_, linkmap_t& lmap_)
{
	off_t offset = findOffsetByVaddr(vaddr_);

	if (offset < 0)
		return NULL;

	if (readCoreData(&lmap_, sizeof(linkmap_t), offset) == sizeof(linkmap_t))
		return &lmap_;

	return NULL;
}

// Read data from given offset
ssize_t Reader::readCoreData(void* buff_, size_t size_, off_t offset_)
{
	if (!buff_)
		return -1;

	return pread(m_fd, buff_, size_, offset_);
}

// Iterate through NOTE data fron given offset and return next prstatus structure
size_t Reader::getNextPrStatus(Elf_Data* notes_, size_t pos_, prstatus_t& prs_)
{
	GElf_Nhdr nhdr;
	size_t name_pos, desc_pos;

	while((pos_ = gelf_getnote(notes_, pos_, &nhdr, &name_pos, &desc_pos)) > 0) {
		if (nhdr.n_type != NT_PRSTATUS)
			continue;

		memcpy(&prs_, (char *)notes_->d_buf + desc_pos, sizeof(prs_));
		break;
	}

	return pos_;
}

// Return RSP register from prstatus structure
GElf_Addr Reader::getStack(const prstatus_t& prs_)
{
	return ((regs_t*) &(prs_.pr_reg))->rsp;
}

// Read stack segment
Elf_Data* Reader::getStackData(const prstatus_t& prs_, GElf_Addr& vaddr_dst_)
{
	GElf_Phdr phdr;
	GElf_Addr vaddr = getStack(prs_);

	if (!findPhdrByVaddr(vaddr, phdr))
		return NULL;

	// let's align stack pointer and take this as segment begin address
	vaddr = vaddr / phdr.p_align * phdr.p_align; // align

	off_t offset = phdr.p_offset + (vaddr - phdr.p_vaddr);
	size_t size = phdr.p_filesz - (vaddr - phdr.p_vaddr);

	vaddr_dst_ = vaddr;

	return elf_getdata_rawchunk(m_core, offset, size, ELF_T_BYTE);
}

} //namespace CoRipper
