//////////////////////////////////////////////////////////////////////////
//
//   Win32 Standard Application template
//   (c) 2002-2005 _Servil_
//

#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include <excpt.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <time.h>
// #include <ctype.h>
// #include <share.h>
#include <windows.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

#pragma hdrstop
#pragma comment(linker, "/subsystem:console")

#include "cmangle.h"
#define AVOID_DLL 1
#include "common.h"
#include "textbuffer.h"
#include "pcre.h"
#ifdef _DEBUG
#pragma comment(lib, "libpcred.lib")
#else // !_DEBUG
#pragma comment(lib, "libpcre.lib")
#endif // _DEBUG

#if defined(_DEBUG) && !defined(new)
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif // _DEBUG && !new

int main(int argc, char* argv[], char* env[]);

#define OVECSIZE 30

inline pcre *pcre_compile(const char *const pattern, int const options = 0,
	const unsigned char *const tableptr = 0) {
	_ASSERTE(pattern != 0 && *pattern != 0);
	const char *errptr;
	int erroffset;
#ifndef _DEBUG
	return pcre_compile(pattern, options, &errptr, &erroffset, tableptr);
#else // _DEBUG
	pcre *result = pcre_compile(pattern, options, &errptr, &erroffset, tableptr);
	if (result == 0) _RPT4(_CRT_ERROR, "%s(\"%s\", ...): pcre_compile failed: %s(%u)",
		__FUNCTION__, pattern, errptr, erroffset);
	return result;
#endif // _DEBUG
}
inline int pcre_exec(const pcre *const code, const char *const subject,
	int *const ovector = 0, int const ovecsize = 0, int const startoffset = 0,
	int const options = 0) {
	_ASSERTE(code != 0 && subject != 0);
	if (ovector != 0 && ovecsize > 0) ZeroMemory(ovector, ovecsize << 2);
	return code != 0 && subject != 0 ? pcre_exec(code, 0/*pcre_extra **/,
		subject, strlen(subject), startoffset, options, ovector, ovecsize) : -1;
}

class pcregexp {
protected:
	pcre *regex;
	pcre_extra *extra;

public:
	pcregexp() : regex(0), extra(0) { }
	inline pcregexp(const char *const pattern, int const options = 0,
		const unsigned char *const tableptr = 0) : regex(0), extra(0) {
		compile(pattern, options, tableptr);
	}
	inline pcregexp(const char *const pattern, int const options,
		const char **const errptr, int *const erroffset,
		const unsigned char *const tableptr) : regex(0), extra(0) {
		compile(pattern, options, errptr, erroffset, tableptr);
	}
	inline virtual ~pcregexp() { release(); }

	bool compile(const char *const pattern, int const options,
		const char **const errptr, int *const erroffset,
		const unsigned char *const tableptr) {
		release();
		_ASSERTE(pattern != 0 && *pattern != 0);
		if (pattern == 0 || *pattern == 0) return 0;
		regex = pcre_compile(pattern, options, errptr, erroffset, tableptr);
#ifdef _DEBUG
		if (regex == 0) _RPT4(_CRT_ERROR, "%s(\"%s\", ...): pcre_compile failed: %s(%u)",
			__FUNCTION__, pattern, errptr, erroffset);
#endif // _DEBUG
		return regex != 0;
	}
	bool compile(const char *const pattern, int const options = 0,
		const unsigned char *const tableptr = 0) {
		release();
		_ASSERTE(pattern != 0 && *pattern != 0);
		if (pattern == 0 || *pattern == 0) return 0;
		const char *errptr;
		int erroffset;
		regex = pcre_compile(pattern, options, &errptr, &erroffset, tableptr);
#ifdef _DEBUG
		if (regex == 0) _RPT4(_CRT_ERROR, "%s(\"%s\", ...): pcre_compile failed: %s(%u)",
			__FUNCTION__, pattern, errptr, erroffset);
#endif // _DEBUG
		return regex != 0;
	}
	bool study(/*int const options = 0, */const char **errptr = 0) {
		_ASSERTE(regex != 0);
		if (extra != 0) {
			free(extra);
			extra = 0;
		}
		if (regex != 0) {
			const char *_errptr;
			extra = pcre_study(regex, 0/*options*/, &_errptr);
			if (errptr != 0) *errptr = _errptr;
#ifdef _DEBUG
			if (extra == 0) _RPT2(_CRT_WARN, "%s(...): pcre_study without result: %s",
				__FUNCTION__, _errptr);
#endif // _DEBUG
		}
		return extra != 0;
	}
	inline void release() {
		if (extra != 0) { free(extra); extra = 0; }
		if (regex != 0) { free(regex); regex = 0; }
	}
	int exec(const char *const subject, int const startoffset = 0,
		int const options = 0) {
		_ASSERTE(regex != 0 && subject != 0);
		int ovector[OVECSIZE];
		return regex != 0 && subject != 0 && pcre_exec(regex, extra, subject,
			strlen(subject), startoffset, options, ovector, OVECSIZE) >= 0 ? 0 : -1;
	}
	int exec(const char *const subject, int *const ovector, int const ovecsize,
		int const startoffset = 0, int const options = 0) {
#ifdef _DEBUG
		if (ovector == 0 && ovecsize > 0)
			_RPT1(_CRT_WARN, "%s(...): ovector size is non-zero but pointer to ovector is NULL: no subpatterns will be captured",
				__FUNCTION__);
		if (ovector != 0 && ovecsize == 0)
			_RPT1(_CRT_WARN, "%s(...): ovector size is zero but pointer to ovector is not NULL: no subpatterns will be captured",
				__FUNCTION__);
#endif // _DEBUG
		if (ovector == 0 || ovecsize == 0) return exec(subject, startoffset, options);
		_ASSERTE(regex != 0 && subject != 0);
		ZeroMemory(ovector, ovecsize << 2);
		return regex != 0 && subject != 0 ? pcre_exec(regex, extra, subject,
			strlen(subject), startoffset, options, ovector, ovecsize) : -1;
	}
	inline match(char *const subject, int const cflags = 0) {
		_ASSERTE(subject != 0 && regex != 0);
		if (subject == 0 || regex == 0) return -1;
		int ovector[OVECSIZE];
		return exec(subject, ovector, OVECSIZE) >= 1 ? ovector[0] : -1;
	}
	int copy_named_substring(const char *const subject, int *const ovector,
		const char *const stringname, char *buffer, int const buffersize) {
		_ASSERTE(buffer != 0 && buffersize > 0);
		if (buffer == 0 || buffersize == 0) return -1;
		*buffer = 0;
		_ASSERTE(regex != 0 && subject != 0 && ovector != 0);
		return regex != 0 && subject != 0 && ovector != 0 ?
			pcre_copy_named_substring(regex, subject, ovector, 0xFFFFFFF,
			stringname, buffer, buffersize) : -1;
	}
	int copy_substring(const char *const subject, int *const ovector,
		int const stringnumber, char *buffer, int const buffersize) {
		_ASSERTE(buffer != 0 && buffersize > 0);
		if (buffer == 0 || buffersize == 0) return -1;
		*buffer = 0;
		_ASSERTE(subject != 0 && ovector != 0);
		return subject != 0 && ovector != 0 ? pcre_copy_substring(subject, ovector,
			0xFFFFFFF, stringnumber, buffer, buffersize) : -1;
	}
	int get_named_substring(const char *const subject, int *const ovector,
		const char *const stringname, const char **const stringptr) {
		_ASSERTE(stringptr != 0);
		if (stringptr == 0) return -1;
		*stringptr = 0;
		_ASSERTE(regex != 0 && subject != 0 && ovector != 0);
		return regex != 0 && subject != 0 && ovector != 0 ?
			pcre_get_named_substring(regex, subject, ovector, 0xFFFFFFF, stringname,
			stringptr) : -1;
	}
	inline int get_stringnumber(const char *const name) {
		_ASSERTE(regex != 0 && name != 0);
		return regex != 0 && name != 0 ? pcre_get_stringnumber(regex, name) : -1;
	}
	int get_substring(const char *const subject, int *const ovector,
		int const stringnumber, const char **const stringptr) {
		_ASSERTE(stringptr != 0);
		if (stringptr == 0) return -1;
		*stringptr = 0;
		_ASSERTE(subject != 0 && ovector != 0);
		return subject != 0 && ovector != 0 ? pcre_get_substring(subject, ovector,
			0xFFFFFFF, stringnumber, stringptr) : -1;
	}
	/*
	int get_substring_list(const char *const subject, int *const ovector,
		const char ***const listptr) {
		_ASSERTE(listptr != 0);
		if (listptr == 0) return -1;
		*listptr = 0;
		_ASSERTE(subject != 0 && ovector != 0);
		return subject != 0 && ovector != 0 ? pcre_get_substring_list(subject,
			ovector, 0xFFFFFFF, listptr) : -1;
	}
	*/
	inline bool compiled() { return regex != 0; }
};

static const size_t MAXLINELENGTH = 0x20000; // should be enough?

#ifdef _DEBUG
#undef OutputDebugString
void OutputDebugString(const char *format, ...) {
	char tmp[MAXLINELENGTH];
	__try {
		va_list va;
		va_start(va, format);
		_vsnprintf(tmp, MAXLINELENGTH, format, va);
		va_end(va);
		OutputDebugStringA(tmp);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		_snprintf(tmp, sizeof tmp, "OutputDebugString(...): qvsnprintf(%s, ...) crashed",
			format);
		OutputDebugStringA(tmp);
	}
	return;
}
#else // !_DEBUG
#define OutputDebugString __noop
#define OutputDebugStringA __noop
#define OutputDebugStringW __noop
#endif // _DEBUG

static FILE *log = 0;

bool openlog(char *const filename = 0) {
	if (log != 0) return true;
	if ((log = fopen(filename != 0 && *filename != 0 ?
		filename : "omfpat.log", "atcS")) == 0) return false;
	fprintf(log, "-------------------------------------------------------------------------------\n");
	_tzset();
	char date[0x100], time[16];
	fprintf(log, "log opened at %s %s\n", _strdate(date), _strtime(time));
	fprintf(log, "-------------------------------------------------------------------------------\n");
	return true;
}

int writelog(char *const format, ...) {
	if (openlog()) {
		va_list va;
		va_start(va, format);
		int result = vfprintf(log, format, va);
		va_end(va);
		return result;
	}
	return 0;
}

class COmfInfo {
	struct link_t;
	typedef struct link_t *link_p;
	struct link_t {
	friend class COmfInfo;
		char *modname;
		inline link_t() : modname(0), next(0) { }
		inline link_t(char *const modname) : modname(0), next(0) {
			setModName(modname);
		}
		inline virtual ~link_t() { if (modname != 0) delete modname; }
		inline bool hasName() { return modname != 0 && *modname != 0; }
	private:
		bool setModName(char *const modname) {
			_ASSERTE(modname != 0);
			if (this->modname != 0) delete this->modname;
			if (modname != 0 && *modname != 0) {
				size_t sz = strlen(modname) + 1;
				if ((this->modname = new char[sz]) == 0) return false;
				strcpy(this->modname, modname);
			}
			return true;
		}
		link_p next;
	};

private:
	link_p links;
	char implements[0x100], verstr[0x100], library[0x100];
	unsigned __int32 loadflags, dictoffset;
	unsigned __int16 pagesize, dictlen;
	unsigned __int8 loadbyte, dictflags;
	unsigned int verbase, vendor, version;

	bool AddLink(char *const link) {
		_ASSERTE(link != 0 && *link != 0);
		if (link != 0 && *link != 0) {
			link_p last = 0;
			for (link_p tmp = links; tmp != 0; tmp = tmp->next) {
				if (tmp->hasName() && strcmp(link, tmp->modname) == 0) return false; // no dupes
				last = tmp;
			}
			if ((tmp = new link_t(link)) == 0) {
				_RPTF2(_CRT_ERROR, "%s(\"%s\"): couldnot allocate new item", __FUNCTION__, link);
				return false;
			}
			(links != 0 ? last->next : links) = tmp;
			return true;
		}
		return false;
	}
	virtual void Destroy() {
		link_p link;
		while ((link = links) != 0) {
			links = links->next;
			delete link;
		}
	}
	unsigned short omf_readindex(void *const record, unsigned short &index) {
		if (*((unsigned __int8 *)record + index) & 0x80)
			return ((*((unsigned __int8 *)record + index++) & 0x7F) << 8) +
				*((unsigned __int8 *)record + index++);
		else
			return *((unsigned __int8 *)record + index++);
	}
	unsigned __int32 omf_readvarvalue(void *const record,
		unsigned short &index) {
		unsigned __int32 value = *((unsigned __int8 *)record + index++);
		switch (value) {
			case 0x81:
				value = *(unsigned __int16 *)((__int8 *)record + index);
				index += 2;
				break;
			case 0x84:
				value = (*(unsigned __int32 *)((__int8 *)record + index)) & 0xFFFFFF;
				index += 3;
				break;
			case 0x88:
				value = *(unsigned __int32 *)((__int8 *)record + index);
				index += 4;
				break;
		}
		return value;
	}
	bool isKnownModule(char *const pattern, char *const modname) {
		if (pattern != 0 && *pattern != 0 && modname != 0 && *modname != 0) {
			char tmp[0x100];;
			_snprintf(tmp, sizeof tmp, "@%s@", modname);
			return _strnicmp(pattern, tmp, strlen(tmp)) == 0;
		}
		return false;
	}
	bool isKnownType(char *const pattern, char *const modname) {
		if (pattern != 0 && *pattern != 0 && modname != 0 && *modname != 0) {
			char tmp[0x100];;
			_snprintf(tmp, sizeof tmp, "%s@", modname);
			return _strnicmp(pattern, tmp, strlen(tmp)) == 0;
		}
		return false;
	}

public:
	COmfInfo(char *const objfile = 0) : links(0) {
		LoadObjFile(objfile);
	}
	inline virtual ~COmfInfo() { Destroy(); }

	bool LoadObjFile(char *const objfile);
	bool HasLink(char *const link) {
		_ASSERTE(link != 0 && *link != 0);
		if (link != 0 && *link != 0) {
			for (link_p tmp = links; tmp != 0; tmp = tmp->next)
				if (tmp->hasName() && strcmp(link, tmp->modname) == 0) return true; // no dupes
		}
		return false;
	}
	bool VclCanStartLinkedModule(char *const pattern) {
		_ASSERTE(pattern != 0 && *pattern != 0);
		if (pattern != 0 && *pattern != 0) {
			if (isKnownModule(pattern, implements)) return true; // self
			for (link_p tmp = links; tmp != 0; tmp = tmp->next)
				if (tmp->hasName() && isKnownModule(pattern, tmp->modname)) return true; // no dupes
		}
		return false;
	}
	bool VclCanStartLinkedType(char *const pattern) {
		_ASSERTE(pattern != 0 && *pattern != 0);
		if (pattern != 0 && *pattern != 0) {
			if (isKnownType(pattern, implements)) return true; // self
			for (link_p tmp = links; tmp != 0; tmp = tmp->next)
				if (tmp->hasName() && isKnownType(pattern, tmp->modname)) return true; // no dupes
		}
		return false;
	}
	inline const char *getImplements() { return (const char *)implements; }
};

bool COmfInfo::LoadObjFile(char *const objfile) {
	_ASSERTE(objfile != 0 && *objfile != 0);
	void *buf = 0;
	library[0] = 0;
	Destroy();
	implements[0] = 0;
	verstr[0] = 0;
	loadbyte = 0;
	loadflags = 0;
	pagesize = 0;
	dictoffset = 0;
	dictlen = 0;
	dictflags = 0;
	verbase = 0;
	vendor = 0;
	version = 0;
	if (objfile == 0 || *objfile == 0) return false;
	int libio = _open(objfile, _O_BINARY | _O_RDONLY, _S_IREAD);
	if (libio == -1) return false;
	try {
		#define recId (unsigned char)sig[0]
		char name[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(objfile, 0, 0, name, ext);
		char sig[3];
		if (_lseek(libio, 0, SEEK_SET) == 0 && _read(libio, sig, 1) == 1
			&& (recId == 0xF0 || recId == 0x80)) {
			// OMF lib or object
			char translator[0x100], modname[0x100];
			translator[0] = 0;
			modname[0] = 0;
			unsigned int chsbad = 0;
			unsigned int chsgood = 0;
			unsigned int reccnt = 0;
			bool seg_use32 = false;
			#define recsize ((unsigned __int16 *)&sig[1])
			_lseek(libio, 0x01, SEEK_SET);
			while (!_eof(libio)) {
				long recoff = _tell(libio) - 1;
				if (_read(libio, recsize, 2) != 2) throw "error reading the file";
				if ((buf = malloc(*recsize)) == 0
					|| _read(libio, buf, *recsize) != *recsize) throw "error reading the file";
				__asm inc reccnt
				/*
				 * The Checksum field is a 1-byte field that contains the negative sum (modulo 256) of all other bytes in the record.
				 * In other words, the checksum byte is calculated so that the low-order byte of the sum of all the bytes in the
				 * record, including the checksum byte, equals 0. Overflow is ignored. Some compilers write a 0 byte rather than
				 * computing the checksum, so either form should be accepted by programs that process object modules.
				 */
				if ((recId & ~1) != 0xF0 /* exclude F0H and F1H records */
					&& *((unsigned __int8 *)buf + *recsize - 1) /* checksum byte must be set */) { // care checksum
					unsigned char realsum = 0;
					// TODO Borland omf scheme specific
					for (unsigned __int16 cntr = 0; cntr < 3; cntr++) realsum += (unsigned __int8)sig[cntr];
					for (cntr = 0; cntr < *recsize; cntr++) realsum += *((unsigned __int8 *)buf + cntr);
					if (realsum == 0)
						chsgood++; // ok
					else {
						_RPT0(_CRT_WARN, "wrong checksum in omf library:");
						_RPT1(_CRT_WARN, "  record offset=%08X", recoff);
						_RPT1(_CRT_WARN, "  record type=%02X", recId);
						_RPT1(_CRT_WARN, "  record size=%04X", *recsize);
						_RPT2(_CRT_WARN, "  current/stored checksum: %02X/%02X", realsum,
							*((unsigned __int8 *)buf + *recsize - 1));
						chsbad++;
					} // bad sum
				} // care checksum
				switch (recId) {
					case 0x80: // 80H THEADR-Translator Header Record
						strncpy(translator, (char *)((__int8 *)buf + 1), *(unsigned __int8 *)buf + 1);
						break;
					case 0x82: // 82H LHEADR-Library Module Header Record
						strncpy(library, (char *)((__int8 *)buf + 1), *(unsigned __int8 *)buf + 1);
						break;
					case 0x88: { // 88H COMENT-Comment Record
						unsigned __int8 cmttype = *(unsigned __int8 *)buf; // comment type
						unsigned __int8 cmtcls = *((unsigned __int8 *)buf + 0x01); // comment class
						unsigned __int8 cmtsubtype = *((unsigned __int8 *)buf + 0x02);
						switch (cmtcls) {
							case 0x00:
								// 000010 COMENT  Purge: No , List: Yes, Class: 0   (000h)
								//     Translator: Delphi Pascal V17.0
								break;
							case 0xA0: // OMF extensions
								switch (cmtsubtype) {
									case 0x01: { // 88H IMPDEF-Import Definition Record (Comment Class A0, Subtype 01)
										unsigned __int8 impbyord = *((unsigned __int8 *)buf + 0x03);
										char impname[0x100], dllname[_MAX_PATH];
										strncpy(impname, (char *)((__int8 *)buf + 0x05),
											*((unsigned __int8 *)buf + 0x04) + 1);
										unsigned __int8 *tmp = (unsigned __int8 *)buf + 0x05 +
											*((unsigned __int8 *)buf + 0x04);
										strncpy(dllname, (char *)(tmp + 1), *tmp + 1);
										break;
									} // IMPDEF
								} // switch cmtsubtype
								break;
							case 0xA3: // 88H LIBMOD-Library Module Name Record (Comment Class A3)
								strncpy(modname, (char *)((__int8 *)buf + 0x02),
									*((unsigned __int8 *)buf + 0x01) + 1);
								break;
							case 0xFB: {
								char *dot;
								switch (cmtsubtype) {
									case 0x08: {
										// 000048 COMENT  Purge: No , List: Yes, Class: 251 (0FBh), SubClass: 8 (08h)
										//     Link: ComObj.obj
										char link[0x100];
										strncpy(link, (char *)((__int8 *)buf + 0x03),
											*((unsigned __int8 *)buf + 0x03) + 1);
										if ((dot = strchr(link, '.')) != 0) *dot = 0;
										AddLink(link);
										break;
									}
									case 0x0A:
										// 000035 COMENT  Purge: No , List: Yes, Class: 251 (0FBh), SubClass: 10 (0Ah)
										//     Implements: OWC10XP.obj
										strncpy(implements, (char *)((__int8 *)buf + 0x03),
											*((unsigned __int8 *)buf + 0x03) + 1);
										if ((dot = strchr(implements, '.')) != 0) *dot = 0;
										break;
									case 0x0C:
										// 00002A COMENT  Purge: Yes, List: Yes, Class: 251 (0FBh), SubClass: 12 (0Ch)
										//     Package Module Record, Lead Byte: 01h, Flags: 00004000h
										// 01 00 40 00 00 88 10 00 80
										loadbyte = *((unsigned __int8 *)buf + 0x03);
										loadflags = *((unsigned __int32 *)buf + 1);
										break;
								} // switch cmtsubtype
								break;
							}
						} // switch cmtcls
						break;
					}
					case 0x8A:
					case 0x8B: { // 8AH or 8BH MODEND-Module End Record
						unsigned __int8 modtype = *(unsigned __int8 *)buf;
						modname[0] = 0;
						Destroy();
						implements[0] = 0;
						break;
					}
					case 0x8C: { // 8CH EXTDEF-External Names Definition Record
						/*
						unsigned short pos = 0;
						while (pos + 1 < *recsize && *((__int8 *)buf + pos) != 0) {
							char name[0x100];
							strncpy(name, (char *)((__int8 *)buf + pos + 1),
								*((unsigned __int8 *)buf + pos) + 1);
							if (AddPublic(name, unknown, modname)) __asm inc result
							pos += *((unsigned __int8 *)buf + pos) + 2;
						}
						*/
						break;
					}
					case 0x90:
					case 0x91: { // 90H or 91H PUBDEF-Public Names Definition Record
						unsigned short basegrpindex, basesegindex, pos = 0;
						unsigned __int16 baseframe;
						unsigned char offsize;
						char name[0x100];
						basegrpindex = omf_readindex(buf, pos);
						basesegindex = omf_readindex(buf, pos);
						if (basegrpindex == 0 && basesegindex == 0) {
							baseframe = *(unsigned __int16 *)((unsigned __int8 *)buf + pos);
							pos += 2;
						}
						offsize = 2 << (unsigned __int8)(recId == 0x91 || seg_use32);
						while (pos + 1 < *recsize) {
							unsigned __int8 len = *((unsigned __int8 *)buf + pos);
							strncpy(name, (char *)((__int8 *)buf + pos + 1), len + 1);
							// add name
							pos += 1 + len;
							unsigned __int32 offset = offsize == 2 ? *(unsigned __int16 *)
								((__int8 *)buf + pos) : *(unsigned __int32 *)((__int8 *)buf + pos);
							pos += offsize;
							unsigned short type = omf_readindex(buf, pos);
						} // walk pubnames
						break;
					}
					case 0x98:
					case 0x99: // 98H or 99H SEGDEF-Segment Definition Record
						seg_use32 = *(unsigned __int8 *)buf & 1;
						break;
					case 0xB0: { // B0H COMDEF-Communal Names Definition Record
						unsigned short pos = 0;
						while (pos + 1 < *recsize && *((__int8 *)buf + pos)) {
							char name[0x100];
							strncpy(name, (char *)((__int8 *)buf + pos + 1),
								*((unsigned __int8 *)buf + pos) + 1);
							// add name
							pos += *((unsigned __int8 *)buf + pos) + 1;
							unsigned short typindex = omf_readindex(buf, pos);
							unsigned char datatype = *((unsigned __int8 *)buf + pos++);
							unsigned __int32 value;
							if (datatype >= 1 && datatype <= 0x5F || datatype == 0x62) // NEAR
								value = omf_readvarvalue(buf, pos);
							else if (datatype == 0x61) { // FAR
								value = omf_readvarvalue(buf, pos);
								value = omf_readvarvalue(buf, pos);
							}
						} // walk comdefs
						break;
					}
					case 0xCC: // CCH VERNUM - OMF Version Number Record
						strncpy(verstr, (char *)((__int8 *)buf + 1),
							*((unsigned __int8 *)buf));
						sscanf("%u.%u.%u", verstr, verbase, vendor, version);
						break;
					case 0xF0: // Library Header Record
						pagesize = *recsize + 3;
						dictoffset = *(unsigned __int32 *)buf;
						dictlen = *((unsigned __int16 *)buf + 2);
						dictflags = *((unsigned __int8 *)buf + 6);
						break;
					case 0xF1:
						goto filedone; // Library End Record, quit parsing
				} // switch recId
				if (buf != 0) {
					free(buf);
					buf = 0;
				}
				// skip leading zeros to next paragraph alignment
				while (!_eof(libio) && _read(libio, sig, 1) == 1 && sig[0] == 0);
			} // walk the file
		filedone:
			/*
			if (dictoffset && dictlen) {
				// process dictionary, if possible
				safe_free(buf);
				size_t dictsize = dictlen << 9;
				if ((buf = malloc(0x200)) && _lseek(libio, dictoffset, SEEK_SET) ==
					dictoffset) {
					__int8 (*HTAB)[1] = (__int8 (*)[1])buf;
					__int8 *FFLAG = (__int8 *)buf + 37;
					for (unsigned short iter = 0; iter < dictlen; iter++) {
						if (_read(libio, buf, 0x200) != 0x200) throw "error reading the file";
						unsigned short pos = 38;
						char name[0x100];
						while (pos < 0x200 && *((__int8 *)buf + pos) != 0) {
							strncpy(name, (char *)((__int8 *)buf + pos + 1),
								*((unsigned __int8 *)buf + pos) + 1);
							if (AddPublic(name)) _asm inc result // no typeinfo available
							pos += 1 + *((unsigned __int8 *)buf + pos);
							unsigned __int16 *blocknumber = (unsigned __int16 *)
								((__int8 *)buf + pos);
							pos += 2 + ??? + (pos & 1); // Borland specific!!!
						} // walk symbols
					} // walk pages
				} // ready to read
			} // process dictionary
			*/
			OutputDebugString("omf library processed: %u records (chsum: %u good/%u bad)",
				reccnt, chsgood, chsbad);
		}
#ifdef _DEBUG
	} catch (const char *message) {
		_RPT1(_CRT_ERROR, "omf scan failed: %s", message != 0 && *message != 0 ? message :
			"unknown expection");
		if (buf != 0) free(buf);
		_close(libio);
		return false;
#endif // _DEBUG
	} catch (...) {
		_RPT1(_CRT_ERROR, "omf scan failed: %s", "unknown expection");
		if (buf != 0) free(buf);
		_close(libio);
		return false;
	}
	return true;
}

class CPartList {
public:
	struct part_t;
	typedef struct part_t *part_p;
	struct part_t {
	friend class CPartList;
		char *text;
		bool isname;
		inline part_t() : text(0), next(0) { }
		inline part_t(const char *const text, bool const isname) : isname(isname),
			text(0), next(0) {
			setText(text);
		}
		inline virtual ~part_t() { if (text != 0) delete text; }
		inline bool hasText() const { return text != 0 && *text != 0; }
	private:
		bool setText(const char *const text) const {
			_ASSERTE(text != 0);
			if (this->text != 0) delete this->text;
			if (text != 0 && *text != 0) {
				size_t sz = strlen(text) + 1;
				if (((char *)this->text = new char[sz]) == 0) return false;
				strcpy(this->text, text);
			}
			return true;
		}
		part_p next;
	};

private:
	part_p parts, enumerator;

	virtual void Destroy() {
		part_p part;
		while ((part = parts) != 0) {
			parts = parts->next;
			delete part;
		}
	}

public:
	CPartList() : parts(0), enumerator(0) { }
	inline virtual ~CPartList() { Destroy(); }

	bool Add(const char *const text, bool const isname) {
		_ASSERTE(text != 0 && *text != 0);
		if (text != 0 && *text != 0) {
			part_p last = 0;
			for (part_p tmp = parts; tmp != 0; tmp = tmp->next) last = tmp;
			if ((tmp = new part_t(text, isname)) == 0) {
				_RPTF2(_CRT_ERROR, "%s(\"%s\"): couldnot allocate new item", __FUNCTION__, text);
				return false;
			}
			(parts != 0 ? last->next : parts) = tmp;
			return true;
		}
		return false;
	}
	inline const part_p GetNext() {
		return (const part_p)(enumerator = enumerator != 0 ? enumerator->next : parts);
	}
	inline Reset() { enumerator = 0; }
	inline unsigned int GetItemCount() {
		unsigned int result = 0;
		for (part_p tmp = parts; tmp != 0; tmp = tmp->next) result++;
		return result;
	}
};

static COmfInfo *objinfo = 0;

class CBplManager {
friend int main(int argc, char* argv[], char* env[]);
public:
	struct module_t;
	typedef struct module_t *module_p;
	struct module_t {
	friend int main(int argc, char* argv[], char* env[]);
		union {
			LPVOID lpBaseOfImage;
			HMODULE hModule;
			HINSTANCE hInstance;
		};
		SIZE_T dwSize;
		CHAR FileName[_MAX_PATH];
		PCHAR lpBaseName;
		PIMAGE_DOS_HEADER doshdr;
		PIMAGE_NT_HEADERS pehdr;

		inline module_t() : lpBaseName(0), sections(0), exports(0), next(0) { }
		module_t(HMODULE const hModule, SIZE_T const dwSize = 0);
		virtual ~module_t();

		inline BOOL hasName() {
			return FileName[0] != 0;
		}
		inline LONG getBaseOffset() {
			return (DWORD)hModule - pehdr->OptionalHeader.ImageBase;
		}
		inline LPVOID RVA2EA(DWORD const dwRVA) {
			return (LPVOID)((DWORD)hModule + dwRVA);
		}
		inline DWORD EA2RVA(LPVOID const lpAddress) {
			return (DWORD)lpAddress - (DWORD)hModule;
		}
		inline LPVOID getEntryPoint() {
			return pehdr->OptionalHeader.AddressOfEntryPoint != 0 ?
				RVA2EA(pehdr->OptionalHeader.AddressOfEntryPoint) : 0;
		}

		struct section_t;
		typedef struct section_t *section_p;
		struct section_t {
			LPVOID lpBaseAddress;
			PIMAGE_SECTION_HEADER header;

			inline section_t() : next(0) { }
			inline section_t(LPVOID const lpBaseAddress) : lpBaseAddress(lpBaseAddress),
				header(0), next(0) { }

			LPSTR getName(LPSTR const Name);

		private:
			friend class CBplManager;
			friend struct module_t;
			section_p next;
		} *sections;

		section_p FindSection(LPCVOID lpAddress);

		struct export_t;
		typedef struct export_t *export_p;
		struct export_t {
			LPSTR lpDllName;
			LPVOID lpFunc;
			DWORD dwRVA;
			LPSTR lpName;
			WORD Ordinal;
			export_p next;

			inline export_t() : lpDllName(NULL), lpName(NULL), next(0) { }
			inline export_t(LPVOID const lpFunc, DWORD const dwRVA = 0,
				WORD const Ordinal = 0) : lpFunc(lpFunc), dwRVA(dwRVA), Ordinal(Ordinal),
				lpDllName(NULL), lpName(NULL), next(0) { }

			inline BOOL hasName() { return lpName != NULL && *lpName != 0; }
		} *exports;

		export_p FindExport(LPVOID const lpAddress);
		inline export_p FindExport(DWORD const dwFuncRva) {
			return FindExport((LPVOID)((DWORD)hModule + dwFuncRva));
		}
		// exact match
		export_p FindExport(LPSTR const lpName);
		export_p FindExport(WORD const Ordinal);
		unsigned int GetExportCount() {
			unsigned int result = 0;
			for (export_p export = exports; export != 0; export = export->next)
				result++;
			return result;
		}

	private:
		friend class CBplManager;
		module_p next;
	}; // module_t

private:
	module_p modules;

	void DestroyModule(module_p &module);
	PIMAGE_DOS_HEADER GetDosHdr(HMODULE const hModule);
	PIMAGE_NT_HEADERS GetPeHdr(HMODULE const hModule);

public:
	inline CBplManager() : modules(0) { }
	virtual ~CBplManager() {
		module_p module;
		while ((module = modules) != 0) {
			modules = modules->next;
			delete module;
		}
	}

	// module_t... manipulation
	module_p AddModule(LPSTR const lpDllName);
	bool RemoveModule(module_p const module);
	bool RemoveModule(HMODULE const hModule);
	module_p FindModule(LPVOID const lpAddress, BOOL const bExactMatch = FALSE);
	inline module_p FindModule(HMODULE const hModule, BOOL const bExactMatch = FALSE) {
		return FindModule((LPVOID)hModule, bExactMatch);
	}
	module_p FindModule(LPSTR const lpModuleName);
	module_t::export_p FindExport(LPSTR const lpName) { // seek through all modules, return 1-st match
		_ASSERTE(lpName != 0 && *lpName != 0);
		module_t::export_p export = 0;
		if (lpName != 0 && *lpName != 0)
			for (module_p module = modules; export == 0 && module != 0; module = module->next)
				export = module->FindExport(lpName);
		return export;
	}
	// exact match
	inline module_t::export_p FindExport(LPVOID const lpAddress) {
		module_p module = FindModule(lpAddress);
		return module != 0 ? module->FindExport(lpAddress) : 0;
	}
	module_t::export_p FindExportByTail(LPSTR const lpCoreName, pcregexp *const regexp = 0) {
		_ASSERTE(lpCoreName != 0 && *lpCoreName != 0);
		module_t::export_p match = 0;
		if (lpCoreName != 0 && *lpCoreName != 0) {
			size_t len = strlen(lpCoreName);
			for (module_p module = modules; module != 0; module = module->next)
				for (module_t::export_p export = module->exports; export != 0; export = export->next)
					if (export->hasName() && strlen(export->lpName) >= len
						&& strcmp(lpCoreName, export->lpName + strlen(export->lpName) - len) == 0
						&& (regexp == 0 || regexp->match(export->lpName) >= 0))
						if (match == 0)
							match = export;
						else { // ambiguous
							bool ambiguous = true;
							if (objinfo != 0) {
								bool expcanstart = objinfo->VclCanStartLinkedModule(export->lpName);
								bool matchcanstart = objinfo->VclCanStartLinkedModule(match->lpName);
								if (matchcanstart != expcanstart) {
									ambiguous = false;
									if (expcanstart) match = export;
								}
							}
							if (ambiguous) {
								_RPT4(_CRT_WARN, "%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								writelog("%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match\n",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								return 0;
							}
						}
			/*
#ifdef _DEBUG
			if (match != 0) {
				_ASSERTE(match->hasName());
				OutputDebugString("%s(\"%s\"): lookup successfull: %s", __FUNCTION__,
					lpCoreName, match->lpName);
			}
#endif // _DEBUG
			*/
		}
		return match;
	}
	module_t::export_p FindExportBySubstr(LPSTR const lpCoreName, pcregexp *const regexp = 0) {
		_ASSERTE(lpCoreName != 0 && *lpCoreName != 0);
		module_t::export_p match = 0;
		if (lpCoreName != 0 && *lpCoreName != 0) {
			size_t len = strlen(lpCoreName);
			for (module_p module = modules; module != 0; module = module->next)
				for (module_t::export_p export = module->exports; export != 0; export = export->next)
					if (export->hasName() && strlen(export->lpName) >= len
						&& strstr(export->lpName, lpCoreName) != 0 && (regexp == 0
						|| regexp->match(export->lpName) >= 0))
						if (match == 0)
							match = export;
						else { // ambiguous
							bool ambiguous = true;
							if (objinfo != 0) {
								bool expcanstart = objinfo->VclCanStartLinkedModule(export->lpName);
								bool matchcanstart = objinfo->VclCanStartLinkedModule(match->lpName);
								if (matchcanstart != expcanstart) {
									ambiguous = false;
									if (expcanstart) match = export;
								}
							}
							if (ambiguous) {
								_RPT4(_CRT_WARN, "%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								writelog("%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match\n",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								return 0;
							}
						}
			/*
#ifdef _DEBUG
			if (match != 0) {
				_ASSERTE(match->hasName());
				OutputDebugString("%s(\"%s\"): lookup successfull: %s", __FUNCTION__,
					lpCoreName, match->lpName);
			}
#endif // _DEBUG
			*/
		}
		return match;
	}
	module_t::export_p FindType(LPSTR const lpPrefix, LPSTR const lpCoreName,
		pcregexp *const regexp = 0) {
		_ASSERTE(lpPrefix != 0 && *lpPrefix != 0 && lpCoreName != 0 && *lpCoreName != 0);
		module_t::export_p match = 0;
		if (lpCoreName != 0 && *lpCoreName != 0) {
			size_t len = strlen(lpCoreName);
			int ovector[OVECSIZE];
			char *m;
			for (module_p module = modules; module != 0; module = module->next)
				for (module_t::export_p export = module->exports; export != 0; export = export->next)
					if (export->hasName() && strncmp(export->lpName, lpPrefix, 5) == 0
						&& strlen(export->lpName) >= len && strcmp(lpCoreName,
						m = export->lpName + strlen(export->lpName) - len) == 0
						&& !isalpha(*(m - 1)) && (regexp == 0
						|| regexp->exec(export->lpName, ovector, OVECSIZE) >= 0))
						if (match == 0)
							match = export;
						else { // ambiguous
							bool ambiguous = true;
							if (objinfo != 0 && regexp != 0) {
								char tmp[0x100];
								bool expcanstart = regexp->exec(export->lpName, ovector, OVECSIZE) >= 2
									&& pcre_copy_substring(export->lpName, (int *)&ovector, 2, 1, tmp, sizeof tmp) > 0
									&& objinfo->VclCanStartLinkedType(tmp);
								bool matchcanstart = regexp->exec(match->lpName, ovector, OVECSIZE) >= 2
									&& pcre_copy_substring(match->lpName, (int *)&ovector, 2, 1, tmp, sizeof tmp) > 0
									&& objinfo->VclCanStartLinkedType(tmp);
								if (matchcanstart != expcanstart) {
									ambiguous = false;
									if (expcanstart) match = export;
								}
							}
							if (ambiguous) {
								_RPT4(_CRT_WARN, "%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								writelog("%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match\n",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								return 0;
							}
						}
			/*
	#ifdef _DEBUG
			if (match != 0) {
				_ASSERTE(match->hasName());
				OutputDebugString("%s(\"%s\"): lookup successfull: %s", __FUNCTION__,
					lpCoreName, match->lpName);
			}
	#endif // _DEBUG
			*/
		}
		return match;
	}
	module_t::export_p FindFunction(LPSTR const lpCoreName, pcregexp *const regexp = 0) {
		_ASSERTE(lpCoreName != 0 && *lpCoreName != 0);
		module_t::export_p match = 0;
		if (lpCoreName != 0 && *lpCoreName != 0) {
			size_t len = strlen(lpCoreName);
			int ovector[OVECSIZE];
			module_t::export_p export;
			for (module_p module = modules; module != 0; module = module->next)
				for (export = module->exports; export != 0; export = export->next)
					if (export->hasName() && strlen(export->lpName) >= len
						&& strcmp(lpCoreName, export->lpName + strlen(export->lpName) - len) == 0
						&& (regexp == 0 || regexp->exec(export->lpName, ovector, OVECSIZE) >= 0))
						if (match == 0)
							match = export;
						else { // ambiguous
							bool ambiguous = true;
							if (objinfo != 0) {
								bool expcanstart = objinfo->VclCanStartLinkedModule(export->lpName);
								bool matchcanstart = objinfo->VclCanStartLinkedModule(match->lpName);
								if (matchcanstart != expcanstart) {
									ambiguous = false;
									if (expcanstart) match = export;
								}
							}
							if (ambiguous) {
								_RPTF4(_CRT_WARN, "%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								writelog("%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match\n",
									__FUNCTION__, lpCoreName, match->lpName, export->lpName);
								return 0;
							}
						}
			if (match == 0 && regexp != 0) {
				char *start = strchr(lpCoreName, '$');
				if (start != 0) {
					if (regexp->exec(lpCoreName, ovector, OVECSIZE) >= 2) {
						char zzz[0x100];
						pcre_copy_substring(lpCoreName, (int *)&ovector, 2, 1, zzz, sizeof zzz);
						pcregexp findnumber("\\d+");
						if (findnumber.compiled()) {
							// now parse the core name
							char foo[0x100], *name;
							size_t nlen;
							CPartList coreparts;
							start++;
							while (findnumber.exec(start, ovector, OVECSIZE) >= 1) {
								if (ovector[0] > 0) {
									strncpy(foo, start, ovector[0]);
									foo[ovector[0]] = 0;
									coreparts.Add(foo, false);
								}
								nlen = strtoul(start + ovector[0], &name, 10);
								if (nlen > 0 && strlen(name) >= nlen) {
									strncpy(foo, name, nlen);
									foo[nlen] = 0;
									coreparts.Add(foo, true);
									start = name + nlen;
								} else {
									strncpy(foo, start + ovector[0], ovector[1] - ovector[0]);
									foo[ovector[1] - ovector[0]] = 0;
									coreparts.Add(foo, false);
									start += ovector[1];
								}
							}
							if (*start != 0) coreparts.Add(start, false); // tail
							for (module = modules; module != 0; module = module->next)
								for (export = module->exports; export != 0; export = export->next) {
									if (export->hasName() && strlen(export->lpName) >= len
										&& (start = strchr(export->lpName, '$')) != 0
										&& strstr(export->lpName, zzz) != 0
										&& regexp->exec(export->lpName, ovector, OVECSIZE) >= 1) {
										CPartList exportparts;
										start++;
										while (findnumber.exec(start, ovector, OVECSIZE) >= 1) {
											if (ovector[0] > 0) {
												strncpy(foo, start, ovector[0]);
												foo[ovector[0]] = 0;
												exportparts.Add(foo, false);
											}
											nlen = strtoul(start + ovector[0], &name, 10);
											if (nlen > 0 && strlen(name) >= nlen) {
												strncpy(foo, name, nlen);
												foo[nlen] = 0;
												exportparts.Add(foo, true);
												start = name + nlen;
											} else {
												strncpy(foo, start + ovector[0], ovector[1] - ovector[0]);
												foo[ovector[1] - ovector[0]] = 0;
												exportparts.Add(foo, false);
												start += ovector[1];
											}
										}
										if (*start != 0) exportparts.Add(start, false); // tail
										if (coreparts.GetItemCount() != exportparts.GetItemCount()) goto next;
										coreparts.Reset();
										CPartList::part_p item1, item2;
										while ((item1 = coreparts.GetNext()) != 0) {
											if ((item2 = exportparts.GetNext()) == 0) goto next; // asssertion: part count mismatch
											if (!item1->hasText() || !item2->hasText()) goto next;
											char *ttt;
											if (strcmp(item1->text, item2->text) != 0) {
												if (!item1->isname || !item2->isname
													|| (len = strlen(item2->text)) <= (nlen = strlen(item1->text))
													|| strcmp(item1->text, ttt = item2->text + len - nlen) != 0
													|| __iscsym(*(ttt - 1))) goto next;
											}
										}
										if (exportparts.GetNext() != 0) goto next; // asssertion: part count mismatch
										if (match == 0)
											match = export;
										else { // ambiguous
											bool ambiguous = true;
											if (objinfo != 0) {
												bool expcanstart = objinfo->VclCanStartLinkedModule(export->lpName);
												bool matchcanstart = objinfo->VclCanStartLinkedModule(match->lpName);
												if (matchcanstart != expcanstart) {
													ambiguous = false;
													if (expcanstart) match = export;
												}
											}
											if (ambiguous) {
												_RPTF4(_CRT_WARN, "%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match",
													__FUNCTION__, lpCoreName, match->lpName, export->lpName);
												writelog("%s(\"%s\"): ambiguous matches for this name: %s and %s, returning no match\n",
													__FUNCTION__, lpCoreName, match->lpName, export->lpName);
												return 0;
											}
										}
									}
							next:;
								}
						}
					}
				}
#ifdef _DEBUG
				if (match != 0) {
					_ASSERTE(match->hasName());
					OutputDebugString("%s(\"%s\"): lookup successfull: %s", __FUNCTION__,
						lpCoreName, match->lpName);
				}
#endif // _DEBUG
			}
		}
		/*
#ifdef _DEBUG
		if (match != 0) {
			_ASSERTE(match->hasName());
			OutputDebugString("%s(\"%s\"): lookup successfull: %s", __FUNCTION__,
				lpCoreName, match->lpName);
		}
#endif // _DEBUG
		*/
		return match;
	}
};

#define REPLACE_BY_FULLMATCH { \
	_snprintf(tmp, MAXLINELENGTH, "%-.*s%s%s", pos - ln + ovector[0], ln, \
		export->lpName, pos + ovector[1]); \
	strncpy(ln, tmp, MAXLINELENGTH); \
	pos += ovector[0] + strlen(export->lpName); \
	changed = true; \
	complete++; \
}

class CTypeList {
public:
	struct type_t;
	typedef struct type_t *type_p;
	struct type_t {
	friend class CTypeList;
		char *text;
		inline type_t() : text(0), next(0) { }
		inline type_t(const char *const text) : text(0), next(0) {
			setText(text);
		}
		inline virtual ~type_t() {
			if (text != 0) delete text;
		}
		inline bool hasText() const { return text != 0 && *text != 0; }
	private:
		bool setText(const char *const text) const {
			_ASSERTE(text != 0);
			if (this->text != 0) delete this->text;
			if (text != 0 && *text != 0) {
				size_t sz = strlen(text) + 1;
				if (((char *)this->text = new char[sz]) == 0) return false;
				strcpy(this->text, text);
			}
			return true;
		}
		type_p next;
	};

private:
	type_p types, enumerator;

	virtual void Destroy() {
		type_p type;
		while ((type = types) != 0) {
			types = types->next;
			delete type;
		}
	}

public:
	CTypeList() : types(0), enumerator(0) { }
	inline virtual ~CTypeList() { Destroy(); }

	bool Add(const char *const text) {
		_ASSERTE(text != 0 && *text != 0);
		if (text != 0 && *text != 0) {
			type_p last = 0;
			for (type_p tmp = types; tmp != 0; tmp = tmp->next) last = tmp;
			if ((tmp = new type_t(text)) == 0) {
				_RPTF2(_CRT_ERROR, "%s(\"%s\"): couldnot allocate new item", __FUNCTION__, text);
				return false;
			}
			(types != 0 ? last->next : types) = tmp;
			return true;
		}
		return false;
	}
	inline const type_p GetNext() {
		return (const type_p)(enumerator = enumerator != 0 ? enumerator->next : types);
	}
	inline Reset() { enumerator = 0; }
	inline unsigned int GetItemCount() {
		unsigned int result = 0;
		for (type_p tmp = types; tmp != 0; tmp = tmp->next) result++;
		return result;
	}
};

/////////////////////////////////////////////////////////////////////////////
// main function body - executable entry point
int main(int argc, char* argv[], char* env[]) {
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_finddata_t fi;
	long fh = _findfirst(argv[1], &fi);
	if (fh != -1) {
		char drive[_MAX_DRIVE], path[_MAX_PATH], ln[MAXLINELENGTH], tmp[MAXLINELENGTH];
		_splitpath(argv[1], drive, path, 0, 0);
		pcregexp regexp[6];
		regexp[0].compile("(?<!\\S)((?:\\@\\w+)*\\@{1,2}" BCNAME "(?:\\@\\$[a-zA-Z]+)?\\$[a-zA-Z]+)[\\w\\@\\$\\%\\&\\?]+(?!\\S)");
		regexp[0].study();
		regexp[1].compile("(?<!\\S)\\@\\$x[pt]\\$\\d*([\\w\\@\\$\\%\\&\\?]+)(?!\\S)");
		regexp[1].study();
		regexp[2].compile("(?<!\\S)(?:\\@\\w+)*\\@" BCNAME "\\@(?!\\S)");
		regexp[2].study();
		regexp[3].compile("(?<!\\S)(?:\\@\\w+)*\\@" BCNAME "(?!\\S)");
		regexp[3].study();
		regexp[4].compile("^\\@\\$x[pt]\\$(\\d+)([\\w\\@]+)$");
		regexp[4].study();
		regexp[5].compile("^(?:\\w+\\@)*(\\w+)$");
		CBplManager bplmanager;
		for (int index = 2; index < argc; index++) bplmanager.AddModule(argv[index]);
		int ovector[OVECSIZE];
		CTypeList typelist;
		if (bplmanager.modules != 0) {
			printf("please wait...");
			for (CBplManager::module_p module1 = bplmanager.modules; module1 != 0;
				module1 = module1->next) {
				for (CBplManager::module_t::export_p export1 = module1->exports;
					export1 != 0; export1 = export1->next)
					if (export1->hasName() && (strncmp(export1->lpName, "@$xp$", 5) == 0
						|| strncmp(export1->lpName, "@$xt$", 5) == 0)
						&& regexp[4].exec(export1->lpName, ovector, OVECSIZE) >= 3) {
						const char *t = export1->lpName + ovector[4];
						if (regexp[5].exec(t, ovector, OVECSIZE) >= 2) {
							const char *u = t + ovector[2];
							unsigned int matchcount = 0;
							for (CBplManager::module_p module2 = bplmanager.modules; module2 != 0;
								module2 = module2->next)
								for (CBplManager::module_t::export_p export2 = module2->exports;
									export2 != 0; export2 = export2->next)
									if (export2->hasName()
										&& strncmp(export2->lpName, export1->lpName, 5) == 0
										&& regexp[4].exec(export2->lpName, ovector, OVECSIZE) >= 3) {
										const char *t = export2->lpName + ovector[4];
										if (regexp[5].exec(t, ovector, OVECSIZE) >= 2
											&& strcmp(u, t + ovector[2]) == 0) matchcount++;
									}
							_ASSERTE(matchcount > 0);
							if (matchcount < 2) typelist.Add(t);
						}
					}
				putchar('.');
			}
			printf("\n");
		}
		do {
			if ((fi.attrib & _A_SUBDIR) == 0) {
				char fname[_MAX_FNAME];
				_splitpath(fi.name, 0, 0, fname, 0);
				_makepath(tmp, drive, path, 0, 0);
				strcat(tmp, fi.name);
				strncpy(fi.name, tmp, sizeof fi.name);
				CFileBuffer file(fi.name);
				if (file.GetLineCount() > 0) try {
					printf("processing file %s...", fi.name);
					if (FileExist(tmp)) objinfo = new COmfInfo(tmp);
					unsigned int line = 0;
					unsigned int total = 0; // total unmatched
					unsigned int complete = 0; // total successfull
					while (file.ReadLine(ln, ++line)) {
#ifdef _DEBUG
						char old[MAXLINELENGTH];
						strncpy(old, ln, MAXLINELENGTH);
#endif // _DEBUG
						bool changed = false;
						char *pos = ln;
						CBplManager::module_t::export_p export;
						// functions
						if (regexp[0].compiled()) {
							while (regexp[0].exec(pos, ovector, OVECSIZE) >= 2) {
								pcre_copy_substring(pos, (int *)&ovector, 2, 0, tmp, MAXLINELENGTH);
								if (bplmanager.FindExport(tmp) == 0) {
									total++;
									if ((export = bplmanager.FindFunction(tmp, &regexp[0])) != 0 && export->hasName())
										REPLACE_BY_FULLMATCH
									else { // @Object@Function$qqrp
										pcre_copy_substring(pos, (int *)&ovector, 2, 1, tmp, MAXLINELENGTH);
										if ((export = bplmanager.FindExportBySubstr(tmp, &regexp[0])) != 0 && export->hasName())
											REPLACE_BY_FULLMATCH
										else {
											char tm[0x100];
											pcre_copy_substring(pos, (int *)&ovector, 2, 0, tm, sizeof tm);
											writelog("%s(%u): full name for function not found finally: %s\n",
												fi.name, line, tm);
											_RPT3(_CRT_WARN, "%s(%u): full name for function not found finally: %s",
												fi.name, line, tm);
											bool ch = false;
											typelist.Reset();
											CTypeList::type_p type;
											while ((type = typelist.GetNext()) != 0) {
												int ovector[15];
												if (type->hasText() && regexp[5].exec(type->text, ovector, 15) >= 2
													&& ovector[2] > 0) {
													char *t = type->text + ovector[2];
													char locn[0x100], globn[0x100];
													_snprintf(locn, sizeof locn, "%u%s", strlen(t), t);
													_snprintf(globn, sizeof globn, "%u%s", strlen(type->text), type->text);
													char *z, *x = tm;
													while ((z = strstr(x, locn)) != 0) {
														char y[0x100];
														_snprintf(y, sizeof y, "%-.*s%s%s", z - tm, tm,
															globn, z + strlen(locn));
														strncpy(tm, y, sizeof tm);
														x = z + strlen(globn);
														changed = true;
														ch = true;
													}
												}
											}
											if (ch) {
												_snprintf(tmp, MAXLINELENGTH, "%-.*s%s%s", pos - ln + ovector[0], ln,
													tm, pos + ovector[1]);
												strncpy(ln, tmp, MAXLINELENGTH);
												pos += ovector[0] + strlen(tm);
												changed = true;
											} else
												pos += ovector[1];
										}
									}
								} else
									pos += ovector[1];
							}
							pos = ln;
						}
						// typedefs
						if (regexp[1].compiled()) {
							while (regexp[1].exec(pos, ovector, OVECSIZE) >= 2) {
								pcre_copy_substring(pos, (int *)&ovector, 2, 0, tmp, MAXLINELENGTH);
								if (bplmanager.FindExport(tmp) == 0) {
									total++;
									pcre_copy_substring(pos, (int *)&ovector, 2, 1, tmp, MAXLINELENGTH);
									if ((export = bplmanager.FindType(pos + ovector[0], tmp, &regexp[1])) != 0 && export->hasName())
										REPLACE_BY_FULLMATCH
									else {
										pcre_copy_substring(pos, (int *)&ovector, 2, 0, tmp, MAXLINELENGTH);
										writelog("%s(%u): full name for type not found finally: %s\n",
											fi.name, line, tmp);
										_RPT3(_CRT_WARN, "%s(%u): full name for type not found finally: %s",
											fi.name, line, tmp);
										pos += ovector[1];
									}
								} else
									pos += ovector[1];
							}
							pos = ln;
						}
						// objects
						if (regexp[2].compiled()) {
							while (regexp[2].exec(pos, ovector, OVECSIZE) >= 1) {
								pcre_copy_substring(pos, (int *)&ovector, 1, 0, tmp, MAXLINELENGTH);
								if (bplmanager.FindExport(tmp) == 0) {
									total++;
									if ((export = bplmanager.FindExportByTail(tmp, &regexp[2])) != 0 && export->hasName())
										REPLACE_BY_FULLMATCH
									else {
										pcre_copy_substring(pos, (int *)&ovector, 1, 0, tmp, MAXLINELENGTH);
										writelog("%s(%u): full name for object not found finally: %s\n",
											fi.name, line, tmp);
										_RPT3(_CRT_WARN, "%s(%u): full name for object not found finally: %s",
											fi.name, line, tmp);
										pos += ovector[1];
									}
								} else
									pos += ovector[1];
							}
							pos = ln;
						}
						// variables - data
						if (regexp[3].compiled()) {
							while (regexp[3].exec(pos, ovector, OVECSIZE) >= 1) {
								pcre_copy_substring(pos, (int *)&ovector, 1, 0, tmp, MAXLINELENGTH);
								if (bplmanager.FindExport(tmp) == 0) {
									total++;
									if ((export = bplmanager.FindExportByTail(tmp, &regexp[3])) != 0 && export->hasName())
										REPLACE_BY_FULLMATCH
									else {
										pcre_copy_substring(pos, (int *)&ovector, 1, 0, tmp, MAXLINELENGTH);
										writelog("%s(%u): full name for variable not found finally: %s\n",
											fi.name, line, tmp);
										_RPT3(_CRT_WARN, "%s(%u): full name for variable not found finally: %s",
											fi.name, line, tmp);
										pos += ovector[1];
									}
								} else
									pos += ovector[1];
							}
							pos = ln;
						}
						if (changed) {
#ifdef _DEBUG
							OutputDebugString("Changes made on line %u:", line);
							OutputDebugString("  old: %s", old);
							OutputDebugString("  new: %s", ln);
#endif // _DEBUG
							file.WriteLine(ln);
						}
					}
					printf("done: %u full names comlpeted, %u names unmatched\n",
						complete, total - complete);
					writelog("%s: %u full names comlpeted, %u names unmatched\n",
						fi.name, complete, total - complete);
					if (objinfo != 0) {
						delete objinfo;
						objinfo = 0;
					}
				} catch (...) {
					file.Close(false); // no save
					_RPTF0(_CRT_ERROR, "%s(...): unhandled exception, nothing done");
				}
				file.Close(true); // save
			}
		} while (_findnext(fh, &fi) != -1);
		_findclose(fh);
		if (log != 0) {
			printf("review log for detailed information about refused types\n");
			fclose(log);
		}
		for (index = 0; index < 6; index++) regexp[index].release();
	}
	_ASSERTE(_CrtDumpMemoryLeaks() == 0);
	return 0;
}

#define _expdir (tmp->pehdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT])
#define _ExpByOrd (expdir->NumberOfNames == 0 || expdir->AddressOfNames == 0 \
	|| expdir->AddressOfNames == expdir->AddressOfNameOrdinals)
#define _ExpBufAddr(RVA) ((__int8 *)expdir + ((DWORD)(RVA) - _expdir.VirtualAddress))
#define _IsInExpBuf(VA) ((__int8 *)(VA) >= (__int8 *)expdir \
	&& (__int8 *)(VA) < (__int8 *)expdir + _expdir.Size)
#define _IsInExpDir(RVA) (((DWORD)(RVA)) >= _expdir.VirtualAddress \
	&& ((DWORD)(RVA)) < _expdir.VirtualAddress + _expdir.Size)
#define _ExportItem(type, member, index) ((*((type(*)[]) \
	_ExpBufAddr(expdir->AddressOf##member)))[(index)])
#define _Ordinal(index) _ExportItem(const WORD, NameOrdinals, (index))
#define _NameRVA(index) _ExportItem(LPCSTR const, Names, (index))
#define _FuncRVA(index) _ExportItem(const DWORD, Functions, (index))
#define RVA2EA(dwRVA) ((LPVOID)((char *)hModule + (dwRVA)))

CBplManager::module_p CBplManager::AddModule(LPSTR const lpDllName) {
	_ASSERTE(lpDllName != 0 && *lpDllName != 0);
	if (lpDllName == 0 || *lpDllName == 0) return 0;
	HMODULE hModule = LoadLibrary(lpDllName);
	if (hModule == 0) {
		_RPT2(_CRT_WARN, "%s(\"%s\"): Failed to load dll", __FUNCTION__, lpDllName);
		return 0;
	}
	module_p last;
	for (module_p tmp = modules; tmp != 0; tmp = tmp->next) {
		if (hModule == tmp->hModule) return 0; /*tmp*/ // no dupes
		last = tmp;
	}
	if ((tmp = new module_t(hModule)) == 0) {
		_RPTF2(_CRT_ERROR, "%s(...): failed to allocate new item (%s)", __FUNCTION__, "module_t");
		return 0;
	}
	printf("loading library %s...", lpDllName);
	tmp->doshdr = GetDosHdr(hModule);
	tmp->pehdr = GetPeHdr(hModule);
	if (tmp->dwSize == 0) {
		MODULEINFO ModInfo;
		if (GetModuleInformation(NULL, hModule, &ModInfo, sizeof ModInfo) != 0)
			tmp->dwSize = ModInfo.SizeOfImage;
		else if (tmp->pehdr != 0)
			tmp->dwSize = tmp->pehdr->OptionalHeader.SizeOfImage;
	}
	if (GetModuleFileName(hModule, tmp->FileName, sizeof tmp->FileName) == 0)
		strcpy(tmp->FileName, lpDllName);
	if (tmp->hasName())
		if ((tmp->lpBaseName = strrchr(tmp->FileName, '\\')) != 0)
			tmp->lpBaseName++;
		else
			tmp->lpBaseName = tmp->FileName;
	else { // !hasName()
		tmp->lpBaseName = 0;
		_RPTF1(_CRT_WARN, "[CBplManager] Failed to get module filename for %08X anyway",
			hModule);
	}
	if (tmp->pehdr != 0) {
		module_t::section_p section, lastsec = 0;
		for (WORD index = 0; index < tmp->pehdr->FileHeader.NumberOfSections; index++) {
			if ((section = new module_t::section_t) == 0) {
				_RPTF2(_CRT_ERROR, "%s(...): failed to allocate new item (%s)", __FUNCTION__, "module_t::section_t");
				continue;
			}
			section->header = (PIMAGE_SECTION_HEADER)(tmp->pehdr + 1) + index;
			section->lpBaseAddress = RVA2EA(section->header->VirtualAddress);
			(tmp->sections == 0 ? tmp->sections : lastsec->next) = section;
			lastsec = section;
		}
		size_t sz;
		if (_expdir.VirtualAddress != 0 && _expdir.Size != 0) {
			PIMAGE_EXPORT_DIRECTORY expdir = (PIMAGE_EXPORT_DIRECTORY)RVA2EA(_expdir.VirtualAddress);
			module_t::export_p lastexport = 0;
			for (DWORD iter = 0; iter < expdir->NumberOfNames; iter++) {
				LPCSTR const *namerva;
				if (!_ExpByOrd) { // by name
					namerva = &_NameRVA(iter);
					if (!_IsInExpBuf(namerva) || *namerva == 0 || !_IsInExpDir(*namerva)) {
						_RPTF3(_CRT_ASSERT, "_IsInExpBuf(&_NameRVA(0x%X)) && _NameRVA(0x%X) != 0 && _IsInExpDir(_NameRVA(0x%X))",
							iter, iter, iter);
						continue;
					}
				}
				WORD ordinal = expdir->Base;
				if (expdir->AddressOfNameOrdinals != 0
					&& expdir->AddressOfNameOrdinals != expdir->AddressOfNames) {
					const WORD *ord = &_Ordinal(iter);
					if (!_IsInExpBuf(ord)) {
						_RPTF1(_CRT_ASSERT, "_IsInExpBuf(&_Ordinal(0x%X))", iter);
						continue;
					}
					ordinal = *ord;
				} else
					ordinal = iter;
				const DWORD *funcrva = &_FuncRVA(ordinal);
				if (!_IsInExpBuf(funcrva)) {
					_RPTF1(_CRT_ASSERT, "_IsInExpBuf(&_FuncRVA(0x%X))", ordinal);
					continue;
				}
				module_t::export_p export = new module_t::export_t;
				if (export == 0) {
					_RPTF2(_CRT_ERROR, "%s(...): failed to allocate new item (%s)", __FUNCTION__, "module_t::export_t");
					continue;
				}
				try {
					if (expdir->Name != 0) export->lpDllName = (char *)(_ExpBufAddr(expdir->Name));
					char redirect[0x200], *name;
					if (_IsInExpDir(*funcrva) && (name = strrchr(strncpy(redirect,
						(char *)_ExpBufAddr(*funcrva), sizeof redirect), '.')) != 0) { // this is redirect
						*name++ = 0;
						module_p module = FindModule(redirect);
						module_t::export_p export2;
						if (module != 0 && (export2 = module->FindExport(name)) != 0)
							export->lpFunc = export2->lpFunc;
						else { // ;-))
							HMODULE hLib = LoadLibrary(redirect);
							export->lpFunc = GetProcAddress(hLib, name);
#ifdef _DEBUG
							_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "[CBplManager] %s(...): Using unsafe redirect lookup method for `%s.%s` module=%s LoadLibrary(\"%s\")=%08X GetProcAddress(\"%s\")=%08X",
								__FUNCTION__, redirect, name, module != 0 ? module->hasName() ?
								module->lpBaseName : "<unnamed>" : "<NULL>", redirect, hLib, name, export->lpFunc);
#endif // _DEBUG
							FreeLibrary(hLib);
						}
					} else // normal export
						export->lpFunc = RVA2EA(*funcrva);
					if (!_ExpByOrd) { // by name
						char *lpExportName = (char *)_ExpBufAddr(*namerva);
						_ASSERTE(*lpExportName != 0);
						export->lpName = lpExportName;
					}
					export->Ordinal = expdir->Base + ordinal;
					(lastexport == 0 ? tmp->exports : lastexport->next) = export;
					lastexport = export;
				} catch (...) {
					delete export;
					(lastexport == 0 ? tmp->exports : lastexport->next) = 0;
					_ASSERTE(_IsInExpDir(expdir->Name));
					_RPTF3(_CRT_ERROR, "%s(...): Failed to add export: module=%s index=0x%X",
						__FUNCTION__, (char *)_ExpBufAddr(expdir->Name), iter);
				}
			} // iterate
			if (expdir->NumberOfNames != expdir->NumberOfFunctions) {
#ifdef _DEBUG
				if (expdir->NumberOfNames > expdir->NumberOfFunctions)
					_RPTF2(_CRT_WARN, "[CBplManager] expdir->NumberOfNames > expdir->NumberOfFunctions (%u!=%u)",
						expdir->NumberOfNames, expdir->NumberOfFunctions);
#endif // _DEBUG
				for (iter = 0; iter < expdir->NumberOfFunctions; iter++) {
					const DWORD *funcrva = &_FuncRVA(iter);
					if(!_IsInExpBuf(funcrva)) {
						_RPTF1(_CRT_ASSERT, "_IsInExpBuf(&_FuncRVA(0x%X))", iter);
						continue;
					}
					for (module_t::export_p export = tmp->exports; export != 0; export = export->next)
						if (export->lpFunc == RVA2EA(*funcrva)) break;
					if (export != 0) continue; // no dupes
					if ((export = new module_t::export_t) == 0) {
						_RPTF2(_CRT_ERROR, "%s(...): failed to allocate new item (%s)", __FUNCTION__, "module_t::export_t");
						continue;
					}
					try {
						export->dwRVA = *funcrva;
						export->lpFunc = RVA2EA(*funcrva);
						export->Ordinal = expdir->Base + iter;
						(lastexport == 0 ? tmp->exports : lastexport->next) = export;
						lastexport = export;
					} catch (...) {
						delete export;
						(lastexport == 0 ? tmp->exports : lastexport->next) = 0;
						_ASSERTE(_IsInExpDir(expdir->Name));
						_RPTF2(_CRT_ASSERT, "Failed to add noname export: module=%s index=0x%X",
							(char *)_ExpBufAddr(expdir->Name), iter);
					}
				} // iterate
			} // NumberOfNames != NumberOfFunctions
		} // have exports
	} // have PE header
	printf("done: %u exports\n", tmp->GetExportCount());
	return (modules == 0 ? modules : last->next) = tmp;
}

#undef _expdir
#undef _ExpBufAddr
#undef _IsInExpBuf
#undef _IsInExpDir
#undef _ExportItem
#undef _Ordinal
#undef _NameRVA
#undef _FuncRVA
#undef _ExpByOrd
#undef RVA2EA

bool CBplManager::RemoveModule(HMODULE const hModule) {
	module_p last = 0;
	for (module_p tmp = modules; tmp != 0; tmp = tmp->next) {
		if (hModule == tmp->hModule) break;
		last = tmp;
	}
	if (tmp == 0) return false; // no dll loaded at that address
	(last != 0 ? last->next : modules) = tmp->next;
	delete tmp;
	return true;
}

bool CBplManager::RemoveModule(module_p const module) {
	module_p last = 0;
	for (module_p tmp = modules; tmp != 0; tmp = tmp->next) {
		if (module == tmp) break;
		last = tmp;
	}
	if (tmp == 0) return false; // no dll loaded at that address
	(last != 0 ? last->next : modules) = tmp->next;
	delete tmp;
	return true;
}

CBplManager::module_p CBplManager::FindModule(LPVOID const lpAddress, BOOL const bExactMatch) {
	if (lpAddress != 0)
		for (module_p tmp = modules; tmp != 0; tmp = tmp->next)
			if (lpAddress == tmp->lpBaseOfImage || !bExactMatch
				&& lpAddress > tmp->lpBaseOfImage && (char *)lpAddress <
				(char *)tmp->lpBaseOfImage + tmp->dwSize) return tmp;
	return 0;
}

CBplManager::module_p CBplManager::FindModule(LPSTR const lpModuleName) {
	if (lpModuleName != 0 && *lpModuleName != 0)
		for (module_p tmp = modules; tmp != 0; tmp = tmp->next)
			if (tmp->hasName()) {
				char fname[_MAX_FNAME];
				_splitpath(tmp->FileName, 0, 0, fname, 0);
				if (stricmp(lpModuleName, tmp->FileName) == 0
					|| stricmp(lpModuleName, tmp->lpBaseName) == 0
					|| stricmp(lpModuleName, fname) == 0) return tmp;
			}
	return 0;
}

CBplManager::module_t::module_t(HMODULE const hModule, SIZE_T const dwSize) :
	hModule(hModule), dwSize(dwSize), lpBaseName(0), sections(0), exports(0),
	next(0) {
	ZeroMemory(&FileName, sizeof FileName);
}

CBplManager::module_t::~module_t() {
	section_p tmp1;
	while ((tmp1 = sections) != 0) {
		sections = sections->next;
		delete tmp1;
	}
	export_p tmp2;
	while ((tmp2 = exports) != 0) {
		exports = exports->next;
		delete tmp2;
	}
	if (hModule != NULL) FreeLibrary(hModule);
}

CBplManager::module_t::export_p CBplManager::module_t::FindExport(LPVOID const lpAddress) {
	if (lpAddress != 0)
		for (export_p export = exports; export != 0; export = export->next)
			if (lpAddress == export->lpFunc) return export;
	return 0;
}

CBplManager::module_t::export_p CBplManager::module_t::FindExport(LPSTR const lpName) {
	_ASSERTE(lpName != 0 && *lpName != 0);
	if (lpName != 0 && *lpName != 0)
		if (*lpName == '#') // by ord
			return FindExport(strtoul(lpName + 1, 0, 10));
		else
			for (export_p export = exports; export != 0; export = export->next)
				if (strcmp(lpName, export->lpName) == 0) return export;
	return 0;
}

CBplManager::module_t::export_p CBplManager::module_t::FindExport(WORD const Ordinal) {
	for (export_p export = exports; export != 0; export = export->next)
		if (export->Ordinal != 0 && Ordinal == export->Ordinal) return export;
	return 0;
}

CBplManager::module_t::section_p CBplManager::module_t::FindSection(
	LPCVOID lpAddress) {
	for (section_p section = sections; section != 0; section = section->next)
		if (lpAddress > section->lpBaseAddress && (char *)lpAddress <
			(char *)section->lpBaseAddress + section->header->Misc.VirtualSize) break;
	return section;
}

LPSTR CBplManager::module_t::section_t::getName(LPSTR const Name) {
	if (Name != 0) {
		strncpy((char *)Name, (char *)header->Name, 9);
		for (int i = 7; i >= 0 && Name[i] == ' '; i--) Name[i] = 0;
	}
	return Name;
}

PIMAGE_DOS_HEADER CBplManager::GetDosHdr(HMODULE const hModule) {
	return (hModule != 0 && ((PIMAGE_DOS_HEADER)hModule)->e_magic == IMAGE_DOS_SIGNATURE)
		? (PIMAGE_DOS_HEADER)hModule : NULL;
}

PIMAGE_NT_HEADERS CBplManager::GetPeHdr(HMODULE const hModule) {
	PIMAGE_DOS_HEADER DosHdr = GetDosHdr(hModule);
	if (DosHdr != 0) {
		PIMAGE_NT_HEADERS PeHdr = (PIMAGE_NT_HEADERS)((LPBYTE)hModule + DosHdr->e_lfanew);
		if (PeHdr->Signature == IMAGE_NT_SIGNATURE
			&& PeHdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC) return PeHdr;
	}
	return 0;
}
