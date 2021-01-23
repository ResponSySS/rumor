/*

  Really Unintelligent Music transcriptOR
  main program, options and thread management

  Copyright (C) 2003 Vaclav Smilauer
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 as
  published by the Free Software Foundation.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

*/

#include"rumor.hh"

#include<pthread.h>
#include<iostream>
#include<sstream>
#include<list>
#include<argp.h>
#include<fstream>
#include<typeinfo>


MidiClient* pMidiClient;
Metronome* pMetronome;
Notator* pNotator;
Options* pOpts;

pthread_t* MidiClient::Thread=NULL;
void* MidiClient::ThreadLoop(void*){pMidiClient->Loop(); return NULL;}
void MidiClient::CreateThread(void){
  Thread=new pthread_t;
  pthread_create(Thread,NULL,MidiClient::ThreadLoop,NULL);
}

volatile int Terminating=0;
void SigHandler(int sig){Terminating=1;}

#ifdef HAVE_GUILE
#define SCRIPT_ERROR_HANDLER(func) \
  catch (script_error& e){\
  LOG(0,"SCRIPT ERROR (" func " aborted): "<<e.what()<<"\n"); \
  return to_SCM(0);}
  
SCM gh_rumor_rhythms_wrap(SCM grain, SCM P, SCM Q, SCM rhys) try{
  assert(pNotator);
  return pNotator->gh_rumor_rhythms(grain,P,Q,rhys);
} SCRIPT_ERROR_HANDLER("rumor-rhythms")

SCM gh_rumor_pitches_wrap(SCM plist) try{
  assert(pNotator);
  return pNotator->gh_rumor_pitches(plist);
} SCRIPT_ERROR_HANDLER("rumor-pitches")

SCM gh_rumor_kbd_wrap(SCM repl, SCM trsp, SCM rest) try{
  assert(pMidiClient);
  if(typeid(*pMidiClient)!=typeid(KbdMidiClient))
    throw script_error("not in keyboard emulation mode (--kbd)");
  return dynamic_cast<KbdMidiClient*>(pMidiClient)->gh_rumor_kbd(repl,trsp,rest);
} SCRIPT_ERROR_HANDLER("rumor-kbd")

SCM gh_rumor_beats_wrap(SCM channel, SCM beat1st, SCM beats) try{
  assert(pMetronome);
  return pMetronome->gh_rumor_beats(channel, beat1st, beats);
} SCRIPT_ERROR_HANDLER("rumor-beats")
#undef SCRIPT_ERROR_HANDLER
#endif


void main2(int,char**);

int main(int argc,char**argv){
#ifdef HAVE_GUILE
  gh_enter(argc,argv,main2);
#else
  main2(argc,argv);
#endif  
  return 0;
}


//this is the REAL main
void main2(int argc, char **argv) try{


  //FIXME: detect locale and choose appropriate notation style here
  /*setlocale(LC_MESSAGES,"");
  bindtextdomain(PACKAGE,LOCALEDIR);
  textdomain(PACKAGE);
  */

	
  pOpts=new Options;
  if(argp_parse(&argp_instance,argc,argv,0,0,0)!=0||
    pOpts->SanityCheck()!=0){
    exit(1);
    /*char *dummy_argv[]={"rumor","--usage"};
    /argp_parse(&argp_instance,2,dummy_argv,0,0,0);*/
  }  

  pMetronome=new Metronome();
  pNotator=new Notator();

  switch(pOpts->MidiInt){
#   ifdef HAVE_ALSA
    case INTERFACE_ALSA: pMidiClient=new AlsaMidiClient(); break;
#   endif
#   ifdef HAVE_OSS      
    case INTERFACE_OSS:  pMidiClient=new OssMidiClient(); break;      
#   endif
  }
  if (pOpts->KbdMidi){
    pMidiClient=new KbdMidiClient(pMidiClient); //former pMidiClient will be deleted by KbdMidiClient
  }

# ifdef HAVE_GUILE
  gh_new_procedure("rumor-rhythms",(SCM (*)())gh_rumor_rhythms_wrap,3,0,1);
  gh_new_procedure("rumor-kbd",    (SCM (*)())gh_rumor_kbd_wrap,2,0,1);
  gh_new_procedure("rumor-pitches",(SCM (*)())gh_rumor_pitches_wrap,0,0,1);
  gh_new_procedure("rumor-beats",  (SCM (*)())gh_rumor_beats_wrap,3,0,0);
  for(std::list<std::string>::iterator I=pOpts->GuileScripts.begin(); I!=pOpts->GuileScripts.end(); I++){
    gh_load(I->c_str());
  }  
# endif      

  if (pOpts->SampleSheet){pNotator->SampleSheet();exit(0);}

  LOG(1,"Hit ^-C to exit. In emergency, try violent keystroke ^-\\.\n");

  pMidiClient->CreateThread();
  // attention: no semicolon after LOG, it is a macro ending with }
  if (pOpts->Wait) LOG(2,"Metronome waits for MIDI at beat no. "<<pOpts->WaitBeat<<".\n")
  else if (pOpts->Flat) LOG(2,"Flat mode, metronome will never start.\n")
  else pMetronome->CreateThread();

  signal(SIGINT,SigHandler);
  sigset_t smask; sigemptyset(&smask); //no signals blocked
  while (!Terminating) sigsuspend(&smask);

  if(Metronome::Thread){
    LOG(3,"Cancelling metronome thread...\n");  
    pthread_cancel(*Metronome::Thread);
    pthread_join(*Metronome::Thread,NULL);
    LOG(2,"Metronome thread finished\n");
  } else LOG(3,"Metronome thread never ran\n");
  LOG(3,"Cancelling MIDI thread...\n");
  pthread_cancel(*MidiClient::Thread);
  pthread_join(*MidiClient::Thread,NULL);
  LOG(2,"MIDI thread finished\n");

  delete pMetronome;
  delete pMidiClient;    
  delete pNotator;    

  LOG(2,"Exitting peacefully after ^C...\n");
  exit(0);
  
}catch(std::exception& e){
  std::cerr<<std::endl<<"Exception: "<<e.what()<<std::endl;
  exit(1);
}

