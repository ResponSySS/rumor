/*

 Really Unintelligent Music transcriptOR
 ALSA interface
  
*/

#include"rumor.hh"

#ifdef HAVE_ALSA


void AlsaMidiClient::PlayNote(unsigned pitch, unsigned velocity, unsigned instrument, unsigned channel){
  snd_seq_event_t* ev_inst=new snd_seq_event_t;
  snd_seq_ev_set_subs(ev_inst);
  snd_seq_ev_set_direct(ev_inst);
  snd_seq_ev_set_pgmchange(ev_inst,channel,instrument);
  snd_seq_ev_set_source(ev_inst,OutPort);
  snd_seq_event_output_direct(SeqHandle,ev_inst);

  snd_seq_event_t* ev_note=new snd_seq_event_t;
  snd_seq_ev_set_subs(ev_note);
  snd_seq_ev_set_direct(ev_note);
  snd_seq_ev_set_noteon(ev_note,channel,pitch,velocity);
  snd_seq_ev_set_source(ev_note,OutPort);
  snd_seq_event_output_direct(SeqHandle,ev_note);
}

AlsaMidiClient::AlsaMidiClient(void)
{
  TRFL;
  if ((snd_seq_open(&SeqHandle, "default", SND_SEQ_OPEN_DUPLEX, 0)<0)||
      ((Client=snd_seq_client_id(SeqHandle))<0))
  {
    throw std::runtime_error("Error opening ALSA sequencer.");
  }
  LOG(1,"We are ALSA client "<<Client<<"\n");
  snd_seq_set_client_name(SeqHandle, "Rumor Client");

  const char *instr="Rumor IN";
  if((InPort=snd_seq_create_simple_port(SeqHandle, instr,
     SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
     SND_SEQ_PORT_TYPE_APPLICATION))<0)
    throw std::runtime_error("Error creating sequencer IN port.");


  const char *outstr="Rumor OUT";
  if((OutPort=snd_seq_create_simple_port(SeqHandle, outstr,
     SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
     SND_SEQ_PORT_TYPE_APPLICATION))<0)
    throw std::runtime_error("Error creating sequencer OUT port.");

  SetupConnections();
}

void AlsaMidiClient::SetupConnections(void){
  TRFL;
  snd_seq_addr_t FromMidi,ToMe, FromMe, ToMidi;
  snd_seq_port_subscribe_t *subs_Midi_Me, *subs_Me_Midi;

  if(snd_seq_parse_address(SeqHandle,&FromMidi,pOpts->AlsaInPort.c_str())<0|| 
     snd_seq_parse_address(SeqHandle,&ToMidi,pOpts->AlsaOutPort.c_str())<0){
//    throw std::runtime_error("ALSA port address parsing error");     
    std::cerr<<"ALSA port address parsing error; connect manually using `aconnect'"<<std::endl;
    return;
  }
  ToMe.client=FromMe.client=Client;
  ToMe.port=InPort; FromMe.port=OutPort;
  
  snd_seq_port_subscribe_alloca(&subs_Midi_Me);
  snd_seq_port_subscribe_set_sender(subs_Midi_Me, &FromMidi);
  snd_seq_port_subscribe_set_dest(subs_Midi_Me, &ToMe);
  
  snd_seq_port_subscribe_alloca(&subs_Me_Midi);
  snd_seq_port_subscribe_set_sender(subs_Me_Midi,&FromMe);
  snd_seq_port_subscribe_set_dest(subs_Me_Midi,&ToMidi);
  
  if((pOpts->KbdMidi?false:
        snd_seq_get_port_subscription(SeqHandle, subs_Midi_Me)==0
        ||snd_seq_subscribe_port(SeqHandle, subs_Midi_Me)<0)
    ||(snd_seq_get_port_subscription(SeqHandle, subs_Me_Midi)==0
        ||snd_seq_subscribe_port(SeqHandle, subs_Me_Midi)<0)){
    LOG(0,"ALSA port connection error; do it manually using `aconnect'\n");
    return;
  }
}

void AlsaMidiClient::Loop(void){
  int npfd = snd_seq_poll_descriptors_count(SeqHandle, POLLIN);
  struct pollfd* pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(SeqHandle, pfd, npfd, POLLIN);
  while (1) {
    if (pthread_testcancel(),poll(pfd, npfd, 100000)>0){
      HandleMidiEvent();
    } 
  }
}


void AlsaMidiClient::HandleMidiEvent(void){
  snd_seq_event_t *ev;
  do {
    snd_seq_event_input(SeqHandle,&ev);
    snd_seq_ev_set_subs(ev);  
    snd_seq_ev_set_direct(ev);    
    if ((ev->type==SND_SEQ_EVENT_NOTEON)||(ev->type==SND_SEQ_EVENT_NOTEOFF)){
      //hidden noteoff: noteon && velocity==0
      if ((ev->type==SND_SEQ_EVENT_NOTEON)&&(ev->data.note.velocity))
        pNotator->NoteOn(ev->data.note.note);
      else pNotator->NoteOff(ev->data.note.note);
    }
    snd_seq_ev_set_source(ev,OutPort);
    snd_seq_event_output_direct(SeqHandle,ev);
  } while (snd_seq_event_input_pending(SeqHandle, 0) > 0);
}


#endif /* HAVE_ALSA */
