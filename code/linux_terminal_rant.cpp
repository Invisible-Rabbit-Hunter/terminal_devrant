#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <gumbo.h>
#include <curses.h>

#include "linux_terminal_rant.h"
#include "entities.c"

#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

void init_string(struct string *str) {
  str->length = 0;
  str->pointer = (char *)malloc(str->length + 1);
  if (str->pointer == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  str->pointer[0] = '\0';
}

void append_string(struct string *str, char *Append)
{
    size_t new_length = str->length + ArrayCount(Append);
    str->pointer = (char *)realloc(str->pointer, new_length+1);
    if (str->pointer == NULL) {
      fprintf(stderr, "realloc() failed\n");
      exit(EXIT_FAILURE);
    }
    memcpy(str->pointer+str->length, Append, ArrayCount(Append));
    str->pointer[new_length] = '\0';
    str->length = new_length;

}

size_t writefunc(void *pointer, size_t size, size_t nmemb, struct string *str)
{
  size_t new_length = str->length + size*nmemb;
  str->pointer = (char *)realloc(str->pointer, new_length+1);
  if (str->pointer == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(str->pointer+str->length, pointer, size*nmemb);
  str->pointer[new_length] = '\0';
  str->length = new_length;

  return size*nmemb;
}

uint
SearchForClass(GumboNode *Node, const char *ClassName, uint ResultCount, GumboNode **ResultArray)
{
    if(Node->type == GUMBO_NODE_ELEMENT)
    {
        GumboAttribute *ClassAttribute;
        if((ClassAttribute = gumbo_get_attribute(&Node->v.element.attributes, "class")) &&
           (strcmp(ClassAttribute->value, ClassName) == 0))
        {
            ResultArray[ResultCount++] = Node;
        }

        GumboVector* Children = &Node->v.element.children;
        for(uint ChildIndex = 0;
            ChildIndex < Children->length;
            ++ChildIndex)
        {
            ResultCount = SearchForClass((GumboNode *)(Children->data[ChildIndex]), ClassName, ResultCount, ResultArray);
        }
    }

    return ResultCount;
}

void removeSubstring(char *String, const char *ToRemove)
{
    while((String = strstr(String, ToRemove)))
    {
        memmove(String, String + strlen(ToRemove), 1 + strlen(String + strlen(ToRemove)));
    }
}

void insertString(char *String, char *ToInsert, int Offset)
{
    memmove(String + Offset, String + Offset + strlen(ToInsert), strlen(String + Offset + strlen(ToInsert)));
}

char *
GetNodeText(GumboNode *Node)
{
    uint StartPosition = Node->v.element.start_pos.offset + Node->v.element.original_tag.length;
    uint EndPosition = Node->v.element.end_pos.offset;
    uint StringLength = EndPosition - StartPosition;
    char *Result = (char *)Node->v.element.original_tag.data + Node->v.element.original_tag.length;
    *(Result + StringLength) = '\0';

    decode_html_entities_utf8(Result, 0);
    removeSubstring(Result, "<br />");

    return(Result);
}

uint
LoadRant(CURL *curl, Rant *Rant)
{
    CURLcode res;

    char *URL = Rant->URL;

    struct string str;

    init_string(&str);

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
    res = curl_easy_perform(curl);

    GumboOutput *Output = gumbo_parse(str.pointer);

    GumboNode *RantArr[1024] = {};
    uint RantCount;

    RantCount = SearchForClass(Output->root, "rantlist-title", 0, RantArr);

    for(uint RantArrIndex = 0;
        RantArrIndex < RantCount;
        ++RantArrIndex)
    {
        GumboNode *RantNode = RantArr[RantArrIndex];

        char *Links[] = ;

        printf("%s\n", GetNodeText(RantNode));
    };

    // strcpy(Rants[RantsCount++].RantText, GetNodeText(RantList[RantIndex], str.pointer));

    return 0;
}

uint
GatherRants(CURL *curl, uint RantsCount, Rant Rants[], char *Feed, uint PageIndex)
{
    CURLcode res;

    uint RantCount = 0;

    struct string str;

    init_string(&str);

    char URL[50];

    sprintf(URL, "https://www.devrant.io/feed/%s/%d", Feed, PageIndex);

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
    res = curl_easy_perform(curl);

    GumboOutput *Output = gumbo_parse(str.pointer);

    GumboNode *RantList[20] = {};
    RantCount = SearchForClass(Output->root, "rantlist-title", 0, RantList);
    if(RantCount)
    {
        for(uint RantIndex = 0;
            RantIndex < RantCount;
            ++RantIndex)
        {
            GumboNode *Rant = RantList[RantIndex];

            sprintf(Rants[RantsCount].URL, "https://www.devrant.io%s", gumbo_get_attribute(&Rant->v.element.attributes, "href")->value);
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, Output);
    free(str.pointer);

    return RantCount;
}

void WordWrap(char *Text, uint width)
{
    int CharacterIndex = 0;
    int LineCharacterIndex = 0;
    int PrevSpaceIndex = 0;
    for(char Character = *Text;
        Character;
        Character = *(Text + CharacterIndex))
    {
        if((LineCharacterIndex != 0) &&
           ((LineCharacterIndex % width) == 0))
        {
            if(Character != '\n')
            {
                *(Text + PrevSpaceIndex) = '\n';
            }
        }

        if(Character == ' ')
        {
            PrevSpaceIndex = CharacterIndex;
        }

        if(Character == '\n')
        {
            LineCharacterIndex = 0;;
        }
        else
        {
            ++LineCharacterIndex;
        }

        ++CharacterIndex;

    }
}

int main(int argc, char *argv[])
{
    CURL *curl;

    curl = curl_easy_init();

    if(curl)
    {

        Rant Rants[40] = {};
        uint RantCount = 0;

        RantCount += GatherRants(curl, RantCount, Rants, (char *)"recent", 1);

        LoadRant(curl, &Rants[0]);

#if 0
        WINDOW *RantWindow;

        initscr();
        raw();
        keypad(stdscr, true);
        noecho();
        cbreak();
        timeout(1);

        RantCount += GatherRants(curl, RantCount, Rants, (char *)"recent", 1);

        bool Running = true;
        uint CurrentRantIndex = 0;
        Rant CurrentRant = Rants[CurrentRantIndex];

        while(Running)
        {
            int width = 80, height = 24;

            int Input = getch();
            refresh();

            switch(Input)
            {
                case 27:
                case 3:
                case 0:
                {
                    printf("(%d:%c)\n", Input, Input);
                    Running = false;
                } break;

                case KEY_LEFT:
                case 'n':
                {
                    CurrentRantIndex = MOD(--CurrentRantIndex, RantCount);
                    CurrentRant = Rants[CurrentRantIndex];
                } break;

                case KEY_RIGHT:
                case 'p':
                {
                    CurrentRantIndex = MOD(++CurrentRantIndex, RantCount);
                    CurrentRant = Rants[CurrentRantIndex];
                } break;
            }

            clear();

            WordWrap(CurrentRant.Text, width);

            printw("------------------------\n");
            printw("%s\n", CurrentRant.Text);
            printw("------------------------\n");

            refresh();

        }
        endwin();
#endif
        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    return 0;
}
