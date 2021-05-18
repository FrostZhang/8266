#ifndef DATACOMPENT
#define DATACOMPENT

extern const char *REPORTED;   
extern const char *CMD;             
extern const char *CMD1;           
extern const char *OUTPUT0;     
extern const char *OUTPUT1;     
extern const char *LOCAL_IP;   
extern const char *REQUESTID; 

typedef struct data_res
{
    int cmd; //cmd
    char* cmd1;
}data_res;

//ini data then mqtt http udp and...
void datacompentini();
char* setreported(const char *key,int number);
char* setreported2(const char *key,const char* number);
void datafree(void *object);
data_res* getreported(char *data);
char *setrequest(const char* id);

#endif
