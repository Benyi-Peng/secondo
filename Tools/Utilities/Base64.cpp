
#include <assert.h>
#include <string>
#include <Base64.h>
#include <iostream>

char 
Base64::base64Alphabet[] = {'A','B','C','D','E','F','G','H','I','J','K',
                        'L','M','N','O','P','Q','R','S','T','U','V',
	  	        'W','X','Y','Z','a','b','c','d','e','f','g',
		        'h','i','j','k','l','m','n','o','p','q','r','s',
		        't','u','v','w','x','y','z','0','1','2','3',
		        '4','5','6','7','8','9','+','/'};


Base64::Base64() {

  currentPos = 5; // needed to fill the outbuffer for encoding
  endReached = false;
  filled = 3;
}



int
Base64::getIndex(char b){
 
 if(b>='A' && b<='Z')
     return b-'A';
  if(b>='a' && b<='z')
     return b-'a'+26;
  if(b>='0' && b<='9')
     return b-'0'+52;
  if(b=='+')
     return 62;
  if(b=='/')
     return 63;
  if(b=='=')
     return 0;
  return -1;
}


bool
Base64::isAllowed(char b){
  return getIndex(b)>=0;
}


bool
Base64::getNext(char& byte, istream& in) {
 
  static char inbuffer[5] = {0,0,0,0,0};
  static char outbuffer[3] = {0,0,0};
  char ch = 0;
  //static int k = 0;    

  if( currentPos<filled ) {  //  data is in the buffer
      byte = outbuffer[currentPos];
      currentPos++;
      return true;
  }

  if(endReached) // no more data to load
     return false;

  // fill the inbuffer
  for(int i=0;i<4;i++){ // get the next allowed input bytes
      in.get(ch);

      while( !in.eof() && !isAllowed(ch) ) { // override not allowed bytes
	in.get(ch);
      }
      if( in.eof() && (i<4) ) { // not a full quadrupel found
	  cerr << "Base64::decode - unexpected end of input!" << endl;
	  exit(1);
      }
      if( in.eof() ){ // end of input
	 endReached=true;
	 return true;
      }
     inbuffer[i] = ch;  //store value
  }

  //cerr << k << ": " << inbuffer << endl;
  //k++;

  // cat inbuffer
  int all =  (getIndex(inbuffer[0])<<18) +
	    (getIndex(inbuffer[1])<<12) +
	    (getIndex(inbuffer[2])<<6)  +
	     getIndex(inbuffer[3]);

  // extract outbytes
  for(int i=2;i>=0;i--){
      outbuffer[i]=(char)(all & 255);
      all = all >> 8;
  }

  filled=3; // three bytes in outbuffer
  if(inbuffer[3]=='='){
      filled=2;
      endReached=true;
  }
  if(inbuffer[2]=='='){
      endReached=true;
      filled=1;
  }
  currentPos = 1; // the first byte is given now
  byte = outbuffer[0];
  return true;

}

void
Base64::decode(istream& in, ostream& out) {
       
  char ch = 0;
  while ( getNext(ch, in) ) {
     out.put(ch);
  }
  /*
  if( !in.eof() ) {
    ios_base::iostate s = in.rdstate();
    cerr << "End of input stream not reached!" << endl;
    cerr << "Stream Status:" << endl;
    if (s & ios_base::eofbit) cerr << "eofbit: End of file reached" << endl;
    if (s & ios_base::failbit) cerr << "failbit: last I/O operation caused an error" << endl;
    if (s & ios_base::badbit) cerr << "badbit: Illegal operation" << endl;
    exit(1);
  }
  */
}




void
Base64::encode(char* buffer, string& text, int length) {

  assert (length <= 54 ); // 54 * 4/3 = 72, a linebreak is only inserted after 72 bytes

  static char resultbuffer[73];
  
  char* result =0;
  unsigned char *data = 0;
  int pos = 0, outPos = 0;

  while (length > 0) {
     data = (unsigned char*) &(buffer[pos]);
     // set all undefined byte to zero
     for(int i=length;i<3;i++) {
	data[i] = 0;
     }
     // code all given bytes in a single integer
     // this code must be modified for windows, since endianess is important.
     int all = 65536*data[0]+ 256*data[1]+ data[2];
     
     result = &(resultbuffer[outPos]);
     // extract the 6 bit values
     for(int i=3;i>=0;i--){
       int index = (all & 63);
       assert( (index >= 0) && (index < 64) );
       result[i] = base64Alphabet[index];
       all = all >> 6;
     }
     for(int i=0;i<3-length;i++) {
	result[3-i]='=';
     }

     length -= 3;
     pos += 3;
     outPos += 4;
  }
  
  result[4] = 0;
  string resultStr(&(resultbuffer[0]));
  text = resultStr + "\n";    

}

void
Base64::encodeStreams(istream& in, ostream& out) {

  string textFragment="";
  const int inLength = 54; 
  char buf[inLength+1]; 
  buf[inLength] = 0;
  int readFaults = 0;                 

  do {

    in.read(buf, inLength);
    int noBytes = in.gcount();
    if (noBytes != inLength) {
       readFaults++;
    }
    assert( readFaults <= 1 );
    encode(buf, textFragment, noBytes);
    
    out << textFragment;

 } while (in.good());

}
