/*

 Really Unintelligent Music transcriptOR
 notation handling, both rhythm and pitch
  
*/

#include"rumor.hh"
#include"lang.hh"
const char Notator::WT_name[]={'c','d','e','f','g','a','b'};
//halftone number of whole tones
const int Notator::WT_ht[]={0,2,4,5,7,9,11};
// wholetone bases for any scale halftones
// see comment in NoteName(int,int)
const int Notator::Scale_wt[]={0,1,1,2,2,3,3,4,5,5,6,6};


Notator::Notator(void):os(std::cout),RefRhythm(10,false){
  //RefRhythm: invalid length 1/2^10 will make the first note's duration always explicit
  assert(pMetronome);
  pNotations=NULL;
  KeyBase=pOpts->KeyBase;
  KeyAcc=pOpts->KeyAcc;
  Legato=pOpts->Legato;
  RefPitch_wt=4*7; //"c" in absolute notation
    //in relative notation, we prefer to start with "c''"
  if(!pOpts->AbsolutePitches) RefPitch_wt+=2*7;

  assert(LangData[pOpts->Lang]);
  for(int i=0; i<35; i++){
    //DBGLOG(1,"Assigning "<<LangData[pOpts->Lang][i]<<"to "<<i/5<<","<<(i%5)-2);
    note_names[std::make_pair(i/5,(i%5)-2)]=LangData[pOpts->Lang][i];
  }

  
  if (pOpts->Full&&!pOpts->SampleSheet) {
    os<<"\\score{ \\notes ";
    if (!pOpts->AbsolutePitches) os<<"\\relative "<<note_names[std::make_pair(0,0)]<<"''";
    os<<"\\context Staff { \\time "
      <<pOpts->MeterP<<"/"<<pOpts->MeterQ<<" "
      <<LyStaffAccidentals(pOpts->Accidentals)<<"\n";
  }
}

Notator::~Notator(){
  if (pOpts->SampleSheet) {
    os<<"\n";
    return;
  }
  std::list<MidiNote>::reverse_iterator Last=notes.rbegin();
  if (Last!=notes.rend()){
    if (Last->TU_off()==0){ //finish last note, force noteoff if legato
      Legato=false;
      NoteOff(0,true);
      DBGLOG(1,"Forced note-off\n");
    }
    if (!pOpts->Strip&&(Last->TU_off()%pMetronome->TU_bar!=0)){ //trailing rest(s)
      PrintNote(MidiNote(0,Last->TU_off(),pMetronome->TU_bar*(Last->TU_off()/pMetronome->TU_bar+1)));
    }
  }  
  if (pOpts->Full) os<<"}\n\\paper{ }\n\\midi{ \\tempo "<<pOpts->MeterQ<<" = "<<pOpts->BPM<<" }\n}\n";
  else os<<"\n";
}

void Notator::NoteOn(unsigned pitch){
  bool DoNotPushBackThisNote=false;
  if(pOpts->Flat){os<<PitchName(pitch)<<" "<<std::flush; return;}
  MidiNote n(pitch,pMetronome->QuantizeNow(),0);
  if (notes.empty()&&!pOpts->Strip){// leading rest
    unsigned bars=n.TU_on/pMetronome->TU_bar;
    if (bars) DBGLOG(1,"Skipping first "<<bars<<" empty bar(s)\n");
    TRFL;
    PrintNote(MidiNote(0,bars*pMetronome->TU_bar,n.TU_on));
  }  
  if (!notes.empty()){
    std::list<MidiNote>::reverse_iterator Last=notes.rbegin();
    if (Last->TU_off()==0) { //last note has still not been released
      if ((pOpts->Chords)&&(Last->TU_on==n.TU_on)) {
	//potential chord beginning, if the last notes began at the same time
	//as the current one.
	//In this case, n's pitch is appended to the last one and
	//n as such is not pushed to notes
	Last->pitches[pitch]=0;//.push_back(*(n.pitches.begin()));
	DoNotPushBackThisNote=true;
      } else {//this note begins later than the last one; last note is offed and printed
        Last->TU_off(n.TU_on);
	TRFL;
        PrintNote(*Last);
        if (Last->dur()==0) LOG(1,"Zero-length note "<<Last->pitches.begin()->first<<" ignored.\n");
      }
    }
    else{// this onset comes after a rest, print the rest
      if (Last->dur()==0){//last note before rest has zero-length
        if(n.TU_on-Last->TU_off()>0){
          //zero-note followed by a non-zero pause gets 1TU length;
	  //this is intended as a "staccato corrective"
          Last->TU_off(Last->TU_off()+1);
          PrintNote(*Last);
        } else LOG(1,"Zero-length note "<<Last->pitches.begin()->first<<" ignored.\n");
      }
      PrintNote(MidiNote(0,Last->TU_off(),n.TU_on));
    }
  }
  if(!DoNotPushBackThisNote) notes.push_back(n);
}

void Notator::NoteOff(unsigned pitch, bool all_pitches){
  if (Legato||pOpts->Flat) return;
  std::list<MidiNote>::reverse_iterator Last=notes.rbegin();  
  long now=pMetronome->QuantizeNow();
  //overlapping note, just ignore it
  if (!Last->ReleaseChordPitch(pitch,now)) return;
  DBGLOG(3,"Chord has pitches: ");
  for(std::map<unsigned,long>::iterator I=Last->pitches.begin(); I!=Last->pitches.end(); I++){
	  DBGLOG(3,I->first<<"("<<I->second<<"),");
  }
  DBGLOG(3,"\n");
  if (all_pitches) Last->TU_off(now); //forced noteoff
  // print this note that just finished; zero-notes are on hold
  if (Last->dur()>0) PrintNote(*Last);
}


LyNote Notator::CreateRhythm(unsigned TU_on,unsigned TUs, bool DotOK){
  unsigned lev0=pMetronome->TUlev(TU_on);
  for(unsigned lev=0; lev<=pMetronome->MaxLev; lev++){
    //try dotted note first, if it is allowed; then undotted
    for(int dot=((DotOK&&(lev<pMetronome->MaxLev))?1:0); dot>=0; dot--){
      LyNote ln(lev,(bool)dot);
      unsigned lnTUs=ln.TUs();
      if (lnTUs>TUs) continue;
      bool IsOK=true;
      // possibly add more conditions here
      if (IsOK&& (
          (lev<lev0)||
          (dot&&(lev<lev0+1))
         )) IsOK=false;
      
      if (IsOK){
        if (lnTUs==TUs) return ln;
        else return LyNote(ln,FindRhythm(TU_on+lnTUs,TUs-lnTUs,DotOK));
      }
    }
  }
  //never get here, it is a bug or nonsensical meter parameters
  assert(0);
}

LyNote Notator::FindRhythm(unsigned TU_on,unsigned TUs, bool DotOK){
  assert(TU_on+TUs<=pMetronome->TU_bar);
  if (!pNotations) return CreateRhythm(TU_on,TUs,DotOK);
  assert(TU_on+TUs<=pMetronome->TU_bar);
  unsigned len;
  for(len=TUs;len>0;len--){
    if (pNotations->has(TU_on,len) &&
     (DotOK||(!(*pNotations->at(TU_on,len))->HasDot()))) break;
  }  
  if (len==0){ //note not found in what is given
    LyNote ret=CreateRhythm(TU_on,TUs, DotOK);
    unsigned l=ret.TUs();
    if (l==TUs) return ret;
    else return LyNote(ret,FindRhythm(TU_on+l,TUs-l,DotOK));
  }

  LyNote ret(**pNotations->at(TU_on,len));
  if (len==TUs) return ret;
  else return LyNote(ret,FindRhythm(TU_on+len,TUs-len,DotOK));
}



void Notator::PrintNote(MidiNote n){
  std::ostringstream oss;
  unsigned TU_on=n.TU_on%pMetronome->TU_bar;
  assert(n.TU_off()-n.TU_on>=0); //negative duration is error, zero duration can be due to quantization
  unsigned TUs=n.TU_off()-n.TU_on;
  while (TUs>0){
    // FIXME?: there is a bug here: if TUs is very big (like 1000),
    // operations on oss will "randomly" change pMetronome->TU_bar
    // (or even something else) to nonsense values
    unsigned InBar=(pMetronome->TU_bar-TU_on<TUs?pMetronome->TU_bar-TU_on:TUs);
    // dots are never allowed for rests, for notes disallowed optionally
    bool DotOK=(!n.IsRest())&&!pOpts->NoDots;
    LyNote ln=FindRhythm(TU_on,InBar,DotOK);
    for(std::map<unsigned,long>::iterator I=n.pitches.begin(); I!=n.pitches.end(); I++){
      ln.pitches.insert(I->first);
    }
    //ln.pitches=n.pitches; //FIXME: is this possible?
    oss<<PrintLyNote(ln);
    TUs-=InBar;
    TU_on=(TU_on+InBar)%pMetronome->TU_bar;
    if (TU_on==0)  oss<<((!n.IsRest())&&TUs?" ~":" ")<<"|\n"<<std::flush;
    else oss<<" ";
  }
  os<<oss.str()<<std::flush; //pass rdbuf(), not str()
}

std::string Notator::PrintSingleNote(LyNote::SingleNote sn,std::set<unsigned> pitches){
  std::ostringstream oss;
  if((!pOpts->ExplicitDurations)&&(RefRhythm.dot==sn.dot)&&(RefRhythm.lev==sn.lev)){
    oss<<PitchOrChordName(pitches);
  }
  else {
    oss<<PitchOrChordName(pitches)<<(1<<sn.lev)<<(sn.dot?".":"");
  }
  RefRhythm=sn;
  return oss.str();
}

std::string Notator::PrintLyNote(LyNote ln){
  std::ostringstream oss;
  std::string ret;
  for(std::list<LyNote::SingleNote>::iterator I=ln.n.begin(); I!=ln.n.end(); I++){
    if ((I!=ln.n.begin())){
      if (!ln.IsRest()) oss<<" ~ "; //rests need not to be tied
      else oss<<" ";
    }
    oss<<PrintSingleNote(*I,ln.pitches);
  }
  return oss.str();
}

void Notator::SampleSheet(void){
  os<<"\\header { title = \"Rumor sample sheet\"\n"
      "           subtitle = \"rhythm notations, keys\"\n"
      "           opus = \"Grain "<<(1<<pMetronome->MaxLev)<<"\"\n"
      "           composer = \"Meter "<<pMetronome->MeterP<<"/"<<pMetronome->MeterQ<<"\"\n"
      "}\n";
  os<<"\\score{\\notes ";
  if (!pOpts->AbsolutePitches) os<<"\\relative "<<note_names[std::make_pair(0,0)]<<"'' ";
  os<<"\\context Staff { \\fatText \\time "<<pMetronome->MeterP<<"/"<<pMetronome->MeterQ<<"\n";  
  for(unsigned TU_on=0; TU_on<pMetronome->TU_bar; TU_on++){
    for(unsigned TUs=1; TUs<=pMetronome->TU_bar-TU_on; TUs++){
      os<<"\\mark \""<<TU_on<<"+"<<TUs<<"\"\n";
      // What the hell is going on here; if I say 
      //  std::cout<<PrintNote(r_before)<<PrintNote(mn)<<PrintNote(r_after)<<endl;
      //  it will randomly change RefLev and RefDot, why???
      PrintNote(MidiNote(0,0,TU_on));
      PrintNote(MidiNote(72,TU_on,TU_on+TUs));
      PrintNote(MidiNote(0,TU_on+TUs,pMetronome->TU_bar));
      os<<"|\n";
    }
    os<<"\\bar\"||\"\n";    
  }
  os<<"\\bar\".|.\"\n\n";
  //os<<"\\property Score.timing = ##f\n";
  os<<Notator::LyStaffAccidentals(pOpts->Accidentals)<<"\n";
  int TU=0;
  for(KeyBase=0; KeyBase<7; KeyBase++){
    for(KeyAcc=-1; KeyAcc<=1; KeyAcc++){
      //os<<"\n\\mark\""<<note_names[std::make_pair(KeyBase,KeyAcc)]<<"\"\n";
      for(unsigned pitch=60; pitch<72; pitch++){
	PrintNote(MidiNote(pitch,TU,TU+1)); 
        TU++;
	if(pitch==60) os<<"_\""<<note_names[std::make_pair(KeyBase,KeyAcc)]<<"\" ";
      }
    }
  }
  os<<"\n\\bar\"|.\"\n} }\n";
}

// given halftone abolute pitch and wholetone base
// return number of accidentals (flats negative, sharps positive)
int Notator::GetAccidental(int note_ht,int base_wt){
  int diff_ht=note_ht-WT_ht[base_wt];
  // wrap everything +-2 from any multiple of 12 to [-2..2]
  if (abs(diff_ht)>2) diff_ht=(diff_ht+18)%12-6;
  if (abs(diff_ht)>2) throw std::runtime_error("More than 2 accidentals needed.");
  return diff_ht;
}

std::string Notator::PitchOrChordName(std::set<unsigned> pitches){
  if (pitches.size()==0) return "r"; //rest
  if (pitches.size()==1) return PitchName(*(pitches.begin())); //single note

  //chord
  //TODO: reorder pitches within chord (ascending/descending)
  unsigned save_RefPitch_wt;
  std::string ret("<");
  for (std::set<unsigned>::iterator I=pitches.begin(); I!=pitches.end(); I++){
    if (I!=pitches.begin()) ret+=" ";
    ret+=PitchName(*I);
    //in chords, the first note is the reference at the beginning of another chord
    //however, within the chord, each note is relative to the preceeding one
    if (I==pitches.begin()) save_RefPitch_wt=RefPitch_wt;
  }
  ret+=">";
  RefPitch_wt=save_RefPitch_wt;
  return ret;
}
// enharmonics resolution based on current key
std::string Notator::PitchName(int Pitch_ht, bool WithOctaveTics){
  /*
  for enharmonics reolution, we do not need to distinguish
  scales beginning from the same note, e.g. from c:
  c-ionian (=major):  c d  e  f  g a  b
  c-dorian:           c d  eb f  g a  bb
  c-phrygic:          c db eb f  g ab bb
  c-lydic:            c d  e  f# g a  b
  c-mixolydic:        c d  e  f  g a  bb
  c-aiolian (=minor): c d  eb f  g ab bb
  where we preserve a-b-c-d-e-f-g continuity, as musical
  bontone wishes
  */
  
  assert(Pitch_ht!=0); //this should be filtered out in PitchOrChordName
  //if (Pitch_ht==0) return "r";   // rest
  
  std::string ret;
  // pitch of the current key
  int KeyBase_ht=WT_ht[KeyBase]+KeyAcc;
  // pitch of the note, relatively to current key base
  int RelPitch_ht=(Pitch_ht-KeyBase_ht+12)%12;
  // c-based wholetone "pitch" of the note
  unsigned Pitch_wt=(KeyBase+Scale_wt[RelPitch_ht])%7;
  // sharps/flats
  int Acc=GetAccidental((KeyBase_ht+RelPitch_ht)%12,Pitch_wt);
  // c0-based wholetone number
  unsigned AbsPitch_wt=Pitch_wt+7*((Pitch_ht-Acc)/12);
  assert((Acc>=-2)&&(Acc<=2)&&(Pitch_wt>=0)&&(Pitch_wt<=6));
  ret+=note_names[std::make_pair(Pitch_wt,Acc)];
  if(WithOctaveTics) {
//    TREXP(AbsPitch_wt);
//    TREXP(RefPitch_wt);
    int NumTics;
    char OctaveTic;
    if(pOpts->AbsolutePitches){
      if (AbsPitch_wt>=RefPitch_wt){
        OctaveTic='\'';
	NumTics=(AbsPitch_wt-RefPitch_wt)/7;
      } else {
        OctaveTic=',';
	NumTics=(RefPitch_wt-AbsPitch_wt-1)/7+1;
      }
    } else {
      OctaveTic=(RefPitch_wt>AbsPitch_wt?',':'\'');
      NumTics=(abs(RefPitch_wt-AbsPitch_wt)+3)/7;
      RefPitch_wt=AbsPitch_wt;
    }
    for (int i=0; i<NumTics; i++){
      ret+=OctaveTic;
    }
  }
  return ret;
}

#ifdef HAVE_GUILE
SCM Notator::gh_rumor_pitches(SCM plist){
  int total=0;
  //TREXP(gh_length(plist));
  for(unsigned i=0; i<gh_length(plist); i++){
    SCM pdesc=gh_list_ref(plist,to_SCM(i));
    if (gh_length(pdesc)!=6) throw script_error("invalid pitch description length");
    unsigned wt=SCM_to<unsigned>(gh_list_ref(pdesc,to_SCM(0)));
    if ((wt<0)||(wt>6)) throw script_error("invalid base tone number (must be 0..6)");
    for(int j=1;j<6;j++){
      note_names[std::make_pair(wt,j-3)]=SCM_to<std::string>(gh_list_ref(pdesc,to_SCM(j)));
      total++;
    }
  }
  LOG(2,"pitches: "<<total<<" pitches assigned\n");
  return to_SCM(total);
}

SCM Notator::gh_rumor_rhythms(SCM G, SCM P, SCM Q, SCM rhys){
  int Granularity=Metronome::log_2(SCM_to<unsigned>(G));
  unsigned MNum=SCM_to<unsigned>(P);
  unsigned MDen=SCM_to<unsigned>(Q);
  if (Granularity<0) throw script_error("invalid grain specified");

  unsigned TUdiv=1,TUmul=1;
  unsigned TU_bar=((1<<Granularity)*MNum)/MDen;
  LOG(1,"rhythms: grain "<<(1<<Granularity)<<" ("<<TU_bar<<"TU/bar), meter "<<MNum<<"/"<<MDen<<"\n");
  if ((MNum*pMetronome->MeterQ!=MDen*pMetronome->MeterP)){
    std::ostringstream oss; oss<<"meters "<<MNum<<"/"<<MDen<<" and "<<
      pMetronome->MeterP<<"/"<<pMetronome->MeterQ<<" not compatible";
    throw script_error(oss.str());
  }
  if (TU_bar!=pMetronome->TU_bar){
    unsigned  MTb=pMetronome->TU_bar;
    if ((MTb>TU_bar)&&(MTb%TU_bar==0)) {
      TUmul=MTb/TU_bar;
      LOG(2,"rhythms will be multiplied by "<<TUmul<<"\n");
    }
    else if ((TU_bar>MTb)&&(TU_bar%MTb==0)){
      TUdiv=TU_bar/MTb;
      LOG(2,"rhythms will be divided by "<<TUdiv<<"\n");
    }
    else {throw script_error("bars not compatible");}
  }
    
  if (pNotations){
    assert(pNotations->TU_bar=TU_bar);
    LOG(1,"warning: notations will be merged");
  }
  else pNotations=new Notations((TU_bar*TUmul)/TUdiv);

  unsigned rhyOK=0;

  for(unsigned i=0; i<gh_length(rhys); i++){
    SCM onsets=gh_list_ref(rhys,to_SCM(i));
    SCM rhy=gh_list_ref(onsets,to_SCM(0));
    //first get rhythm description
    LyNote* lyn=NULL;
    for(unsigned j=0; j<gh_vector_length(rhy); j++){
      // primitive rhythm
      SCM rhy0=gh_vector_ref(rhy,to_SCM(j));
      unsigned len,dot;
      if (gh_pair_p(rhy0)){
        len=SCM_to<unsigned>(gh_car(rhy0)); dot=SCM_to<unsigned>(gh_cdr(rhy0));
      } else {
        assert(gh_number_p(rhy0));
        len=SCM_to<unsigned>(rhy0); dot=0;
      }
      unsigned ex=0;
      while ((ex<6)&&(len!=(unsigned)1<<ex)) ex++;
      if (ex==6) { //is not 2^n
        LOG(1,"Error: invalid duration "<<len<<" (skipped)\n"); break;
      }
      assert((dot==0)||(dot==1)); //FIXME: are more dots needed?
      // this note is invalid: duration is not integer given our metronome settings
      if (!LyNote(ex,dot).OK()) break;
      if (!lyn) lyn=new LyNote(ex,dot);
      else {
        LyNote* _lyn=lyn;
        lyn=new LyNote(*_lyn,LyNote(ex,dot));
        delete _lyn;
      }
    }
    if (!lyn) continue;
    LOG(2,"rhythm "<<lyn->DbgPrint()<<": ");
    for(unsigned j=1; j<gh_length(onsets); j++){
      unsigned TU_on=SCM_to<unsigned>(gh_list_ref(onsets,to_SCM(j)));
      // invalid TU_on; rhythm is too refined
      if((TU_on*TUmul)%TUdiv!=0){
        LOG(2,"[rhythm onset "<<TU_on<<"(*"<<TUmul<<"/"<<TUdiv<<") skipped]\n");
        continue;
      }
      TU_on=(TU_on*TUmul)/TUdiv;
      if (TU_on+lyn->TUs()>TU_bar){
        LOG(1,"Error: rhythm ["<<TU_on<<","<<lyn->TUs()<<"] ends after barline (skipped)\n");
        continue;
      }
      LOG(2,"["<<TU_on<<","<<lyn->TUs()<<"]");
      if (!pNotations->has(TU_on,lyn->TUs())){
        *pNotations->at(TU_on,lyn->TUs())=new LyNote(*lyn);
        rhyOK++;
      }
      else LOG(1,"Error: rhythm "<<TU_on<<","<<lyn->TUs()<<" already assigned (skipped)\n");
    }
    LOG(2,"\n");
    delete lyn;
  }
  LOG(1,"rhythms: "<<rhyOK<<" valid rhythms assigned\n");
  return to_SCM(rhyOK);
}

#endif /* HAVE_GUILE */

std::string Notator::LyStaffAccidentals(int acc){
  std::ostringstream oss;
  if (acc){
    oss<<"\\property Staff.keySignature = #'(";
    for(int i=1; i<=abs(pOpts->Accidentals); i++){
      int wt=(pOpts->Accidentals>0?6+4*i:3+3*i)%7;
      oss<<"("<<wt<<" . "<<(pOpts->Accidentals>0?1:-1)<<")";
    }
    oss<<")";
  }
  return oss.str();
}
