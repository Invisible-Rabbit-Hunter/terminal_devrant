#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "json/json.c"
#include "json/json-builder.c"

#include "linux_terminal_rant.h"

struct string
{
    char *Text;
    size_t Length;
};

struct string *
InitString()
{
    struct string *Result = calloc(1, sizeof(char));

    return Result;
}

size_t
write_cb(void *ptr, size_t size, size_t nmemb, struct string *String)
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

json_value *
GetRants(CURL *curl, char *API, char *Feed, int32_t Count)
{
    char PATH[50] = {};
    char URL[100] = {};

    sprintf(PATH, "devrant/rants?app=3&sort=%s&limit=%d", Feed, Count);
    sprintf(URL, "%s/%s", API, PATH);

    struct string *Result = InitString();

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, Result);

    curl_easy_perform(curl);

    json_value *Rants = json_parse(Result->Text, Result->Length);

    free(Result->Text);

    return Rants;
}

void
PrintRants(json_value *Rants)
{
    Rants = json_get_object_value(Rants, "rants");

    int ArrayIndex;
    for(ArrayIndex = 0;
        ArrayIndex < Rants->u.array.length;
        ++ArrayIndex)
    {
        json_value *CurrentRant = json_get_array_value(Rants, ArrayIndex);

        printf("%s\n\n-\n\n", json_get_object_value(CurrentRant, "text")->u.string.ptr);
    }

}

int main(int argc, char **argv)
{
    char API[] = "https://www.devrant.io/api";
    CURL *curl = curl_easy_init();

    clock_t start = clock(), diff;

    json_value *Rants = GetRants(curl, API, "recent", 20);

    diff = clock() - start;

    float msec = (float)(diff * 1000000 / CLOCKS_PER_SEC)/1000000.0f;
    printf("Time taken: %f seconds\n", msec);

    //
    // if(json_get_object_value(Rants, "success"))
    // {
    //     PrintRants(Rants);
    // }
    //
    json_value_free(Rants);

    curl_easy_cleanup(curl);
    return 0;
}
