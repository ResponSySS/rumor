#include<argp.h>
#include"rumor.hh"

const char *argp_program_version=PACKAGE_VERSION;
const char *argp_program_bug_address=PACKAGE_BUGREPORT;
const char *prog_doc=
  "Rumor: Really Unintelligent Music transcriptOR"
  "\v"
  "Rumor listens to your MIDI keyboard and outputs "
  "the monophonic melody (pitches and rhythm) in Lilypond "
  "format. See documentation and http://klobouk.fsv.cvut.cz/~xsmilauv/rumor/ "
  "for more information.";
const char *args_doc=NULL;
enum {longopt_alsa=1,longopt_oss,longopt_sample_sheet,longopt_kbd,longopt_script,longopt_flat,longopt_absolute_pitches,longopt_explicit_durations,longopt_lang,longopt_no_chords};

const struct argp_option options_vec[]={

  {0,0,0,0,"Input options:"},
  {"flat",longopt_flat,0,0,"Do not quantize at all, only print pitches"},
  {"tempo",'t',"BPM",0,"metronome tempo in beats per minute [100]"},
  {"meter",'m',"P[/]Q",0,"bar meter [44]"},
  {"grain",'g',"N",0,"time resolution is duration of Nth note [16]"},
  {"legato",'l',0,0,"legato quantization (no rests)"},
  {"no-chords",longopt_no_chords,0,0,"Disable chords (simultaneous notes)"},
  {"wait",'w',"BEAT",OPTION_ARG_OPTIONAL,"wait for MIDI input to start metronome, at the beat specified [1]"},

  {0,0,0,0,"Output options:"},

  {"full",'f',0,0,"output complete Lilypond file"},
  {"strip",'s',0,0,"strip leading and trailing rests (bars may not be complete)"},
  {"key",'k',"NOTE",0,"key base note (e.g. ces, fis, a) of corresponing ionian (major) scale; this is for enharmonics resoution [c]"},
  {"accidentals",'a',"NUM",0,"sharps positive, flats negative; makes no sense unless with --full or --sample-sheet [0]"},
  {"no-dots",'D',0,0,"disallow dotted notes"},
  {"lang",longopt_lang,"{ne,en,en-short,de,no,sv,it,ca,es}",0,"[ne]"},
  {"absolute-pitches",longopt_absolute_pitches,0,0,"do not use relative notation"},
  {"explicit-durations",longopt_explicit_durations,0,0,"Always show duration, even if same as the last one"},
  {"sample-sheet",longopt_sample_sheet,0,0,"dumps a Lilypond file showing all durations and all keys"},
  {"verbose",'v',0,0,"Print various information and warnings (more occurences are valid)"},

  {0,0,0,0,"Interface options:"},

# ifdef HAVE_ALSA
  {"alsa",longopt_alsa,"[IC:IP,]OC:OP",OPTION_ARG_OPTIONAL,"use ALSA, with specified I/O clients and ports [64:0,65:0]"},
# endif
# ifdef HAVE_OSS
  {"oss",longopt_oss,"DEV",OPTION_ARG_OPTIONAL,"use OSS, with specified device number on /dev/sequencer"},
# endif
  {"kbd",longopt_kbd,0,0,"emulate MIDI keyboard using computer keyboard"},
# ifdef HAVE_GUILE
  {"script",longopt_script,"FILE",0,"run guile script in FILE before beginning (can be given more than once)"},
#endif  
  {0}
};



#define DO_OPTION_ERROR(msg) \
  { std::cerr<<msg<<"\n"; return EINVAL;}
#define OPTION_ERROR(msg) DO_OPTION_ERROR("Argument error: "<<msg)
#define CHECK_OPT_RANGE(opt,lo,hi) \
  { if ((pOpts->opt<lo)||(pOpts->opt>hi)) {\
      OPTION_ERROR("option " #opt "="<<pOpts->opt<<" out of range [" #lo ".." #hi "]");} }
#define CONS_OPTION_ERROR(msg) DO_OPTION_ERROR("Option consistency error: "<<msg)
#define CONS_OPTION_WARN(msg) {LOG(1,"Option warning: " msg);}

int Options::SanityCheck(void){
  if (1<<Granularity<MeterQ) CONS_OPTION_ERROR("grain must be bigger than meter Q");
  if (Wait&&((WaitBeat<=0)||(WaitBeat>MeterP))) CONS_OPTION_ERROR("waiting for beat that does not exist (must be in 1.."<<MeterP<<")");
  if ((!Full&&!SampleSheet)&&(Accidentals!=0)) CONS_OPTION_WARN("--accidentals has no effect unless with --full or --sample-sheet\n");
  if (Full&&Strip) {
    CONS_OPTION_WARN("--full and --strip together make no sense; --strip ignored\n");
    Strip=false;
  }
  if(Flat){
    if(Full){
      CONS_OPTION_WARN("--full ignored as it makes no sense with --flat\n");
      Full=false;
    } 
    //if(ExplicitDurations) CONS_OPTION_WARN("--explicit-durations has no effect with --flat\n");
  }
  return 0;
}


error_t
parse_option(int key, char* optarg, argp_state *state){
  switch(key){
    case longopt_sample_sheet: pOpts->SampleSheet=true; break;
    case longopt_kbd: pOpts->KbdMidi=true; break;
    case longopt_flat: pOpts->Flat=true; break;
    case longopt_absolute_pitches: pOpts->AbsolutePitches=true; break;
    case longopt_explicit_durations: pOpts->ExplicitDurations=true; break;
    case longopt_no_chords: pOpts->Chords=0; break;
    case 'v': pOpts->Verbosity++; break;
    case 's': pOpts->Strip=true; break;
    case 'D': pOpts->NoDots=true; break;
    case 'l': pOpts->Legato=true; break;  
    case 'f': pOpts->Full=true; break;
    case 't':
      pOpts->BPM=atoi(optarg);
      CHECK_OPT_RANGE(BPM,10,999); break;
    case 'g':{
      pOpts->Granularity=Metronome::log_2(atoi(optarg));
      if (pOpts->Granularity<0) OPTION_ERROR("grain must be 2^n");
      CHECK_OPT_RANGE(Granularity,1,6);
      break;
      }
    case 'a':
      pOpts->Accidentals=atoi(optarg);
      CHECK_OPT_RANGE(Accidentals,-7,7); break;  
    case 'w':
      pOpts->Wait=true;
      if (optarg) pOpts->WaitBeat=atoi(optarg);
      break;
#   ifdef HAVE_ALSA
    case longopt_alsa:
      pOpts->MidiInt=INTERFACE_ALSA;
      if(optarg){
        std::string s_opt(optarg);      
        if (s_opt.find(',')!=std::string::npos){
          pOpts->AlsaInPort=s_opt.substr(0,s_opt.find(',')+1);
          pOpts->AlsaOutPort=s_opt.substr(s_opt.find(',')+1,s_opt.length());
        }
        else {
          pOpts->AlsaOutPort=s_opt;
        }
      }
      break;
#   endif
#   ifdef HAVE_OSS        
    case longopt_oss:
      pOpts->MidiInt=INTERFACE_OSS;
      if(optarg) pOpts->OssDevNum=atoi(optarg);
      break;
#   endif
#   ifdef HAVE_GUILE
    case longopt_script: pOpts->GuileScripts.push_back(optarg); break;
#   endif   
    case 'm':{
      if(std::string(optarg).find('/')!=std::string::npos){
        std::string s_opt(optarg);      
        pOpts->MeterP=atoi(s_opt.substr(0,s_opt.find('/')+1).c_str());
        pOpts->MeterQ=atoi(s_opt.substr(s_opt.find('/')+1,s_opt.length()).c_str());
      }
      else {
        if (atoi(optarg)>99) {OPTION_ERROR("use slash to disambiguate meter "<<optarg);}
        pOpts->MeterP=atoi(optarg)/10;
        pOpts->MeterQ=atoi(optarg)%10;
      }
      if (Metronome::log_2(pOpts->MeterQ)<0) OPTION_ERROR("meter Q must be 2^n");
      CHECK_OPT_RANGE(MeterP,2,33);
      CHECK_OPT_RANGE(MeterQ,2,33);
      break;     
    }
    case longopt_lang:{
      std::string nm(optarg);
      int lang=0;
      while(Notator::Langs[lang]!=NULL){
        if(nm==Notator::Langs[lang]) {
          pOpts->Lang=lang;
          return 0;
        }
		lang++;
      }
      OPTION_ERROR("language `"<<nm<<"' not supported");
      break;
    }
    case 'k':{
      std::string nm(optarg);
      for(int i=0; i<35; i++){
		if(nm!=Notator::LangData[pOpts->Lang][i]) continue;
		if(abs((i%5)-2)>1){
        		OPTION_ERROR("double sharp/flat key is illegal (may require triple accidentals)");
		}
		pOpts->KeyBase=i/5;
		pOpts->KeyAcc=(i%5)-2;
		return 0;
      }
	  OPTION_ERROR("key ``"<<nm<<"'' not recognized");
      break;
    }
    case ARGP_KEY_ARG:
    case ARGP_KEY_END: break;
    default:
      return ARGP_ERR_UNKNOWN;  
  }
  return 0;
}

#undef CHECK_OPT_RANGE
#undef OPTION_ERROR
#undef CONS_OPTION_ERROR
#undef DO_OPTION_ERROR

struct argp argp_instance={options_vec,parse_option,args_doc,prog_doc};
