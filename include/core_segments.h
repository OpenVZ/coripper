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

#ifndef __CORE_SEGMENTS_H__
#define __CORE_SEGMENTS_H__

#include <list>
#include <core_reader.h>

namespace CoRipper
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Header

struct Header
{
	typedef boost::shared_ptr<Header> ptr_t;

	Header(const GElf_Ehdr h_ = GElf_Ehdr()): m_header(h_)
	{
	}

	const char* getData() const
	{
		return reinterpret_cast<const char*>(&m_header);
	}
	size_t getSize() const
	{
		return sizeof(m_header);
	}
	size_t getHeaderSize() const
	{
		return m_header.e_phentsize;
	}
	size_t getOffset() const
	{
		return sizeof(m_header) + m_header.e_phnum * m_header.e_phentsize;
	}
private:
	GElf_Ehdr m_header;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Segment

struct Segment
{
	typedef boost::shared_ptr<Segment> ptr_t;
	typedef std::list<ptr_t> list_t;
	typedef GElf_Phdr Header;

	virtual ~Segment() = 0;

	virtual Header getHeader(GElf_Off offset = 0) const
	{
		GElf_Phdr phdr;
		phdr.p_type = PT_LOAD;
		phdr.p_align = 0;
		phdr.p_memsz = 0;
		phdr.p_flags = PF_R | PF_W | PF_X;
		phdr.p_vaddr = 0;
		phdr.p_filesz = 0;
		phdr.p_offset = offset;
		return phdr;
	}

	virtual const char* getBuffer() const = 0;
	virtual size_t getSize() const = 0;
}; //struct Segment

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Stack

struct Stack: Segment
{
	typedef boost::shared_ptr<Stack> ptr_t;

	Stack(GElf_Addr vaddr, Elf_Data* stackData)
	: m_vaddr(vaddr),  m_stackData(stackData)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = Segment::getHeader();
		phdr.p_vaddr = m_vaddr;
		phdr.p_filesz = getSize();
		phdr.p_offset = offset;
		return phdr;
	}
	virtual const char* getBuffer() const
	{
		return reinterpret_cast<const char*>(m_stackData->d_buf);
	}
	virtual size_t getSize() const
	{
		return m_stackData->d_size;
	}

private:
	GElf_Addr m_vaddr;
	Elf_Data* m_stackData;
}; //struct Stack

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct String

struct String: Segment
{
	typedef boost::shared_ptr<String> ptr_t;

	String(GElf_Addr vaddr, const std::vector<char>& buf)
	: m_vaddr(vaddr), m_buf(buf)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = Segment::getHeader();
		phdr.p_vaddr = m_vaddr;
		phdr.p_filesz = getSize();
		phdr.p_offset = offset;
		return phdr;
	}

	virtual const char* getBuffer() const
	{
		return &m_buf[0];
	}

	virtual size_t getSize() const
	{
		return m_buf.size();
	}

private:
	GElf_Addr m_vaddr;
	std::vector<char> m_buf;
}; //struct String

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Linkmap

struct Linkmap: Segment
{
	typedef boost::shared_ptr<Linkmap> ptr_t;
	typedef std::pair<CoRipper::Segment::list_t, CoRipper::Segment::list_t> listPair_t;

	Linkmap(GElf_Addr vaddr, const linkmap_t& lmap)
	: m_vaddr(vaddr), m_lmap(lmap)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = Segment::getHeader();
		phdr.p_vaddr = m_vaddr;
		phdr.p_filesz = getSize();
		phdr.p_offset = offset;
		return phdr;
	}
	virtual const char* getBuffer() const
	{
		return reinterpret_cast<const char*>(&m_lmap);
	}
	virtual size_t getSize() const
	{
		return sizeof(m_lmap);
	}
	GElf_Addr getNext() const
	{
		return reinterpret_cast<GElf_Addr>(m_lmap.l_next);
	}
	GElf_Addr getName() const
	{
		return reinterpret_cast<GElf_Addr>(m_lmap.l_name);
	}

private:
	GElf_Addr m_vaddr;
	linkmap_t m_lmap;
}; //struct Linkmap

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct RDebug

struct RDebug: Segment
{
	typedef boost::shared_ptr<RDebug> ptr_t;

	RDebug(const GElf_Dyn& debugDyn, const rdebug_t& rdebug)
	: m_debugDyn(debugDyn), m_rdebug(rdebug)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = Segment::getHeader();
		phdr.p_vaddr = m_debugDyn.d_un.d_ptr;
		phdr.p_filesz = getSize();
		phdr.p_offset = offset;
		return phdr;
	}
	virtual const char* getBuffer() const
	{
		return reinterpret_cast<const char*>(&m_rdebug);
	}
	virtual size_t getSize() const
	{
		return sizeof(m_rdebug);
	}
	const rdebug_t& getData() const
	{
		return m_rdebug;
	}

private:
	GElf_Dyn m_debugDyn;
	rdebug_t m_rdebug;
}; //struct RDebug

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Dynamic

struct Dynamic: Segment
{
	typedef boost::shared_ptr<Dynamic> ptr_t;

	Dynamic(const GElf_Phdr& dynPhdr, Elf_Data* dynData)
	: m_dynPhdr(dynPhdr), m_dynData(dynData)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = m_dynPhdr;
		phdr.p_type = PT_LOAD;
		phdr.p_paddr = 0;
		phdr.p_offset = offset;
		return phdr;
	}
	virtual const char* getBuffer() const
	{
		return reinterpret_cast<const char*>(m_dynData->d_buf);
	}
	virtual size_t getSize() const
	{
		return m_dynData->d_size;
	}
	Elf_Data* getData()
	{
		return m_dynData;
	}

private:
	GElf_Phdr m_dynPhdr;
	Elf_Data* m_dynData;
}; //struct Dynamic

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Note

struct Note: Segment
{
	typedef boost::shared_ptr<Note> ptr_t;

	Note(const GElf_Phdr& notePhdr, Elf_Data* noteData)
	: m_notePhdr(notePhdr), m_noteData(noteData)
	{
	}

	virtual Segment::Header getHeader(GElf_Off offset) const
	{
		GElf_Phdr phdr = m_notePhdr;
		phdr.p_offset = offset;
		return phdr;
	}
	virtual const char* getBuffer() const
	{
		return reinterpret_cast<const char*>(m_noteData->d_buf);
	}
	virtual size_t getSize() const
	{
		return m_noteData->d_size;
	}
	GElf_Phdr& getHeader()
	{
		return m_notePhdr;
	}
	Elf_Data* getData()
	{
		return m_noteData;
	}

private:
	GElf_Phdr m_notePhdr;
	Elf_Data* m_noteData;
}; //struct Note

////////////////////////////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder
{
	typedef std::pair<Header, Segment::list_t> data_t;

	Builder(Reader& r_): m_reader(&r_)
	{
	}

	~Builder()
	{
	}

	bool getResult(data_t& dst_);
	bool readNote();
	bool readDynamic();
	bool readRDebug();
	bool readStacks();
	bool readLinkmaps();

private:
	Reader* m_reader;
	Segment::list_t m_segments;
	Note::ptr_t m_note;
	Dynamic::ptr_t m_dynamic;
	RDebug::ptr_t m_rdebug;
};

std::ostream& operator<<(std::ostream& o_, const Builder::data_t& d_);

} //namespace CoRipper

#endif //__CORE_SEGMENTS_H__
