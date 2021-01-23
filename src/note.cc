/*

 Really Unintelligent Music transcriptOR
 *Note classes utility functions
  
*/


#include"rumor.hh"

bool MidiNote::ReleaseChordPitch(unsigned pitch, long pitch_TU_off){
  //release the pitch;
  //if the note is not in the pitch, ignore it (note overlap)
  //  and return false
  if (pitches.find(pitch)!=pitches.end()) pitches[pitch]=pitch_TU_off;
  else return false;
  //remove all pitches that have already ended before (quantization-wise)
  for(std::map<unsigned,long>::iterator I=pitches.begin(); I!=pitches.end(); I++){
    if((I->second)&&(I->second<pitch_TU_off)) {
      DBGLOG(2,"Removing pitch "<<I->first<<" (off="<<I->second<<") form chord\n")
      pitches.erase(I);
    }
  }
  return true;
}

long MidiNote::TU_off(void){
  if (forced_TU_off>0) return forced_TU_off;
  long ret=0;
  for(std::map<unsigned,long>::iterator I=pitches.begin(); I!=pitches.end(); I++){
    if(I->second==0) return 0;
    if(I->second>ret) ret=I->second;
  }
  return ret;
}


LyNote::LyNote(unsigned lev, bool dot){
  n.push_back(SingleNote(lev,dot));
}

LyNote::LyNote(LyNote n1, LyNote n2){
  std::list<SingleNote>::iterator I;
  //TODO: use algorithms from STL to copy iterator ranges
  for (I=n1.n.begin(); I!=n1.n.end(); I++){
    n.push_back(*I);
  }
  for (I=n2.n.begin(); I!=n2.n.end(); I++){
    n.push_back(*I);
  }
}

unsigned LyNote::TUs(void){
  unsigned ret=0;
  for(std::list<SingleNote>::iterator I=n.begin(); I!=n.end(); I++){
    ret+=I->TUs();
  }
  return ret;
}

bool LyNote::OK(void){
  for(std::list<SingleNote>::iterator I=n.begin(); I!=n.end(); I++){
    if (!I->OK()) return false;
  }
  return true;
}

bool LyNote::HasDot(void){
  for(std::list<SingleNote>::iterator I=n.begin(); I!=n.end(); I++){
    if (I->dot) return true;
  }
  return false;
}


inline bool LyNote::SingleNote::OK(void){
  return (lev<=pMetronome->MaxLev)&&
    (!dot||((1<<pMetronome->MaxLev)/(1<<lev)%2==0));
}

inline unsigned LyNote::SingleNote::TUs(void){
  assert(OK());
  unsigned ret=(1<<pMetronome->MaxLev)/(1<<lev);
  if (dot) ret=3*ret/2;
  return ret;
}

std::string LyNote::DbgPrint(void){
  std::ostringstream oss;
  oss<<"{";
  for(std::list<SingleNote>::iterator I=n.begin(); I!=n.end(); I++){
    oss<<"["<<(1<<I->lev)
      <<(I->dot?".":"")<<","
      <<I->TUs()<<"]";
  }
  oss<<TUs()<<"}";
  return oss.str();        
}


Notations::Notations(unsigned _TU_bar):TU_bar(_TU_bar){
  arr=new LyNote*[TU_bar*TU_bar]; //FIXME: more intelligent layout?
  for(unsigned i=0;i<TU_bar*TU_bar;i++) arr[i]=NULL;
}

inline unsigned Notations::ii2idx(unsigned on,unsigned len){
  assert(len<=TU_bar-on);
  return on*TU_bar+len; //FIXME: dtto
}

LyNote** Notations::at(unsigned TU_on,unsigned TUs){
  return &arr[ii2idx(TU_on,TUs)];
}

bool Notations::has(unsigned TU_on,unsigned TUs){
  return *at(TU_on,TUs)!=NULL;
}
