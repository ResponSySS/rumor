/*

 Really Unintelligent Music transcriptOR
 OSS interface
 
 Thanks to Craig Stuart Sapp for
 his didactically great code (portions of it used here)
 (http://ccrma-www.stanford.edu/~craig/articles/linuxmidi/)
  
*/

#include"rumor.hh"

#ifdef HAVE_OSS

#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

OssMidiClient::OssMidiClient(void){
   const char* dev="/dev/sequencer";
   int n_midi = 0;
   seqfd=open(dev,O_RDWR,0);
   if (seqfd < 0)
     throw std::runtime_error(std::string("Can not open ").append(dev));
   if(ioctl(seqfd,SNDCTL_SEQ_NRMIDIS,&n_midi)!=0)
     throw std::runtime_error(std::string("Can not open MIDI devices on ").append(dev));
   LOG(1,"There are "<<n_midi<<" MIDI output devices on "<<dev<<" available.\n");
   device=pOpts->OssDevNum;
   if (device>=n_midi){
     std::ostringstream oss;
     oss<<"Invalid device number (valid devices are 0.."<<n_midi-1<<")";
     throw std::runtime_error(oss.str());
   }
} 
OssMidiClient::~OssMidiClient(){
  close(seqfd);
}

void OssMidiClient::Loop(void){
  unsigned char packet[4];
  while(1){
    read(seqfd, &packet, sizeof(packet)); //read is thread cancellation point
    unsigned char cmd=packet[1]&0xf0;
    if (packet[0]==SEQ_MIDIPUTC) {
      if ((cmd==0x90)||(cmd==0x80)){
        read(seqfd, &packet, sizeof(packet));
        write(seqfd,&packet,sizeof(packet));             
        unsigned char pitch=packet[1];
        read(seqfd, &packet, sizeof(packet));        
        write(seqfd,&packet,sizeof(packet));             
        unsigned char velocity=packet[1];
        if((cmd==0x90)&&(velocity!=0)) {
          pNotator->NoteOn(pitch);
        }  
        else if((cmd==0x90)||(cmd==0x80)) pNotator->NoteOff(pitch);
      }
    }
    write(seqfd,&packet,sizeof(packet));     
  }
}

void OssMidiClient::PlayNote(unsigned pitch, unsigned velocity,
                        unsigned instrument, unsigned channel){

   unsigned char ipacket[8];
   ipacket[0]=SEQ_MIDIPUTC;
   ipacket[1]=0xc0; // patch change command
   ipacket[2]=device;
   ipacket[3]=channel;

   ipacket[4]=SEQ_MIDIPUTC;
   ipacket[5]=instrument & 0xff;
   ipacket[6]=device;
   ipacket[7]=0;
   write(seqfd,ipacket,8);
   
   unsigned char npacket[12];

   npacket[0]=SEQ_MIDIPUTC;
   npacket[1]=0x90; // note-on command
   npacket[2]=device;
   npacket[3]=channel;

   npacket[4]=SEQ_MIDIPUTC;
   npacket[5]=pitch;
   npacket[6]=device;
   npacket[7]=0;

   npacket[8]=SEQ_MIDIPUTC;
   npacket[9]=velocity;
   npacket[10]=device;
   npacket[11]=0;
   write(seqfd,npacket,12);  
}


#endif /* HAVE_OSS */
