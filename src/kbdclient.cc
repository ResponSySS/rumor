/*

 Really Unintelligent Music transcriptOR
 computer keyboard MIDI keyboard emulation
  
*/


#include"rumor.hh"

KbdMidiClient::KbdMidiClient(MidiClient* _rmc):
  RealMidiClient(_rmc),LastPitch(0),Channel(0),Instr(16)
  //Channel and Instr are arbitrary
{
  TRFL;  
  if (!isatty(STDIN_FILENO)) throw std::runtime_error("standard input is not a terminal");
  tcgetattr(STDIN_FILENO,&TtyAttr);
  struct termios tmpattr;
  tcgetattr(STDIN_FILENO,&tmpattr);
  tmpattr.c_cc[VTIME]=0;
  tmpattr.c_cc[VMIN]=1;
  tmpattr.c_lflag&=~(ICANON|ECHO);
  DBGLOG(2,"Setting terminal attributes\n");
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&tmpattr);
  
  SetDefaultBindings();
}

KbdMidiClient::~KbdMidiClient(){
  PlayNote(LastPitch,0,Channel,Instr);
  delete RealMidiClient;
  DBGLOG(2,"Reverting terminal attributes\n");
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&TtyAttr);
}

void KbdMidiClient::PlayNote(unsigned pitch, unsigned velocity, unsigned instrument, unsigned channel){
  RealMidiClient->PlayNote(pitch,velocity,instrument,channel);
}

void KbdMidiClient::Loop(){
  char c; unsigned pitch;
  while (read(STDIN_FILENO,&c,1)){
    if ((Keys.count(c)!=0)&&((pitch=Keys[c])!=LastPitch)){
      //DBGLOG(5,"key "<<(int)c<<"->"<<pitch<<"\n");
      PlayNote(LastPitch,0,16,0);
      if (pitch==0) {
	//release last pitch, but all other pitches as well (for chords)
        pNotator->NoteOff(LastPitch,true);
        LastPitch=Keys[c];
      }  
      else {
        LastPitch=pitch;
        PlayNote(LastPitch,127,Instr,Channel);
        pNotator->NoteOn(LastPitch);
      }
    }
  }  
}

void KbdMidiClient::SetDefaultBindings(void){
  struct KP{char k; unsigned p;};
  unsigned c0pitch=60;
  KP kp[]={{'z',0},{'s',1},{'x',2},{'d',3},{'c',4},{'v',5},{'g',6},{'b',7},{'h',8},{'n',9},{'j',10},{'m',11},
           {',',12},{'l',13},{'.',14},{';',15},{'/',16},{'\'',17},
           {'q',12},{'2',13},{'w',14},{'3',15},{'e',16},{'r',17},{'5',18},{'t',19},{'6',20},{'y',21},{'7',22},{'u',23},
           {'i',24},{'9',25},{'o',26},{'0',27},{'p',28},{'[',29},{'=',30},{']',31},
           {0}};
  for(int i=0; kp[i].k!=0; i++){
    Keys[kp[i].k]=kp[i].p+c0pitch;
  }
  Keys[' ']=0;// spacebar for rest
}

#ifdef HAVE_GUILE
SCM KbdMidiClient::gh_rumor_kbd(SCM replace, SCM transposition, SCM rest){
  unsigned keysOK=0;
  unsigned trsp=SCM_to<int>(transposition);
  if (SCM_to<bool>(replace)){
    LOG(2,"Emptying all ("<<Keys.size()<<") key bindings\n");
    Keys.clear();
  }
  for(unsigned i=0; i<gh_length(rest); i++){
    assert(gh_pair_p(gh_list_ref(rest,to_SCM(i))));
    assert(gh_char_p(gh_car(gh_list_ref(rest,to_SCM(i)))));
    assert(gh_number_p(gh_cdr(gh_list_ref(rest,to_SCM(i)))));    
    char key=       SCM_to<char>(gh_car(gh_list_ref(rest,to_SCM(i))));
    unsigned pitch=trsp+SCM_to<unsigned>(gh_cdr(gh_list_ref(rest,to_SCM(i))));
    Keys[key]=pitch;
    DBGLOG(3,"key: "<<key<<"->"<<pitch<<"\n");
    keysOK++;
  }
  LOG(1,"key: "<<keysOK<<" keys assigned\n");
  return to_SCM(keysOK);
}
#endif /* HAVE_GUILE */
