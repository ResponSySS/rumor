/*

 Really Unintelligent Music transcriptOR
 meter and time quantization
  
*/


#include"rumor.hh"


// This should make metronome sound as drums but
// right now I can not get it working, why... ?
/* const unsigned Metronome::BeatPatches[]={15,20};
const unsigned Metronome::BeatPitches[]={15,20};
const unsigned Metronome::BeatVolumes[]={127,95};
const double   Metronome::BeatLengths[]={0.05,0.05};
const unsigned Metronome::BeatChannel  =9;*/


pthread_t* Metronome::Thread=NULL;

void* Metronome::ThreadLoop(void*){pMetronome->Loop(); return NULL;}

void Metronome::CreateThread(void){
  if(pOpts->Wait){
    t0=AbsTime()-(pOpts->WaitBeat-1)*TU_beat*sec_TU;
    BeatCnt=pOpts->WaitBeat-1;
  }
  else {t0=AbsTime(); BeatCnt=0;}  
  Thread=new pthread_t;
  pthread_create(Thread,NULL,Metronome::ThreadLoop,NULL);    
}

double Metronome::AbsTime(void){
  struct timeval tv;
  struct timezone tz;  
  gettimeofday(&tv,&tz);
  return tv.tv_sec+1e-6*tv.tv_usec;
}

double Metronome::RelTime(void){
  return AbsTime()-t0;
}

long Metronome::QuantizeNow(double bias){
  if (!Thread) CreateThread();
  double dt=RelTime()-LastBeatt;
  unsigned ret=TU_beat*BeatCnt+((unsigned)((dt/sec_TU)+0.5+bias));
  if (ret>TU_beat*BeatCnt+TU_beat){
    LOG(1,"Metronome warning: High latency, quantization is not accurate\n");
    DBGLOG(0,"Theoretically quantized to "<<ret<<", but future beat is "<<TU_beat*BeatCnt+TU_beat<<", so I change to this (latest consistent) value\n");
    ret=TU_beat*BeatCnt+TU_beat;
  }
  return ret;
}

void Metronome::SetBPM(double _BPM){
  if (_BPM<=0) return;
  BPM=_BPM;
  sec_TU=60./(BPM*TU_beat);
  LOG(1,"Metronome: "<<beat_bar<<" beats/bar, "<<
    TU_beat<<" TU/beat, "<<
    TU_bar<<" TU/bar, "<<
    sec_TU<<" sec/TU"<<"\n");
}

Metronome::Metronome():
  MeterP(pOpts->MeterP),
  MeterQ(pOpts->MeterQ),
  MaxLev(pOpts->Granularity)
{
  TRFL;
  beat_bar=MeterP;
  TU_beat=(1<<MaxLev)/MeterQ;
  TU_bar=TU_beat*beat_bar;
  SetBPM(pOpts->BPM);
  BeatChannel=15;
  BeatPatches[0]=16; BeatPatches[1]=18;
  BeatPitches[0]=70; BeatPitches[1]=70;
  BeatVolumes[0]=127;BeatVolumes[1]=95;
  BeatLengths[0]=0.05;BeatLengths[1]=0.05;
}

void Metronome::Nanosleep(long nsec){
  timespec ts;
  ts.tv_sec=nsec/1000000000;
  ts.tv_nsec=nsec%1000000000;
  nanosleep(&ts,NULL);
}

#ifdef HAVE_GUILE
SCM Metronome::gh_rumor_beats(SCM channel, SCM beat1st, SCM beats){
  if(gh_length(beat1st)!=4) throw script_error("Invalid number of beat specifications (must be 4: patch, pitch, volume, length)");
  //channels numbered 1--16
  BeatChannel=SCM_to<unsigned>(channel)-1;

  //patches numbered 1--128
  BeatPatches[0]=SCM_to<unsigned>(gh_list_ref(beat1st,to_SCM(0)))-1;
  BeatPitches[0]=SCM_to<unsigned>(gh_list_ref(beat1st,to_SCM(1)));
  BeatVolumes[0]=SCM_to<unsigned>(gh_list_ref(beat1st,to_SCM(2)));
  BeatLengths[0]=SCM_to<double>  (gh_list_ref(beat1st,to_SCM(3)));

  BeatPatches[1]=SCM_to<unsigned>(gh_list_ref(beats,to_SCM(0)))-1;
  BeatPitches[1]=SCM_to<unsigned>(gh_list_ref(beats,to_SCM(1)));
  BeatVolumes[1]=SCM_to<unsigned>(gh_list_ref(beats,to_SCM(2)));
  BeatLengths[1]=SCM_to<double>  (gh_list_ref(beats,to_SCM(3)));
  LOG(1,"Beat parameters assigned\n");
  return to_SCM(0);
}
#endif /* HAVE_GUILE */

void Metronome::Loop(void){
  while (1){
    unsigned beat=BeatCnt%beat_bar;
    //if (!beat) std::cerr<<"|"; else std::cerr<<".";
    pthread_testcancel();
    double t=RelTime();
    LastBeatt=t;
    pMidiClient->PlayNote(BeatPitches[beat?1:0],BeatVolumes[beat?1:0],BeatPatches[beat?1:0],BeatChannel);
    Nanosleep((unsigned long)(1e9*BeatLengths[beat?1:0]));
    pMidiClient->PlayNote(BeatPitches[beat?1:0],0,BeatPatches[beat?1:0],BeatChannel);
    Nanosleep((unsigned long)(1e9*(sec_TU*TU_beat-BeatLengths[beat?1:0])));
    BeatCnt++;    
  }
}

unsigned Metronome::TUlev(unsigned TU){
  unsigned e=0;
  while(((1<<e)*TU)%(1<<MaxLev)) e++;
  return e;
}

int Metronome::log_2(unsigned u){
  int k=-1;
  while(1<<++k<=(signed)u) if (1<<k==(signed)u) return k;
  return -1;
}

Metronome::~Metronome(void){
  TRFL;
  //noteoffs for possible sounding beep during cancellation
  pMidiClient->PlayNote(BeatPitches[0],0,BeatPatches[0],BeatChannel);
  pMidiClient->PlayNote(BeatPitches[1],0,BeatPatches[1],BeatChannel);  
}


