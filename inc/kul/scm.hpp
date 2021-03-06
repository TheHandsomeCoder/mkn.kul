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
#ifndef _KUL_SCM_HPP_
#define _KUL_SCM_HPP_
 
#include "kul/os.hpp"
#include "kul/hash.hpp"
#include "kul/proc.hpp"
#include "kul/string.hpp"

namespace kul{ namespace scm{

class Exception : public kul::Exception{
	public:
		Exception(const char*f, const int l, const std::string& s) : kul::Exception(f, l, s){}
};

class NotFoundException : public kul::Exception{
	public:
		NotFoundException(const char*f, const int l, const std::string& s) : kul::Exception(f, l, s){}
};
}

class SCM{
	protected:
		SCM(){}
	public:
		virtual ~SCM(){}
		const std::string type() { return typeid(*this).name(); }
		virtual const std::string co(const std::string& d, const std::string& r, const std::string& v) const throw(Exception) = 0;
		virtual void up(const std::string& d, const std::string& r, const std::string& v) const throw(Exception) = 0;
		virtual const std::string origin(const std::string& d) const = 0;
		virtual const std::string localVersion(const std::string& d, const std::string& b) const = 0;
		virtual const std::string remoteVersion(const std::string& d, const std::string& url, const std::string& branch) const throw(Exception) = 0;

		virtual bool hasChanges(const std::string& d) const = 0;

		// virtual void setOrigin(const std::string& d, const std::string& r) const = 0;
		virtual void status(const std::string& d) const = 0;
		virtual void diff(const std::string& d) const = 0;
};

namespace scm{
class Git : public SCM{
	public:
		const std::string co(const std::string& d, const std::string& r, const std::string& v) const throw(Exception){
			Dir dr(d, true);
			kul::Process p("git");
			p.arg("clone").arg(r);
			if(!v.empty()) p.arg("-b").arg(v);
			try{
				p.arg(d).start();
			}catch(const kul::proc::ExitException& e){
				dr.rm();
				KEXCEPT(Exception, "SCM ERROR - Check remote dependency location / version");
			}
			return p.toString();
		}
		void up(const std::string& d, const std::string& r, const std::string& v) const throw(Exception){
			if(!Dir(d).is()) co(d, r, v);
			else{
				kul::Process p("git", d);
				p.arg("pull");
				if(!v.empty()) p.arg(r).arg(v);
				try{
					p.start();
				}catch(const kul::proc::ExitException& e){
					KEXCEPT(Exception, "SCM ERROR - Check remote dependency location / version");
				}
			}
		}
		const std::string origin(const std::string& d) const{
			kul::Process p("git", d);
			kul::ProcessCapture pc(p);
			try{
				p.arg("remote").arg("-v").start();
			}catch(const kul::proc::ExitException& e){
				KERR << pc.errs();
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
			if(pc.outs().empty()) KEXCEPT(Exception, "SCM ERROR: Directory may not be git repository : " + d);
			return kul::String::split(kul::String::split(kul::String::lines(pc.outs())[0], "	")[1], " ")[0];
		}
		const std::string localVersion(const std::string& d, const std::string& b) const{
			kul::Process p("git", d);
			kul::ProcessCapture pc(p);
			try{
				p.arg("rev-parse").arg(b).start();
			}catch(const kul::proc::ExitException& e){
				KERR << pc.errs();
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
			if(pc.outs().empty()) KEXCEPT(Exception, "SCM ERROR: Directory may not be git repository : " + d);
			return kul::String::lines(pc.outs())[0];
		}

		const std::string remoteVersion(const std::string& d, const std::string& url, const std::string& b) const throw(Exception){
			kul::Process p("git", d);
			kul::ProcessCapture pc(p);
			try{
				p.arg("ls-remote").arg(url).arg(b).start();
			}catch(const kul::proc::ExitException& e){
				KERR << pc.errs();
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
			if(pc.outs().empty()) KEXCEPT(Exception, "SCM ERROR URL OR BRANCH MAY NOT EXIST: " + url + " / " + b);
			std::string s(kul::String::lines(pc.outs())[0]);
			kul::String::trim(s);
			return s.substr(0, s.find("	"));
		}

		bool hasChanges(const std::string& d) const{
			kul::Process p("git", d);
			kul::ProcessCapture pc(p);
			try{
				p.arg("status").arg("-sb").start();
			}catch(const kul::proc::ExitException& e){
				KERR << pc.errs();
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
			return kul::String::lines(pc.outs()).size() > 1;
		}
		void status(const std::string& d) const{
			kul::Process p("git", d);
			try{
				p.arg("status").start();
			}catch(const kul::proc::ExitException& e){
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
		}
		void diff(const std::string& d) const{
			kul::Process p("git", d);
			try{
				p.arg("diff").start();
			}catch(const kul::proc::ExitException& e){
				KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
			}
		}
		// void setOrigin(const std::string& d, const std::string& r) const{
		// 	kul::Process p("git", d);
		// 	kul::ProcessCapture pc(p);
		// 	try{
		// 		p.arg("remote").arg("set-url").arg(r).start();
		// 	}catch(const kul::proc::ExitException& e){
		// 		KEXCEPT(Exception, "SCM ERROR " + std::string(e.what()));
		// 	}
		// }
};

// SVN URL CHANGE: svn switch –relocate  <from URL> <to URL>
class Svn : public SCM{
	public:
		const std::string co(const std::string& d, const std::string& r, const std::string& v) const throw(Exception){
			Dir dr(d, true);
			kul::Process p("svn", d);
			p.arg("checkout");
			if(v.empty()) p.arg(r);
			else p.arg(r + "/" + v);
			try{
				p.arg(".").start();
			}catch(const kul::proc::ExitException& e){
				dr.rm();
				KEXCEPT(Exception, "SCM ERROR - Check remote dependency location / version");
			}
			return p.toString();
		};
		void up(const std::string& d, const std::string& r, const std::string& v) const throw(Exception){
			if(!Dir(d).is()) co(d, r, v);
			else KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		};
		const std::string origin(const std::string& d) const{
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}
		const std::string localVersion(const std::string& d, const std::string& b) const{
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}
		const std::string remoteVersion(const std::string& d, const std::string& url, const std::string& b) const throw(Exception){
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}

		bool hasChanges(const std::string& d) const{
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}
		void status(const std::string& d) const{
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}
		void diff(const std::string& d) const{
			KEXCEPT(Exception, "SCM ERROR - SVN NOT IMPLEMENTED");
		}
		// void setOrigin(const std::string& d, const std::string& r) const;
};

class Manager{
	private:
		Manager(){
			git = std::make_unique<Git>();
			svn = std::make_unique<Svn>();
			SCMs.insert(std::pair<std::string, SCM*>("git", git.get()));
			SCMs.insert(std::pair<std::string, SCM*>("svn", svn.get()));
		}
		hash::map::S2T<SCM*> SCMs;
		std::unique_ptr<SCM> git;
		std::unique_ptr<SCM> svn;
	public:
		static Manager& INSTANCE(){ 
			static Manager instance; 
			return instance;
		}
		const SCM& get(const std::string& s) throw(NotFoundException){
			if(SCMs.count(s) > 0) return *(*SCMs.find(s)).second;
			KEXCEPT(NotFoundException, "Source Control Management for " + s + " is not implemented");
		}
};

}// END NAMESPACE scm
}// END NAMESPACE kul
#endif /* _KUL_SCM_HPP_ */
