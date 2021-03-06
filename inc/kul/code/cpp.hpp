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
#ifndef _KUL_CODE_CPP_HPP_
#define _KUL_CODE_CPP_HPP_

#include "kul/code/compiler.hpp"

namespace kul{ namespace code{ namespace cpp{ 

class Exception : public kul::Exception{
	public:
		Exception(const char*f, const int l, std::string s) : kul::Exception(f, l, s){}
};

class CCompiler : public Compiler{
	protected:
		CCompiler(const int& v) : Compiler(v){}
	public:
		virtual ~CCompiler(){}
		virtual const std::string cc() 	const = 0;
		virtual const std::string cxx() const = 0;
		virtual const std::string sharedLib(const std::string& lib) const = 0;
		virtual const std::string staticLib(const std::string& lib) const = 0;
		bool sourceIsBin() const{ return false; }
};

class GCCompiler : public CCompiler{
	public:
		GCCompiler(const int& v = 0) : CCompiler(v){}
		const std::string sharedLib(const std::string& lib) const {
			return "lib" + lib + ".so";
		}
		const std::string staticLib(const std::string& lib) const {
			return "lib" + lib + ".a";
		}
		const CompilerProcessCapture buildExecutable(
			const std::string& linker,
			const std::string& linkerEnd,
			const std::vector<std::string>& objects,
			const std::vector<std::string>& libs,
			const std::vector<std::string>& libPaths,
			const std::string& out, 
			const Mode& mode) const throw (kul::Exception) {

			std::string cmd = linker;
			std::vector<std::string> bits;
			if(linker.find(" ") != std::string::npos){
				bits = kul::String::split(linker, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			for(const std::string& path : libPaths)	p.arg("-L" + path);
			if(mode == Mode::STAT) p.arg("-static");
			p.arg("-o").arg(out);
			for(const std::string& o : objects)	p.arg(o);
			for(const std::string& lib : libs)	p.arg("-l" + lib);
			for(const std::string& s: kul::String::split(linkerEnd, ' ')) p.arg(s);
			
			CompilerProcessCapture pc(p);
			try{
				p.start();
			}catch(const kul::proc::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(out);
			pc.cmd(p.toString());
			return pc; 
		}
		const CompilerProcessCapture buildLibrary(
			const std::string& linker, 
			const std::string& linkerEnd,
			const std::vector<std::string>& objects,
			const std::vector<std::string>& libs,
			const std::vector<std::string>& libPaths,
			const kul::File& out, 
			const Mode& mode) const throw (kul::Exception) {

			std::string lib = out.dir().join(sharedLib(out.name()));
			if(mode == Mode::STAT) lib = out.dir().join(staticLib(out.name()));
			std::string cmd = linker;
			std::vector<std::string> bits;
			if(linker.find(" ") != std::string::npos){
				bits = kul::String::split(linker, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			if(mode == Mode::SHAR)		p.arg("-shared");
			p.arg("-o").arg(lib);
			for(const std::string& o : objects)	p.arg(o);
			for(const std::string& s: kul::String::split(linkerEnd, ' ')) p.arg(s);
			CompilerProcessCapture pc(p);
			try{
				p.start();
			}catch(const kul::proc::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(lib);
			pc.cmd(p.toString());
			return pc; 
		}
		const CompilerProcessCapture compileSource(
			const std::string& compiler, 
			const std::vector<std::string>& args, 
			const std::vector<std::string>& incs,
			const std::string& in, 
			const std::string& out, 
			const Mode& mode) const throw (kul::Exception){ 

			std::string cmd = compiler;
			std::vector<std::string> bits;
			if(compiler.find(" ") != std::string::npos){
				bits = kul::String::split(compiler, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			for(const std::string& s : incs) p.arg("-I"+s);
			for(const std::string& s : args) p.arg(s);
			p.arg("-o").arg(out).arg("-c").arg(in);
			CompilerProcessCapture pc(p);
			try{
				p.start();
			}catch(const kul::proc::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(out);
			pc.cmd(p.toString());
			return pc;
		}
		virtual void preCompileHeader(			
			const std::vector<std::string>& incs,
			const hash::set::String& args, 
			const std::string& in, 
			const std::string& out) 	const throw (kul::Exception) {

			using namespace kul;
			
			if(in.rfind(".") == std::string::npos)
				KEXCEPT(Exception, "Unknown header type");
			
			std::string cmd;// = compiler + " -x";
			std::string h = in.substr(in.rfind(".") + 1);

			if(h.compare("h") == 0)
				cmd = cc() + " -x c-header ";
			else
			if(h.compare("hpp") == 0)
				cmd = cxx() + " -x c++-header ";
			else
				KEXCEPT(Exception, "Failed to pre-compile header - uknown file type: " + h);
			cmd += in + " ";
			for(const std::string& s : args)
				cmd += s + " ";
			for(const std::string& s : incs)
				cmd += "-I" + s + " ";

			cmd += " -o " + out;
			if(kul::os::exec(cmd) != 0)
				KEXCEPT(Exception, "Failed to pre-compile header");
		}
		virtual const std::string cc() const {
			return "gcc";
		}
		virtual const std::string cxx() const {
			return "g++";
		}
};

class ClangCompiler : public GCCompiler{
	public:
		ClangCompiler(const int& v = 0) : GCCompiler(v){}
		virtual const std::string cc() const {
			return "clang";
		}
		virtual const std::string cxx() const {
			return "clang++";
		}
};

class IntelCompiler : public GCCompiler{
	public:
		IntelCompiler(const int& v = 0) : GCCompiler(v){}
		virtual const std::string cc() const {
			return "icc";
		}
		virtual const std::string cxx() const {
			return "icpc";
		}
};

class WINCompiler : public CCompiler{
	public:
		WINCompiler(const int& v = 0) : CCompiler(v){}
		virtual const std::string cc() const {
			return "cl";
		}
		virtual const std::string cxx() const {
			return "cl";
		}
		const std::string sharedLib(const std::string& lib) const {
			return lib + ".dll";
		}
		const std::string staticLib(const std::string& lib) const {
			return lib + ".lib";
		}
		const CompilerProcessCapture buildExecutable(
			const std::string& linker, 
			const std::string& linkerEnd,
			const std::vector<std::string>& objects,
			const std::vector<std::string>& libs,
			const std::vector<std::string>& libPaths,
			const std::string& out, 
			const Mode& mode) const throw (kul::Exception){ 

			std::string exe = out + ".exe";

			std::string cmd = linker;
			std::vector<std::string> bits;
			if(linker.find(" ") != std::string::npos){
				bits = kul::String::split(linker, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			p.arg("-OUT:\"" + exe + "\"").arg("-nologo");
			for(const std::string& path : libPaths)	p.arg("-LIBPATH:\"" + path + "\"");
			for(const std::string& o : objects)	p.arg(o);
			for(const std::string& lib : libs) p.arg(staticLib(lib));
			for(const std::string& s: kul::String::split(linkerEnd, ' ')) p.arg(s);

			CompilerProcessCapture pc;
			try{
				p.start();
			}catch(const kul::proc::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(exe);
			pc.cmd(p.toString());
			return pc; 
		}
		const CompilerProcessCapture buildLibrary(
			const std::string& linker, 
			const std::string& linkerEnd,
			const std::vector<std::string>& objects,
			const std::vector<std::string>& libs,
			const std::vector<std::string>& libPaths,
			const kul::File& out, 
			const Mode& mode) const throw (kul::Exception) {

			std::string lib = out.dir().join(sharedLib(out.name()));
			if(mode == Mode::STAT) lib = out.dir().join(staticLib(out.name()));
			std::string cmd = linker;
			std::vector<std::string> bits;
			if(linker.find(" ") != std::string::npos){
				bits = kul::String::split(linker, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			p.arg("-OUT:\"" + lib + "\"").arg("-nologo");
			if(mode == Mode::STAT) p.arg("-LTCG");
			else {
				p.arg("-DLL");
				for(const std::string& path : libPaths)	p.arg("-LIBPATH:\"" + path + "\"");
				for(const std::string& lib : libs) p.arg(staticLib(lib));
			}
			for(const std::string& o : objects)	p.arg(o);
			for(const std::string& s: kul::String::split(linkerEnd, ' ')) p.arg(s);
			CompilerProcessCapture pc;
			try{
				p.start();
			}catch(const kul::proc::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(lib);
			pc.cmd(p.toString());
			return pc; 
		}
		const CompilerProcessCapture compileSource(
			const std::string& compiler, 
			const std::vector<std::string>& args, 
			const std::vector<std::string>& incs,
			const std::string& in, 
			const std::string& out, 
			const Mode& mode) const throw (kul::Exception){ 

			std::string cmd = compiler;
			std::vector<std::string> bits;
			if(compiler.find(" ") != std::string::npos){
				bits = kul::String::split(compiler, ' ');
				cmd = bits[0];
			}
			kul::Process p(cmd);
			for(unsigned int i = 1; i < bits.size(); i++) p.arg(bits[i]);
			p.arg("-nologo");
			for(const std::string& s : incs)	p.arg("-I\"" + s + "\"");
			for(const std::string& s : args)	p.arg(s);
			p.arg("-c").arg("-Fo\"" + out + "\"").arg("\"" + in + "\"");
			CompilerProcessCapture pc;
			if(!kul::LogMan::INSTANCE().inf()) pc.setProcess(p);
			try{
				p.start();
			}catch(const kul::Exception& e){
				pc.exception(std::current_exception());
			}
			pc.tmp(out);
			pc.cmd(p.toString());
			return pc;
		}
		virtual void preCompileHeader(			
			const std::vector<std::string>& incs,
			const hash::set::String& args, 
			const std::string& in, 
			const std::string& out) 	const throw (kul::Exception) {

			KEXCEPTION("Method is not implemented");
		}
};

}}}
#endif /* _KUL_CODE_CPP_HPP_ */
