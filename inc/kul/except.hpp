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
#ifndef _KUL_EXCEPT_HPP_
#define _KUL_EXCEPT_HPP_

#include <sstream>
#include <stdexcept>

#include <kul/def.hpp>

namespace kul{


class Exception : public std::runtime_error{
	protected:
		const char* f;
		const int l;
		const std::exception_ptr ep;
	public:
		~Exception() KNOEXCEPT{}
		Exception(const char*f, const int& l, const std::string& s) : std::runtime_error(s), f(f), l(l), ep(std::current_exception()){}
		Exception(const Exception& e) : std::runtime_error(e.what()), f(e.file()),  l(e.line()), ep(e.ep) {}

		const std::string debug() 			const { return std::string(std::string(f) + " : " + std::to_string(l) + " : " + std::string(what()));}
		const char* file() 					const { return f;}
		const int& line() 					const { return l;}
		const std::exception_ptr& cause() 	const { return ep;}
		const std::string stack()	const {
			std::stringstream ss;
			if(ep){
				try{  								std::rethrow_exception(ep); 	}
				catch(const kul::Exception& e){     ss << e.stack() << std::endl;	}
				catch(const std::exception& e){		ss << e.what() << std::endl;	}
				catch(...){							ss << "UNKNOWN EXCEPTION TYPE" << std::endl; }
			}
			ss << debug();
			return ss.str();
		}
};

class Exit : public Exception{
	private:		
		const int e;
	public:
		Exit(const char*f, const int& l, const std::string& s, const int& e) : Exception(f, l, s), e(e){}
		Exit(const Exit& e) : Exception(*this), e(e.e){}		
		const int& code() const { return e; }
};

#define KEXCEPT(c, m) throw c(__FILE__, __LINE__, m)
#define KEXCEPTION(m) throw Exception(__FILE__, __LINE__, m)

#define KEXIT(e, m) throw kul::Exit(__FILE__, __LINE__, m, e)


}
#endif /* _KUL_EXCEPT_HPP_ */
