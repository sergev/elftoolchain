/*-
 * Copyright (c) 2012,2013 Kai Wang
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ld.h"
#include "ld_arch.h"
#include "ld_dynamic.h"
#include "ld_input.h"
#include "ld_output.h"
#include "ld_reloc.h"
#include "ld_symbols.h"
#include "ld_utils.h"
#include "mips.h"

ELFTC_VCSID("$Id$");

static void
_scan_reloc(struct ld *ld, struct ld_input_section *is,
    struct ld_reloc_entry *lre)
{
	//struct ld_symbol *lsb = ld_symbols_ref(lre->lre_sym);

	switch (lre->lre_type) {

	case R_MIPS_NONE:
	case R_MIPS_32:
	case R_MIPS_26:
	case R_MIPS_PC16:
		break;

	default:
		ld_warn(ld, "can not handle relocation %ju",
		    lre->lre_type);
		break;
	}
}

static void
_process_reloc(struct ld *ld, struct ld_input_section *is,
    struct ld_reloc_entry *lre, struct ld_symbol *lsb, uint8_t *buf)
{
	struct ld_output *lo = ld->ld_output;
	uint32_t p, s;
	int32_t a, v;

	assert(lo != NULL);

	p = lre->lre_offset + is->is_output->os_addr + is->is_reloff;
	s = (uint32_t) lsb->lsb_value;
	READ_32(buf + lre->lre_offset, a);

	switch (lre->lre_type) {

	case R_MIPS_NONE:
		break;

	case R_MIPS_32:
		/* 32-bit byte address. */
		v = s + a;
		WRITE_32(buf + lre->lre_offset, v);
		break;

	case R_MIPS_26:
		/* Word address at lower 26 bits. */
		s += (a & 0x3ffffff) << 2;
		v = (a & ~0x3ffffff)| ((s >> 2) & 0x3ffffff);
		WRITE_32(buf + lre->lre_offset, v);
		break;

	case R_MIPS_PC16:
		/* PC-relative word address at lower 16 bits. */
		s += ((a & 0xffff) << 2) - p;
		v = (a & ~0xffff)| ((s >> 2) & 0xffff);
		WRITE_32(buf + lre->lre_offset, v);
		break;

	default:
		ld_fatal(ld, "Relocation %d not supported", lre->lre_type);
		break;
	}
}

static uint64_t
_get_max_page_size(struct ld *ld)
{
	return 0x1000;
}

static uint64_t
_get_common_page_size(struct ld *ld)
{
	return 0x1000;
}

static int
_is_absolute_reloc(uint64_t r)
{
	if (r == R_MIPS_32)
		return 1;

	return 0;
}

static int
_is_relative_reloc(uint64_t r)
{
	if (r == R_MIPS_REL32)
		return 1;

	return 0;
}

void
mips_register(struct ld *ld)
{
	struct ld_arch *mips_little_endian, *mips_big_endian;

	if ((mips_little_endian = calloc(1, sizeof(*mips_little_endian))) == NULL)
		ld_fatal_std(ld, "calloc");
	if ((mips_big_endian = calloc(1, sizeof(*mips_big_endian))) == NULL)
		ld_fatal_std(ld, "calloc");

	/*
	 * Little endian.
	 */
	snprintf(mips_little_endian->name, sizeof(mips_little_endian->name), "%s", "littlemips");

	mips_little_endian->script = mips_script;
	mips_little_endian->get_max_page_size = _get_max_page_size;
	mips_little_endian->get_common_page_size = _get_common_page_size;
	mips_little_endian->scan_reloc = _scan_reloc;
	mips_little_endian->process_reloc = _process_reloc;
	mips_little_endian->is_absolute_reloc = _is_absolute_reloc;
	mips_little_endian->is_relative_reloc = _is_relative_reloc;
	mips_little_endian->reloc_is_64bit = 0;
	mips_little_endian->reloc_is_rela = 0;
	mips_little_endian->reloc_entsize = sizeof(Elf32_Rel);

	/*
	 * Big endian.
	 */
	snprintf(mips_big_endian->name, sizeof(mips_big_endian->name), "%s", "bigmips");

	mips_big_endian->script = mips_script;
	mips_big_endian->get_max_page_size = _get_max_page_size;
	mips_big_endian->get_common_page_size = _get_common_page_size;
	mips_big_endian->scan_reloc = _scan_reloc;
	mips_big_endian->process_reloc = _process_reloc;
	mips_big_endian->is_absolute_reloc = _is_absolute_reloc;
	mips_big_endian->is_relative_reloc = _is_relative_reloc;
	mips_big_endian->reloc_is_64bit = 0;
	mips_big_endian->reloc_is_rela = 0;
	mips_big_endian->reloc_entsize = sizeof(Elf32_Rel);

	HASH_ADD_STR(ld->ld_arch_list, name, mips_little_endian);
	HASH_ADD_STR(ld->ld_arch_list, name, mips_big_endian);
}
