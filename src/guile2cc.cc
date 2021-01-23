/*
  Convenience templates for more c++-like conversions
    between c++ and guile data,
  not everything from guile/gh.h implemented!
*/

#include"rumor.hh"

#ifdef HAVE_GUILE

#include<stdexcept>

#define SPECIALIZATION_OF_SCM_TO(c_type,scm_conv,scm_pred) \
template<> const c_type SCM_to<c_type>(SCM s){ \
  if (!scm_pred(s)) throw script_error("SCM_to: predicate " #scm_pred " false."); \
  return scm_conv(s);}
SPECIALIZATION_OF_SCM_TO(unsigned long,gh_scm2ulong,gh_number_p)
SPECIALIZATION_OF_SCM_TO(unsigned,gh_scm2ulong,gh_number_p)
SPECIALIZATION_OF_SCM_TO(long,gh_scm2long,gh_number_p)
SPECIALIZATION_OF_SCM_TO(int,gh_scm2int,gh_number_p)
SPECIALIZATION_OF_SCM_TO(double,gh_scm2double,gh_number_p)
SPECIALIZATION_OF_SCM_TO(bool,gh_scm2bool,gh_boolean_p)
SPECIALIZATION_OF_SCM_TO(char,gh_scm2char,gh_char_p)
#undef SPECIALIZATION_OF_SCM_TO

template<> const std::string SCM_to<std::string>(SCM s){
  if (!gh_string_p(s)) throw script_error("SCM_to: predicate " "gh_string_p" " false.");
  char* pstr=gh_scm2newstr(s,NULL);
  std::string ret(pstr);
  delete[] pstr; //FIXME: is this correct?
  return ret;
}
#define SPECIALIZATION_OF_TO_SCM(c_type,scm_conv) \
template<> SCM to_SCM(const c_type& arg){ \
  return scm_conv(arg); }
SPECIALIZATION_OF_TO_SCM(unsigned long,gh_ulong2scm)
SPECIALIZATION_OF_TO_SCM(unsigned,gh_ulong2scm)
SPECIALIZATION_OF_TO_SCM(long,gh_long2scm)
SPECIALIZATION_OF_TO_SCM(int,gh_int2scm)
SPECIALIZATION_OF_TO_SCM(bool,gh_bool2scm)
SPECIALIZATION_OF_TO_SCM(double,gh_double2scm)
SPECIALIZATION_OF_TO_SCM(char,gh_char2scm)
#undef SPECIALIZATION_OF_TO_SCM

template<> SCM to_SCM(const std::string& arg){
  return gh_str02scm(arg.c_str()); }


#endif /*HAVE_GUILE*/
