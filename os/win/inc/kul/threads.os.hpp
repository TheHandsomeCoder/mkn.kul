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
#ifndef _KUL_THREADS_OS_HPP_
#define _KUL_THREADS_OS_HPP_

#include "kul/threads.base.hpp"

#include <Windows.h>
#include <TlHelp32.h>

namespace kul{ 
namespace this_thread{
inline const std::string id(){
	std::ostringstream os;
	os << std::hex << std::hash<std::thread::id>()(std::this_thread::get_id());
	return os.str();
}
inline bool main(){
	const std::tr1::shared_ptr<void> hThreadSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);
    if (hThreadSnapshot.get() == INVALID_HANDLE_VALUE) throw std::runtime_error("GetMainThreadId failed");
    THREADENTRY32 tEntry;
    tEntry.dwSize = sizeof(THREADENTRY32);
    DWORD result = 0;
    DWORD currentPID = GetCurrentProcessId();
    for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
        !result && success && GetLastError() != ERROR_NO_MORE_FILES;
        success = Thread32Next(hThreadSnapshot.get(), &tEntry))
        if (tEntry.th32OwnerProcessID == currentPID) result = tEntry.th32ThreadID;

    std::stringstream ss;
    ss << std::this_thread::get_id();
    return std::to_string(result) == ss.str();
}
inline void kill(){

	HANDLE h = GetCurrentThread();
	TerminateThread(h, 0); 
	CloseHandle(h);

}
} // END NAMESPACE this_thread

class Mutex{
	private:
		CRITICAL_SECTION critSec;
	public:
		Mutex(){
			InitializeCriticalSection(&critSec);
		}
		~Mutex() {
			DeleteCriticalSection(&critSec);
		}
		void lock() {
			EnterCriticalSection(&critSec); 
		}
		void unlock() {
			LeaveCriticalSection(&critSec);
		}
};

namespace threading{
DWORD WINAPI threadFunction(LPVOID th);
}

class Thread : public threading::AThread{
	private:
		HANDLE h;
		friend DWORD WINAPI threading::threadFunction(LPVOID);
	public:
		Thread(const std::shared_ptr<threading::ThreadObject>& t) : AThread(t){}
		template <class T> Thread(const T& t) : AThread(t){}
		template <class T> Thread(const Ref<T>& t) : AThread(t){}
		virtual ~Thread(){}
		void join(){
			if(!s) run();
			// if(h){
				WaitForSingleObject(h, INFINITE);
				CloseHandle(h);
			// }
			s = 0;
		}
		bool detach(){ return CloseHandle(h); }
		void interrupt() throw(kul::threading::InterruptionException){
			TerminateThread(h, 1);
			f = 1;
		}
		void run() throw(kul::threading::Exception){
			if(s) KEXCEPTION("Thread running");
			f = 0;
			s = 1;
			h = CreateThread(0, 5120000, threading::threadFunction, this, 0, 0);
		}
};

namespace threading{
inline DWORD WINAPI threadFunction(LPVOID th){
	reinterpret_cast<Thread*>(th)->act(); 	
	return 0;
}
}


}// END NAMESPACE kul
#endif /* _KUL_THREADS_OS_HPP_ */
