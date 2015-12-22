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
#ifndef _KUL_PROC_BASE_HPP_
#define _KUL_PROC_BASE_HPP_

#include <vector>
#include <sstream>
#include <iostream>
#include <functional>

#include "kul/hash.hpp"
#include "kul/except.hpp"

namespace kul { 

namespace this_proc{
	int id();
	void kill(const int& e);
}

namespace proc{

class Exception : public kul::Exception{
	public:
		Exception(const char*f, const int l, const std::string& s) : kul::Exception(f, l, s){}
};

class ExitException : public kul::proc::Exception{
	private:
		const short ec;
	public:
		ExitException(const char*f, const int l, const short ec, const std::string& s) : Exception(f, l, s), ec(ec){}
		const short& code(){ return ec; }
};
}

class AProcess{
	private:
		bool f, s;
		const bool wfe;
		unsigned int pi;
		const std::string d;
		std::function<void(std::string)> e;
		std::function<void(std::string)> o;
		std::vector<std::string> argv;
		kul::hash::map::S2S evs;
		friend std::ostream& operator<<(std::ostream&, const AProcess&);
	protected:
		AProcess(const std::string& cmd, const bool& wfe) : f(0), s(0), wfe(wfe), pi(0) { argv.push_back(cmd); }
		AProcess(const std::string& cmd, const std::string& d, const bool& wfe) : f(0), s(0), wfe(wfe), d(d){ argv.push_back(cmd); }
		virtual ~AProcess(){}

		const std::string&	directory()const { return d; }
		void setFinished()	{ f = 1; }
		virtual bool kill(int k = 6) = 0;
		virtual void preStart() {}
		virtual void finish()	{}
		virtual void tearDown()	{}
		virtual void run() throw (kul::Exception) = 0;
		bool waitForExit()	const { return wfe; }
		void pid(const unsigned int& pi )  { this->pi = pi; }

		const std::vector<std::string>&		args()	const { return argv; };
		const kul::hash::map::S2S& 			vars()	const { return evs; }
		virtual void out(const std::string& s){
			if(this->o) this->o(s);
			else 		printf("%s", s.c_str());
		}
		virtual void err(const std::string& s){
			if(this->e) this->e(s);
			else 		fprintf(stderr, "%s", s.c_str());
		}
		void error(const int line, std::string s) throw (kul::Exception){
			tearDown();
			throw Exception("kul/proc.hpp", line, s);
		}
	public:
		template <class T> AProcess& arg(const T& a) { 
			std::stringstream ss;
			ss << a;
			if(ss.str().size()) argv.push_back(ss.str());
			return *this; 
		}
		AProcess& arg(const std::string& a) { if(a.size()) argv.push_back(a); return *this; }
		AProcess& var(const std::string& n, const std::string& v) { evs.insert(n, v); return *this;}
		virtual void start() throw(kul::Exception){
			if(this->s) KEXCEPT(kul::proc::Exception, "Process is already started");
			this->s = true;
			this->run();
		}
		const unsigned int& pid() 	const { return pi; }
		bool started()		const { return pi > 0; }
		bool finished()		const { return f; }
		virtual const std::string toString() const{
			std::string s;
			for(const std::string& a : args()) s += a + " ";
			s.pop_back();
			return s;
		}
		void setOut(std::function<void(std::string)> o) { this->o = o; }
		void setErr(std::function<void(std::string)> e) { this->e = e; }
};

inline std::ostream& operator<<(std::ostream &s, const AProcess &p){
	return s << p.toString();
}

class ProcessCapture{
	private:
		std::stringstream so;
		std::stringstream se;
	protected:
		ProcessCapture(){}
		ProcessCapture(const ProcessCapture& pc) : so(pc.so.str()), se(pc.se.str()){}
		virtual void out(const std::string& s){
			so << s;
		}
		virtual void err(const std::string& s){
			se << s;
		}
	public:
		ProcessCapture(AProcess& p){
			p.setOut(std::bind(&ProcessCapture::out, std::ref(*this), std::placeholders::_1));
			p.setErr(std::bind(&ProcessCapture::err, std::ref(*this), std::placeholders::_1));
		}
		virtual ~ProcessCapture(){}
		const std::string outs() const { return so.str(); }
		const std::string errs() const { return se.str(); }
};

}
#endif /* _KUL_PROC_BASE_HPP_ */