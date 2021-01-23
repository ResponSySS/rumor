/*

 Really Unintelligent Music transcriptOR
 class and globals declarations
  
*/

#ifndef INCLUDE_RUMOR_HH
#define INCLUDE_RUMOR_HH

#include<config.h>
//#include<libintl.h>

#include<iostream>
#include<cstring>
#include<list>
#include<map>
#include<set>
#include<sys/time.h>
#include<cassert>
#include<stdexcept>
#include<sstream>

#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<termios.h>

#ifdef HAVE_ALSA
#  include<alsa/asoundlib.h>
#endif

#ifdef HAVE_GUILE
#  include"guile2cc.hh"
#endif

#ifdef DEBUG
#  define DBGLOG(l,a) LOG(4+l,a)
#else
#  define DBGLOG(l,a) {}
#endif

#define DBGCOORD __FILE__<<":"<<__LINE__<<" in `"<<__PRETTY_FUNCTION__<<"'"
#define TRFL DBGLOG(1,DBGCOORD<<"\n")
#define TREXP(exp) DBGLOG(1,DBGCOORD<<": " #exp "="<<(exp)<<"\n")
#define TRCMD(cmd) DBGLOG(1,DBGCOORD<<": " #cmd "\n"); cmd

#define LOG(level,msg) \
  {if(level<=pOpts->Verbosity) { \
    for(int __i=0; __i<level; __i++)std::cerr<<"  "; std::cerr<<msg;}}

/*class option_error: public std::exception{
  std::string _what;
public:
  option_error(const std::string& s): _what(s){ }
  virtual const char* what() const {return _what.c_str();}
};*/


enum{INTERFACE_ALSA,INTERFACE_OSS,PITCHNAMES_NEDERLANDS,PITCHNAMES_ITALIANO};

struct Options{
  int MeterP,MeterQ,BPM,Granularity,Accidentals,KeyAcc,KeyBase,WaitBeat;
  bool Legato,Full,SampleSheet,KbdMidi,Strip,Wait,NoDots,Flat,AbsolutePitches,ExplicitDurations;
  std::string AlsaInPort,AlsaOutPort;
  std::list<std::string> GuileScripts;
  int OssDevNum, MidiInt, Verbosity, Lang, Chords;
  //Chords: 0 for chords disabled, 1 enabled,
  //in teh future possibly 2 for "split" chords (separated stems)
  int SanityCheck(void);
  Options(void):
    MeterP(4),MeterQ(4),BPM(100),Granularity(4),Accidentals(0),KeyAcc(0),KeyBase(0),
    WaitBeat(1),
    Legato(false),Full(false),SampleSheet(false),KbdMidi(false),
    Strip(false), Wait(false), NoDots(false), Flat(false),
    AbsolutePitches(false), ExplicitDurations(false),
    AlsaInPort("64:0"),AlsaOutPort("65:0"),
    OssDevNum(1),
#   ifdef HAVE_ALSA
      MidiInt(INTERFACE_ALSA),
#   else
#   ifdef HAVE_OSS
      MidiInt(INTERFACE_OSS),
#   else
#     error You lose. No ALSA nor OSS. (this is a bug; configure should have aborted if there is no interface)
#   endif /* HAVE_OSS */
#   endif /* HAVE_ALSA */
    Verbosity(0),Lang(0),Chords(1)
    {}
};

/* This class hold a single note (or a chord) with arbitrary duration
 * without any connection with the real (print) appearance in
 * Lilypond. Unlike LyNote, the nested class SingleNote is just rhythmicla
 * representation, the graphical note that actually gets printed.
 * Thus for example LyNote may begin at the first beat, end at
 * the fourth. It will be (normally) printed as two tied
 * SingleNotes, half and quarter. */

class LyNote{
public:
  bool OK(void);
  class SingleNote{
  public:
    unsigned lev;
    bool dot;
    SingleNote(unsigned _lev, bool _dot):lev(_lev),dot(_dot){}
    unsigned TUs(void);
    bool OK(void);
  };
  std::list<SingleNote> n;
  std::set<unsigned> pitches;
  LyNote(unsigned, bool);
  LyNote(LyNote,LyNote);
  std::string DbgPrint(void);
  unsigned TUs(void);
  bool HasDot(void);
  bool IsRest(void){return pitches.size()==0;}
  //friend class Notator;
};

class MidiNote{
  long forced_TU_off;
public:
  std::map<unsigned,long> pitches;
  long dur(void){return TU_off()-TU_on;}
  long TU_on;
  long TU_off(void); // get TU_off
  void TU_off(long _TU_off){forced_TU_off=_TU_off;} //sets TU_off, disregards individual chord pitches
  MidiNote(unsigned _pitch, long _TU_on, long _TU_off=0):
    forced_TU_off(_TU_off),TU_on(_TU_on){if(_pitch) pitches[_pitch]=_TU_off;}
  bool ReleaseChordPitch(unsigned, long);
  bool IsRest(void){return pitches.size()==0;}
};


class Notations{
  unsigned ii2idx(unsigned,unsigned);
  LyNote** arr;
public:
  unsigned TU_bar;
  Notations(unsigned);
  LyNote** at(unsigned,unsigned);
  bool has(unsigned,unsigned);
};


class Notator{
  std::list<MidiNote> notes;
  int GetAccidental(int,int);
  std::string PitchName(int,bool WithOctaveTics=true); 
  std::string PitchOrChordName(std::set<unsigned>);
  std::ostream& os;
  unsigned KeyBase; int KeyAcc;
  unsigned RefPitch_wt;
  LyNote::SingleNote RefRhythm; 
  bool Legato;
  Notations *pNotations;
  std::map<std::pair<unsigned,int>,std::string> note_names;
public:
  static const char *Langs[];
  static const char *LangData[][35];
  #ifdef HAVE_GUILE
    SCM gh_rumor_rhythms(SCM,SCM,SCM,SCM);
    SCM gh_rumor_pitches(SCM);
  #endif  
  static const char WT_name[7];
  static const int WT_ht[7];
  static const int Scale_wt[12];

  Notator(void);
  ~Notator();
  void NoteOn(unsigned);
  void NoteOff(unsigned pitch, bool all_pitches=false);
  std::string PrintLyNote(LyNote);
  std::string PrintSingleNote(LyNote::SingleNote,std::set<unsigned>);
  void PrintNote(MidiNote);
  LyNote FindRhythm(unsigned,unsigned,bool);
  LyNote CreateRhythm(unsigned,unsigned,bool);  
  void SampleSheet(void);
  static std::string LyStaffAccidentals(int);
  
};

class MidiClient{
public:
  virtual void PlayNote(unsigned,unsigned,unsigned,unsigned)=0;
  virtual void Loop(void)=0;
  virtual ~MidiClient(){};
  static void* ThreadLoop(void*);
  void CreateThread(void);
  static pthread_t* Thread;  
};

#ifdef HAVE_ALSA
class AlsaMidiClient: public MidiClient{
  int InPort, OutPort,Client;
  snd_seq_t* SeqHandle;
  void HandleMidiEvent(void);
  void SetupConnections(void);
public:
  AlsaMidiClient(void);
  void PlayNote(unsigned,unsigned,unsigned,unsigned);
  void Loop(void);
};
#endif

#ifdef HAVE_OSS
class OssMidiClient: public MidiClient{
  int seqfd;
  int device;
public:
  OssMidiClient(void);
  ~OssMidiClient();
  void PlayNote(unsigned,unsigned,unsigned,unsigned);
  void Loop(void);
};
#endif

class KbdMidiClient: public MidiClient{
  std::map<char,unsigned> Keys;
  MidiClient* RealMidiClient;
  unsigned LastPitch, Channel, Instr;
  void SetDefaultBindings(void);
  struct termios TtyAttr;
public:
  #ifdef HAVE_GUILE
    SCM gh_rumor_kbd(SCM,SCM,SCM);
  #endif  
  KbdMidiClient(MidiClient*);
  ~KbdMidiClient();
  void PlayNote(unsigned,unsigned,unsigned,unsigned);
  void Loop(void);
};

class Metronome{
  // these may not change at runtime
  double BPM;
  double sec_TU;
  unsigned TU_beat,beat_bar;  
  long BeatCnt;
  double LastBeatt;
  double t0;

  //beat indication parameters
  //first value at the first beat only, other ones elsewhere
  unsigned BeatPatches[2];
  unsigned BeatPitches[2];
  unsigned BeatVolumes[2];
  double BeatLengths[2];
  unsigned BeatChannel;

  static void* ThreadLoop(void*);
  void Loop(void);  
  void Nanosleep(long);

public:  
  // these are needed by Notator
  const unsigned MeterP,MeterQ;
  const unsigned MaxLev;
  unsigned TU_bar;
  
  Metronome(void);
  ~Metronome();
  double AbsTime(void);
  double RelTime(void);
  void SetBPM(double);
  double GetBPM(void){return BPM;}
  long QuantizeNow(double bias=0);
  unsigned TUlev(unsigned);
  void CreateThread(void);
  static int log_2(unsigned);
  static pthread_t* Thread;
  #ifdef HAVE_GUILE
    SCM gh_rumor_beats(SCM, SCM, SCM);
  #endif
};


extern MidiClient *pMidiClient;
extern Metronome *pMetronome;
extern Notator *pNotator;
extern Options *pOpts;
extern struct argp argp_instance;

#endif /* INCLUDE_RUMOR_HH */
