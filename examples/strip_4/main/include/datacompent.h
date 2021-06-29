#ifndef DATACOMPENT
#define DATACOMPENT

extern const char *REPORTED;
extern const char *CMD;
extern const char *CMD1;
extern const char *OUTPUT0;
extern const char *OUTPUT1;
extern const char *LOCAL_IP;
extern const char *REQUESTID;
extern char *KEYS[5];

typedef struct data_res
{
    int cmds[5];
    char *output0; //output0
    char *output1; //output0
} data_res;

//ini data then mqtt http udp and...
void data_initialize();
char *data_bdjs_reported(const char *key, int number);
char *data_bdjs_reported_string(const char *key, const char *number);
void data_free(void *object);
data_res *data_decode_bdjs(char *data);
char *data_bdjs_request(const char *id);
char *data_get_sysmes();
char *data_get_dsdata();
#endif
