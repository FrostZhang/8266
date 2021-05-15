#include "navcompent.h"
#include "stdio.h"
#include "stdlib.h"
#include "sb.h"
#include <esp_log.h>
#include "start.h"

const char *shtml = "<!DOCTYPE html>\
<html lang='ch'>\
<link rel='icon' href='data:;base64,='>\
<head>";
const char *title = "<title>%s</title>\
<meta charset='utf-8'>\
<meta name='viewport' content='width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0'>";

const char *style = "<link rel='stylesheet' type='text/html' href='style'>";

const char *ehead = "</head>";
const char *sbody = "<body>";

const char *ebody = "</body>";
const char *ehtml = "</html>";
const char *h1 = "<h1>Asher-Link\
<span>拥抱智能、享受生活</span>\
</h1>";

const char *form0_1 = "<form id='mqttform' action='' method='post'  target='hiddenIFrame'>\
<label>\
<span>控制goio4 :</span>\
<select  name='open'  class='form-control' onchange='submitForm();' >\
<option value='0'>关闭</option>";

const char *form0_2="<option value='1' %s>打开</option>";

const char *form0_3="</select></label></form>";

const char *formmqtt1 = "<form action='' method='post' target='hiddenIFrame'>";
const char *formmqtt2 = "<label>\
<span>MQTT 账号 :</span>\
<input type='text' value='%s' maxlength='20' name='mqttzz' placeholder='MQTT账号' />\
</label>";
const char *formmqtt3 = "<label>\
<span>MQTT 密码 :</span>\
<input type='password' value='%s' maxlength='20' placeholder='MQTT密码' name='mqttmm' />\
</label>";
const char *formmqtt4 = "<label>\
<span>&nbsp;</span>\
<input type='submit' class='button' value='修改MQTT并重启' />\
</label></form>";

const char *form1_5 = "<label>\
<span>选择 :</span>\
<select name='selection'>\
<option value='Job Inquiry'>Job Inquiry</option>\
<option value='General Question'>General Question</option>\
</select>\
</label>";

const char *form1_6 = "<form action='' method='post' target='hiddenIFrame'>\
<label><span>定时开:</span>\
<label><input name='ds' type='checkbox' value='1' checked='0'/>周一</label>\
<label><input name='ds' type='checkbox' value='2' checked='%s'/>周二</label>\
<label><input name='ds' type='checkbox' value='3' checked='%s'/>周三</label>\
<label><input name='ds' type='checkbox' value='4' checked='%s'/>周四</label>\
<label><input name='ds' type='checkbox' value='5' checked='%s'/>周五</label>\
<label><input name='ds' type='checkbox' value='6' checked='%s'/>周六</label>\
<label><input name='ds' type='checkbox' value='7' checked='%s'/>周日</label>\
</label>";

const char *form1_n = "<label>\
<span>&nbsp;</span>\
<input type='submit' class='button' value='设置并重启' />\
</label></form>";

const char *eform = "</form>";
const char *iframe = "<iframe style='display:none' name='hiddenIFrame' id='hiddenIFrame'/>";

const char *jqueryscript ="<script src='/jquery'></script>";

const char *script = "<script type='text/javascript'>\
function submitForm() { $('#mqttform').submit();}";
const char *escript = "</script>";
const char *cus = "Esp8266";

char *htmlindex()
{
    extern char *mqttusername;
    extern char *mqttpassword;
    extern char *wifissid;

    StringBuilder *sb = sb_create();
    sb_append(sb, shtml);
    sb_appendf(sb, title, cus);
    sb_append(sb, jqueryscript);
    sb_append(sb, script);
    sb_append(sb, escript);
    sb_append(sb, style);
    sb_append(sb, ehead);
    sb_append(sb, sbody);
    sb_append(sb, h1);
    sb_append(sb, form0_1);

    int isopen_4  = get_isopen(4);
    printf("get_isopen  isopen_4 : %d\n",isopen_4);
    if (isopen_4 == 0)
        sb_append(sb, form0_2);
    else
        sb_appendf(sb, form0_2,"selected");
    sb_append(sb, form0_3);

    sb_append(sb, formmqtt1);
    sb_appendf(sb, formmqtt2, mqttusername);
    sb_appendf(sb, formmqtt3, mqttpassword);
    sb_append(sb, formmqtt4);

    //sb_append(sb, form1_5);
    //sb_append(sb, form1_6);
    //sb_appendf(sb, form1_3,mqttpassword);
    //sb_append(sb, form1_4);
    //sb_append(sb, form1_n);
    //sb_append(sb, eform);
    sb_append(sb, iframe);

    sb_append(sb, ebody);
    sb_append(sb, ehtml);
    char *out = sb_concat(sb);
    sb_free(sb);
    return out;
}