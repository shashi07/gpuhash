#include <stdio.h>
#include <iostream>

#include <fstream>
#include "sha1.h"
#define BUFLEN 500*1024*1024

using namespace std;
using namespace sha1;

int readFile(char* filename, unsigned char * &buffer,int &charRead,int offset=0){

    ifstream fileObject;
    fileObject.open(filename, ios::in|ios::binary);

    if(fileObject.is_open()){

        fileObject.seekg (0, fileObject.end);
        int length = fileObject.tellg();
        fileObject.seekg (0, fileObject.beg);
        int l;
        if( offset >= length )
			return 1;
        if(length-offset >= BUFLEN){
            
            l =BUFLEN;
            //buffer = (unsigned char *)malloc((l)*sizeof(unsigned char));
			buffer = new unsigned char[l];
        }
        else{
			l= length-offset;
			cout<<l<<endl;
            //buffer = (unsigned char *)malloc((l)*sizeof(unsigned char));
            buffer = new unsigned char[l];

			buffer[0]='S';
			buffer[l-1] = 'B';
        }
        

        fileObject.seekg(offset);
        fileObject.read((char*)buffer,l);
        charRead = fileObject.gcount();
        fileObject.close();

       
        return 0;
    }

        return -1;

}


int main()
{

    unsigned char *data1=NULL;
    int charRead,offset=0,retStatus;
    unsigned char hash[20];
    while(1){
        retStatus = readFile("/home/ajay/test2.txt",data1,charRead,offset);
        offset+= charRead;
        if (retStatus == -1 || retStatus == 1)
            break;
        cout<<"aa gaya";
        int blocks = charRead/(1024*1024);
        //cout<<charRead-1<<endl;
        //cout<< data1[311341481]<<endl;
        for (int i=0;i<blocks;i++)
            calc(&data1[i*1024*1024],1024*1024,hash);

//        free(data1);
		delete data1;
        if(retStatus==1)
            break;
    }
    printf("Completed");
    return 1;
}
