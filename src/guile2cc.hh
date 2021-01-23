#ifdef HAVE_GUILE
#include<guile/gh.h>
  
// definitions in guile2cc.cc
template<typename T> const T SCM_to(SCM);
template<typename T> SCM to_SCM(const T&);

class script_error: public std::exception{
  std::string _what;
public:
  script_error(const std::string& s): _what(s){ }
  ~script_error() throw(){ }
  virtual const char* what() const throw(){return _what.c_str();}
};

#endif
