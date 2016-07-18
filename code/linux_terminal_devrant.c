#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "json/json.c"
#include "json/json-builder.c"

#include "linux_terminal_rant.h"

size_t
write_cb(void *ptr, size_t size, size_t nmemb, string *String)
{
    String->Text = realloc(String->Text, String->Length + (size*nmemb));
    strncpy(String->Text + String->Length, ptr, size*nmemb);
    String->Length += (size*nmemb);
    return(size*nmemb);
}

json_value *
json_get_object_value(json_value *Object, char *Key)
{
    int KeyIndex;
    for(KeyIndex = 0;
        KeyIndex < Object->u.object.length;
        ++KeyIndex)
    {
        if(strcmp(Object->u.object.values[KeyIndex].name, Key) == 0)
        {
            return Object->u.object.values[KeyIndex].value;
        }
    }

    return 0;
}

json_value *
json_get_array_value(json_value *Object, int32_t Index)
{
    return Object->u.array.values[Index];
}

int32_t
GetRants(CURL *curl, char *API, char *Feed, int32_t Count, int32_t skip, struct rant *Rants)
{
    char PATH[50] = {};
    char URL[100] = {};

    sprintf(PATH, "devrant/rants?app=3&sort=%s&limit=%d&skip=%d", Feed, Count, skip);
    sprintf(URL, "%s/%s", API, PATH);

    string Result = InitString();

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Result);

    curl_easy_perform(curl);

    json_value *JsonResult = json_parse(Result.Text, Result.Length);

    json_value *RantArray = json_get_object_value(JsonResult, "rants");

    if(Rants)
    {
        free(Rants);
    }

    Rants = calloc(Count, sizeof(struct rant));

    for(int32_t RantIndex = 0;
        RantIndex < Count;
        ++RantIndex)
    {
        Rants[RantIndex].Text = InitString();
        Append(&Rants[RantIndex].Text, json_get_array_value(RantArray, Count)->u.string.ptr, json_get_array_value(RantArray, Count)->u.string.length);
    }

    free(Result.Text);

    json_value_free(JsonResult);
    return json_get_object_value(JsonResult, "success")->u.boolean;
}

void
PrintRants(struct rant *Rants)
{
}

int main(int argc, char **argv)
{
    char API[] = "https://www.devrant.io/api";
    CURL *curl = curl_easy_init();

    struct rant *Rants;
    int32_t Success = GetRants(curl, API, "recent", 20, 0, Rants);

    if(Success)
    {
        PrintRants(Rants);
    }

    free(Rants);

    curl_easy_cleanup(curl);
    return 0;
}
