/**
Copyright (c) 2013, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _KUL_SIGNAL_HPP_
#define _KUL_SIGNAL_HPP_

#include "kul/proc.hpp"

#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

void kul_real_se_handler(EXCEPTION_POINTERS* pExceptionInfo );

LONG WINAPI kul_top_level_exception_handler(PEXCEPTION_POINTERS pExceptionInfo){
	kul_real_se_handler(pExceptionInfo);
	return (LONG) 0L;
}

void kul_se_translator_function(unsigned int sig, EXCEPTION_POINTERS* pExceptionInfo ){
	kul_real_se_handler(pExceptionInfo);
}

namespace kul{
class Signal;

class SignalStatic{
	private:
		bool q = 0;
		std::vector<std::function<void(int)>> ab, in, se;
		friend class Signal;
		friend void ::kul_real_se_handler(EXCEPTION_POINTERS* pExceptionInfo);
		static SignalStatic& INSTANCE(){
			static SignalStatic ss;
			return ss;
		}
};

class Signal{
	private:
		bool q = 0;
		std::vector<std::function<void(int)>> ab, in, se;
		friend class Signal;
		friend void ::kul_real_se_handler(EXCEPTION_POINTERS* pExceptionInfo);
	public:
		Signal(){
			_set_se_translator( kul_se_translator_function );
			SetUnhandledExceptionFilter(kul_top_level_exception_handler);
		}
		void quiet() { kul::SignalStatic::INSTANCE().q = 1; }
		void abrt(const std::function<void(int)>& f){ kul::SignalStatic::INSTANCE().ab.push_back(f); }
		void intr(const std::function<void(int)>& f){ kul::SignalStatic::INSTANCE().in.push_back(f); }
		void segv(const std::function<void(int)>& f){ kul::SignalStatic::INSTANCE().se.push_back(f); }
};
}

void kul_real_se_handler(EXCEPTION_POINTERS* pExceptionInfo){
	const std::string& tid(kul::this_thread::id());
	uint sig = pExceptionInfo->ExceptionRecord->ExceptionCode;
	if(pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    	for(auto& f : kul::SignalStatic::INSTANCE().se) f(sig = 11);

    if(!kul::SignalStatic::INSTANCE().q){
	    HANDLE process = GetCurrentProcess();
	    SymInitialize(process, NULL, TRUE);

	    CONTEXT context_record = *pExceptionInfo->ContextRecord;

	    STACKFRAME64 stack_frame;
	    memset(&stack_frame, 0, sizeof(stack_frame));
	    #if defined(_WIN64)
	    int machine_type = IMAGE_FILE_MACHINE_AMD64;
	    stack_frame.AddrPC.Offset = context_record.Rip;
	    stack_frame.AddrFrame.Offset = context_record.Rbp;
	    stack_frame.AddrStack.Offset = context_record.Rsp;
	    #else
	    int machine_type = IMAGE_FILE_MACHINE_I386;
	    stack_frame.AddrPC.Offset = context_record.Eip;
	    stack_frame.AddrFrame.Offset = context_record.Ebp;
	    stack_frame.AddrStack.Offset = context_record.Esp;
	    #endif
	    stack_frame.AddrPC.Mode = AddrModeFlat;
	    stack_frame.AddrFrame.Mode = AddrModeFlat;
	    stack_frame.AddrStack.Mode = AddrModeFlat;

	    SYMBOL_INFO* symbol;
	    symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
	    symbol->MaxNameLen   = 255;
	    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	    std::cout << "[bt] Stacktrace:" << std::endl;
	    while (StackWalk64(machine_type,
	        GetCurrentProcess(),
	        GetCurrentThread(),
	        &stack_frame,
	        &context_record,
	        NULL,
	        &SymFunctionTableAccess64,
	        &SymGetModuleBase64,
	        NULL)) {

	        DWORD64 displacement = 0;
	        if (SymFromAddr(process, (DWORD64)stack_frame.AddrPC.Offset, &displacement, symbol)){
	        	DWORD  dwDisplacement;
				IMAGEHLP_LINE64 line;
				IMAGEHLP_MODULE64 moduleInfo;
	            ZeroMemory(&moduleInfo, sizeof(IMAGEHLP_MODULE64));
	            moduleInfo.SizeOfStruct = sizeof(moduleInfo);

	            std::cout << "[bt] ";
	            if (::SymGetModuleInfo64(process, symbol->ModBase, &moduleInfo))
	            	std::cout << moduleInfo.ModuleName << " ";

	            std::cout << symbol->Name << " + [0x" << std::hex << displacement << "]"; 

				if (SymGetLineFromAddr64(process, (DWORD64)stack_frame.AddrPC.Offset, &dwDisplacement, &line))
					std::cout << " - " << line.FileName << ": " << std::to_string(line.LineNumber);
				else
					std::cout << " - ??:";
				std::cout << std::endl;
	        }
	    }
    }
	exit(sig);
}

#endif /* _KUL_SIGNAL_HPP_ */
