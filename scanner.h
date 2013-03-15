/*
 *	botc source code
 *	Copyright (C) 2012 Santeri `azimuth` Piippo
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	
 *	1. Redistributions of source code must retain the above copyright notice,
 *	   this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright notice,
 *	   this list of conditions and the following disclaimer in the documentation
 *	   and/or other materials provided with the distribution.
 *	3. Neither the name of the developer nor the names of its contributors may
 *	   be used to endorse or promote products derived from this software without
 *	   specific prior written permission.
 *	4. Redistributions in any form must be accompanied by information on how to
 *	   obtain complete source code for the software and any accompanying
 *	   software that uses the software. The source code must either be included
 *	   in the distribution or be available for no more than the cost of
 *	   distribution plus a nominal fee, and must be freely redistributable
 *	   under reasonable conditions. For an executable file, complete source
 *	   code means the source code for all modules it contains. It does not
 *	   include source code for modules or files that typically accompany the
 *	   major components of the operating system on which the executable file
 *	   runs.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SCRIPTREADER_H__
#define __SCRIPTREADER_H__

#include <stdio.h>
#include "str.h"

#define MAX_FILESTACK 8
#define MAX_SCOPE 32
#define MAX_CASE 64

#define PERFORM_FORMAT(in, out) \
	va_list v; \
	va_start (v, in); \
	char* out = vdynformat (in, v, 256); \
	va_end (v);

class Scanner {
public:
	// ====================================================================
	// MEMBERS
	FILE* fp[MAX_FILESTACK];
	char* saFilePath[MAX_FILESTACK];
	int fc;
	
	ulong ulaPosition[MAX_FILESTACK];
	ulong ulaLineNumber[MAX_FILESTACK];
	ulong ulaCurChar[MAX_FILESTACK];
	long laSavedPos[MAX_FILESTACK]; // filepointer cursor position
	str token;
	short dCommentMode;
	long lPrevPos;
	str zPrevToken;
	
	// ====================================================================
	// METHODS
	Scanner (str path);
	~Scanner ();
	void OpenFile (str path);
	void CloseFile (unsigned int u = MAX_FILESTACK);
	char ReadChar ();
	char PeekChar (int offset = 0);
	bool Next (bool peek = false);
	void Prev ();
	str PeekNext (int offset = 0);
	void Seek (unsigned int n, int origin);
	void MustNext (const char* c = "");
	void MustThis (const char* c);
	void MustString (bool gotquote = false);
	void MustNumber (bool fromthis = false);
	void MustBool ();
	bool BoolValue ();
	
	void ParserError (const char* message, ...);
	void ParserWarning (const char* message, ...);
	
private:
	bool bAtNewLine;
	char c;
	void ParserMessage (const char* header, char* message);
};

#endif // __SCRIPTREADER_H__