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
#ifndef _KUL_IPC_HPP_
#define _KUL_IPC_HPP_

#ifndef _KUL_IPC_UUID_PREFIX_
#define _KUL_IPC_UUID_PREFIX_ "/tmp/pipe"
#endif

#include "kul/log.hpp"
#include "kul/proc.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFSIZE 512

namespace kul{ namespace ipc{

class Exception : public kul::Exception{
	public:
		Exception(const char*f, const int l, const std::string& s) : kul::Exception(f, l, s){}
};

class IPCCall{

	protected:
		int fd;
		void writePID() const{
			std::string s = std::to_string(this_proc::id());
			while(s.size() < 9) s = "0" + s;
			write(fd, s.c_str(), 9);
		}
		void writeLength(const std::string& m) const{
			std::string s = std::to_string(m.size());
			while(s.size() < 3) s = "0" + s;
			write(fd, s.c_str(), 3);
		}
};
class Server : public IPCCall{
	private:
		int lp;
		const kul::File uuid;

		void start() throw(Exception){
			uuid.dir().mk();
			mkfifo(uuid.full().c_str(), 0666);
		}
	protected:
		virtual void handle(const std::string& s){
			KLOG(INF) << s;
		}
		void respond(const std::string& s);
	public:
		virtual ~Server(){}
		void listen() throw(Exception){
			char buff[BUFSIZE];
			while(lp){
				memset(buff, 0, BUFSIZE);
				fd = open(uuid.full().c_str(), O_RDONLY);
				if (fd == -1) KEXCEPT(kul::ipc::Exception, "Cannot open FIFO for read");
				int l;
				read(fd, buff, 3);
				std::istringstream ssl(buff);
				ssl >> l;
				memset(buff, 0, BUFSIZE);
				read(fd, buff, l);
				handle(buff);
				close(fd);
				if(lp != -1) lp--;
			}
		}
		Server(const int& lp = -1) throw(Exception) : lp(lp), uuid(std::to_string(kul::this_proc::id()), Dir(_KUL_IPC_UUID_PREFIX_ + std::string("/pid/"))){ start();}
		Server(const std::string& ui, const int& lp = -1) throw(Exception) : lp(lp), uuid(ui, Dir(_KUL_IPC_UUID_PREFIX_)){ start();}
};

class Client : public IPCCall{
	private:
		bool m;
		const kul::File uuid;

		void start() throw(Exception){
			fd = open(uuid.full().c_str(), O_WRONLY);
			if (fd == -1) KEXCEPT(kul::ipc::Exception, "Cannot contact server");
		}
		void stop() const throw(Exception){
			close(fd);
		}
	public:
		virtual ~Client(){
			stop();
		}
		Client(const std::string& ui) throw(Exception) : m(1), uuid(ui, Dir(_KUL_IPC_UUID_PREFIX_)) { start(); }
		Client(const int& pid) throw(Exception) : m(1), uuid(std::to_string(pid), Dir(_KUL_IPC_UUID_PREFIX_ + std::string("/pid/"))) { start(); }
		virtual void send(const std::string& m) const throw(Exception){
			writeLength(m);
			write(fd, m.c_str(), m.size());
		}

};

}// END NAMESPACE ipc
}// END NAMESPACE kul

#endif /* _KUL_IPC_HPP_ */
