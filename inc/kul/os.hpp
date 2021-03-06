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
#ifndef _KUL_OS_HPP_
#define _KUL_OS_HPP_

#include <vector>

#include "kul/cpu.hpp"
#include "kul/os.os.hpp"
#include "kul/except.hpp"
#include "kul/string.hpp"

#include <fstream>
#include <iostream>

namespace kul{

class Dir;
class File;

namespace fs {
class Exception : public kul::Exception{
	public:
		Exception(const char*f, const int l, const std::string& s) : kul::Exception(f, l, s){}
};

class TimeStamps{
	private:
		const uint a, c, m;
		TimeStamps(const uint& a, const uint& c, const uint& m) : a(a), c(c), m(m){}
	public:
		const uint& accessed() const { return a; }
		const uint& created () const { return c; }
		const uint& modified() const { return m; }
		friend class kul::Dir;
		friend class kul::File;
};

class Item{
	public:
		virtual const fs::TimeStamps timeStamps() const = 0;
		virtual const std::string  real() const = 0;
		virtual const std::string  mini() const = 0;
};
} // END NAMESPACE fs

namespace env{
inline const std::string CWD(){
#ifdef _WIN32
	char c[_MAX_PATH];
	getcwd(c, _MAX_PATH);
#else
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
	char c[PATH_MAX];
	getcwd(c, PATH_MAX);
#endif
    std::string str(c);
    return str;
}
bool CWD(const std::string& c);
bool CWD(const Dir& d);
#ifdef _WIN32
inline const char* GET(const char* c){
	return getenv(c);
}
inline void SET(const char* var, const char* val){
	putenv(std::string(std::string(var) + "=" + std::string(val)).c_str());
}
inline const char SEP(){
	return ';';
}
#else
inline const char* GET(const char* c){
	return getenv(c);
}
inline void SET(const char* var, const char* val){
	setenv(var, val, 1);
}
inline const char SEP(){
	return ':';
}
#endif
} // END NAMESPACE env

class Dir : public fs::Item {
	private:
		std::string p;

		static const fs::TimeStamps TIMESTAMPS(const std::string& s){ 
			uint a = 0, c = 0, m = 0;
			fs::KulTimeStampsResolver::GET(s.c_str(), a, c, m);
			return fs::TimeStamps(a, c, m);
		}
		static const std::string LOCL(std::string s){
#ifdef _WIN32
			kul::String::replaceAll(s, "/", "\\");
#else
			kul::String::replaceAll(s, "\\", "/");
#endif
			return s;
		}
		static const std::string PRNT(const std::string& s){
			const std::string& p = s.substr(0, s.rfind(SEP()) + 1);
			return kul::Dir(p).root() ? p : s.substr(0, s.rfind(SEP()));
		}
		static const std::string MINI(const std::string& a){
			return a.find(env::CWD()) == std::string::npos ? a 
				: a.compare(env::CWD()) == 0 ? a.substr(std::string(env::CWD()).size()) 
				: a.substr(std::string(env::CWD()).size() + 1);
		}
	public:
		Dir(){}
		Dir(const std::string& p, bool m = false) throw(fs::Exception) : p(Dir::LOCL(p)) {
#ifndef _WIN32
			if(p.size() && p[0] == '~') this->p = (env::GET("HOME") + p.substr(1));
#endif
			if(m && !is() && !mk()) KEXCEPT(fs::Exception, "Invalid directory path provided");
		}
		Dir(const Dir& d) : p(d.p){}
		Dir(const std::string& s, const Dir& d) : p(d.join(s)){}

		bool cp(const Dir& d) const;
		bool mv(const Dir& d) const{
			return std::rename(this->path().c_str(), d.path().c_str());
		}
		void rm() const;
#ifdef _WIN32
		bool is() const{
			if(path().empty()) return false;
			DWORD ftyp = GetFileAttributesA(path().c_str());
			return (ftyp != INVALID_FILE_ATTRIBUTES && ftyp & FILE_ATTRIBUTE_DIRECTORY);
		}
		bool mk() const{
			if(path().empty()) return false;
			if(!parent().is()) parent().mk();
			return CreateDirectory(locl().c_str(), NULL);
		}
		bool root() const{
			return is() && real().size() == 3;
		}
#else
		bool is() const{
			if(path().empty()) return false;
			DIR * d = opendir(path().c_str());
			if(d) closedir(d);
			return d;
		}
		bool mk() const{
			if(path().empty()) return false;
			if(!parent().is()) parent().mk();
			return mkdir(locl().c_str(), 0777) == 0;
		}
		bool root() const{
			return is() && real().size() == 1;
		}
#endif

		const std::string join(const std::string& s) const{ 
			return p.size() == 0 ? s : root() ? path() + s : JOIN(path(), s); 
		}
		const std::string name() const{
			return root() ? path() : path().substr(path().rfind(SEP()) + 1);
		}
		const std::string  locl() const { return LOCL(path()); }
		const std::string& path() const { return p;}
		const std::string  real() const { return REAL(path()); }
		const std::string  mini() const { return MINI(real()); }
		const fs::TimeStamps timeStamps() const { return TIMESTAMPS(p); }

		const Dir parent() const { return Dir(PRNT(path())); }

		const std::vector<Dir> 	dirs(bool incHidden = false) const throw(fs::Exception){
			if(!is()) KEXCEPT(fs::Exception, "Directory : \"" + path() + "\" does not exist");
			std::vector<Dir> dirs;
#ifdef _WIN32
			WIN32_FIND_DATA fdFile;
			HANDLE hFind = NULL;
			char sPath[2048];
			sprintf(sPath, "%s\\*.*", path().c_str());
			if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
				KEXCEPT(fs::Exception, "Directory : \"" + path() + "\" does not exist");
			do{
				if(strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0){
					sprintf(sPath, "%s\\%s", path().c_str(), fdFile.cFileName);
					if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY){
						if(!incHidden && std::string(sPath).substr(std::string(sPath).rfind(kul::Dir::SEP()) + 1).substr(0, 1).compare(".") == 0) continue; 
						dirs.push_back(Dir(sPath));
					}
				}
			}while(FindNextFile(hFind, &fdFile));
			FindClose(hFind);
#else
			DIR *dir = opendir(real().c_str());
			struct dirent *entry = readdir(dir);
			while (entry != NULL){
				std::string d(entry->d_name);
				kul::Dir dd(JOIN(real(), entry->d_name));
				if(d.compare(".") != 0 && d.compare("..") != 0
					&& !(d.substr(0, 1).compare(".") == 0 && !incHidden)
					&& dd.is())
					dirs.push_back(dd);
				entry = readdir(dir);
			}
			closedir(dir);
#endif
			return dirs;
		}
		const std::vector<File> files(bool recursive = false) const throw(fs::Exception);

		static const std::string JOIN(const std::string& a, const std::string& b) { return a + SEP() + b; }
#ifdef _WIN32
		static const std::string REAL(const std::string& s) throw(fs::Exception){
			char* expanded = _fullpath(NULL, s.c_str(), _MAX_PATH);
			if(expanded){
				std::string dir(expanded);
				delete expanded;
				return dir;
			}
			KEXCEPT(fs::Exception, "Item: \"" + s + "\" does not exist");
		}
		static const std::string SEP(){
			return std::string("\\");
		}
#else
		static const std::string REAL(const std::string& s) throw(fs::Exception){
			char* expanded = realpath(s.c_str() , NULL);
			if(expanded){
				std::string dir(expanded);
				free(expanded);
				return dir;
			}
			KEXCEPT(fs::Exception, "Directory \"" + s + "\" does not exist");
		}
		static const std::string SEP(){
			return std::string("/");
		}
#endif
		friend class File;

		Dir& operator=(const Dir& d){ this->p = d.p; return *this; } 
		bool operator==(const Dir& d) const {
			if(is() && d.is()) return real().compare(d.real()) == 0;
			return path().compare(d.path()) == 0;
		}
		explicit operator bool() const { return is(); }
};

inline std::ostream& operator<<(std::ostream &s, const Dir& d){
	return s << d.path();
}

class File : public fs::Item {
	private:
		std::string n;
		Dir d;
	public:
		File(){}
		File(const std::string& n, bool m = false) : n(Dir::LOCL(n)){
			if(n.find(Dir::SEP()) != std::string::npos){
				this->d = Dir(n.substr(0, n.rfind(Dir::SEP())));
				this->n = this->n.substr(n.rfind(Dir::SEP()) + 1);
			}else
				try{
					d = Dir(Dir::PRNT(Dir::REAL(this->n)), m);
				}catch(const kul::fs::Exception& e){
					this->d = Dir(env::CWD());
				}
			if(this->n.find(d.path()) != std::string::npos)
				this->n = this->n.substr(d.path().size() + 1);
		}
		File(const std::string& n, const Dir& d) : n(n), d(d){}
		File(const std::string& n, const char* c) : n(n), d(c){}
		File(const std::string& n, const std::string& d1) : n(n), d(d1){}
		File(const File& f) : n(f.n), d(f.d){}

		bool cp(const Dir& f) const{
			if(!d.is() && !d.mk()) KEXCEPT(fs::Exception, "Directory: \"" + d.path() + "\" is not valid");
			return cp(kul::File(name(), d));
		}
		bool cp(const File& f) const{
			std::ifstream src(d.join(n), std::ios::binary);
			std::ofstream dst(f.dir().join(f.name()), std::ios::binary);
			return (bool) (dst << src.rdbuf());
		}
#ifdef _WIN32
		bool is() const{
			return !name().empty() && (bool) std::ifstream(d.join(n).c_str());
		}
		bool rm() const{
			if(is()){
				unlink(d.join(n).c_str());
				return true;
			}
			return false;
		}
#else
		bool is() const{
			if(name().empty()) return false;
			struct stat buffer;
			return (stat (d.join(n).c_str(), &buffer) == 0);
		}
		bool rm() const{
			if(is()){
				remove(real().c_str());
				return true;
			}
			return false;
		}
#endif
		bool mk() const{
			FILE* pFile;
			pFile = fopen(full().c_str(),"w");
			if(pFile != NULL){
				fclose(pFile);
			}
			return pFile != NULL;
		}
		bool mv(const File& f) const{
			return std::rename(this->full().c_str(), f.full().c_str());
		}

		const std::string& name() const { return n; }

		const std::string full() const { return Dir::JOIN(d.path(), n); }
		const std::string real() const { return Dir::JOIN(d.real(), n); }
		const std::string mini() const { return Dir::MINI(real()); }
		const ulonglong   size() const{
			ulonglong r = 0;
#ifdef _WIN32
			WIN32_FIND_DATA ffd;
			HANDLE h = FindFirstFile(TEXT(mini().c_str()), &ffd);
			if(h){
				ULARGE_INTEGER ul;
				ul.HighPart = ffd.nFileSizeHigh;
				ul.LowPart = ffd.nFileSizeLow;
				r = ul.QuadPart;
				FindClose(h);
			}
#else
			struct stat att;
			if (stat(mini().c_str(), &att) != -1) r = att.st_size;
#endif
			return r; 
		}
		const Dir& dir() const { return d; }
		const fs::TimeStamps timeStamps() const { return Dir::TIMESTAMPS(mini()); }

		File& operator=(const File& f) = default;
		bool operator==(const File& f) const {
			if(is() && f.is()) return real().compare(f.real()) == 0;
			return full().compare(f.full()) == 0;
		}
		explicit operator bool() const { return is(); }
};

inline bool kul::Dir::cp(const Dir& d) const{
	if(!d.is() && !d.mk()) KEXCEPT(fs::Exception, "Directory: \"" + d.path() + "\" is not valid");
	Dir c(d.join(name()));
	c.mk();
	for(const auto& f : files()) f.cp(c);
	for(const auto& dd : dirs()) dd.cp(c);
	return 1;
}

inline std::ostream& operator<<(std::ostream &s, const File& d){
	return s << d.full();
}

namespace env{
inline bool WHICH(const char* c){
	for(const auto& s : kul::String::split(std::string(env::GET("PATH")), kul::env::SEP())){
		const kul::Dir d(s);
		if(d)
			for(const auto& f : d.files())
				if(f.name().compare(c) == 0) return 1;
	}
	return false;
}
} // END NAMESPACE env


#ifdef _WIN32
namespace os{
inline int exec(const std::string& cmd, bool q = false){
	if(q){
		return system(std::string(cmd + " > nul").c_str());
	}else return system(cmd.c_str());
}
inline int exec(const std::string& cmd, char*const args[]){
	return execvp(cmd.c_str(), args);
}
inline const kul::Dir userDir(){
	const char* h = env::GET("HOME");
	if(h) return kul::Dir(h);
	return kul::Dir(std::string(env::GET("HOMEDRIVE")) + std::string(env::GET("HOMEPATH")));
}
inline const kul::Dir userAppDir(const std::string& app){
	return kul::Dir(userDir().join(app));
}
inline const std::string EOL(){
	#if (_MSC_VER >= 1800 )
	return "\n";
	#else
	return "\r\n";
	#endif
}
} // END NAMESPACE os
#else
namespace os{
inline int exec(const std::string& cmd, bool q = false){
	if(q){
		return system(std::string(cmd + " > /dev/null").c_str());
	}else return system(cmd.c_str());
	return system(cmd.c_str());
}
inline int exec(const std::string& cmd, char*const args[]){
	return execvp(cmd.c_str(), args);
}
inline const kul::Dir userDir(){
	return Dir(env::GET("HOME"));
}
inline const kul::Dir userAppDir(const std::string& app){
	return Dir(Dir::JOIN(env::GET("HOME"), "." + app));
}
inline const std::string EOL(){
	return "\n";
}
} // END NAMESPACE os
#endif
} // END NAMESPACE kul

#ifdef _WIN32

inline bool kul::env::CWD(const std::string& c){
	return _chdir(c.c_str());
}
inline bool kul::env::CWD(const kul::Dir& d){
	return _chdir(d.path().c_str());
}
inline void kul::Dir::rm() const{
	if(is()){
		for(const auto& a : files()) a.rm();
		for(const auto& a : dirs())  a.rm();
		_rmdir(path().c_str());
	}
}
inline const std::vector<kul::File> kul::Dir::files(bool recursive) const throw(fs::Exception){
	if(!is()) KEXCEPT(fs::Exception, "Directory : \"" + path() + "\" does not exist");

	std::vector<File> fs;
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;
	char sPath[2048];
	sprintf(sPath, "%s\\*.*", path().c_str());
	if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) 
		KEXCEPT(fs::Exception, "Directory : \"" + path() + "\" does not exist");

	do{
		if(strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0){
			sprintf(sPath, "%s\\%s", path().c_str(), fdFile.cFileName);
			if(!(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)){
				std::string f(sPath);
				fs.push_back(File(f.substr(f.rfind(kul::Dir::SEP()) + 1), *this));
			}
		}
	}while(FindNextFile(hFind, &fdFile));
	FindClose(hFind);
	if(recursive){
		for(const Dir& d : dirs()){
			std::vector<File> tFiles = d.files(true);
			fs.insert(fs.end(), tFiles.begin(), tFiles.end());
		}
	}
	return fs;
}
#else
inline bool kul::env::CWD(const std::string& c){
	return chdir(c.c_str());
}
inline bool kul::env::CWD(const kul::Dir& d){
	return chdir(d.path().c_str());
}
inline void kul::Dir::rm() const{
	if(is()){
		for(const auto& a : files()) a.rm();
		for(const auto& a : dirs())  a.rm();
		remove(real().c_str());
	}
}
inline const std::vector<kul::File> kul::Dir::files(bool recursive) const throw(fs::Exception){
	if(!is()) KEXCEPT(fs::Exception, "Directory : \"" + path() + "\" does not exist");

	std::vector<File> fs;
	DIR *dir = opendir(path().c_str());
	struct dirent *entry = readdir(dir);
	while (entry != NULL){
		if(!kul::Dir(JOIN(real(), entry->d_name)).is())
			fs.push_back(File(entry->d_name, *this));
		entry = readdir(dir);
	}
	closedir(dir);
	if(recursive){
		for(const kul::Dir& d : dirs()){
			const std::vector<kul::File>& tFs = d.files(true);
			fs.insert(fs.end(), tFs.begin(), tFs.end());
		}
	}
	return fs;
}
#endif


#endif /* _KUL_OS_HPP_ */
