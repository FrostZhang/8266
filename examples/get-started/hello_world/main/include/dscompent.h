#ifndef DSCOMPENT
#define DSCOMPENT

typedef struct
{
    int is_on;
    gpio_num_t gpio;
}dsres;

void ontick(struct tm *timeinfo);

#endif