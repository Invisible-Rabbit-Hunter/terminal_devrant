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

void replaceSubstring(char *String, const char *ToRemove, const char *ToReplace)
{
    while((String = strstr(String, ToRemove)))
    {
        memmove(String + strlen(ToReplace), String + strlen(ToRemove), 1 + strlen(String + strlen(ToRemove)));
        memcpy(String, ToReplace, strlen(ToReplace));
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
    replaceSubstring(Result, "<span>\n\t                                \t", "(");
    replaceSubstring(Result, "</span>", ")");

    return(Result);
}

void
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
    GumboNode *UserList[20] = {};
    uint RantCount;

    RantCount = SearchForClass(Output->root, "rantlist-title", 0, RantArr);
    SearchForClass(Output->root, "username-details", 0, UserList);


    Rant->Content.Text = GetNodeText(RantArr[0]);
    Rant->Content.User = GetNodeText(UserList[0]);
    for(uint RantArrIndex = 1;
        RantArrIndex < RantCount;
        ++RantArrIndex)
    {
        GumboNode *RantNode = RantArr[RantArrIndex];

        Rant->Comments[RantArrIndex - 1].Text = GetNodeText(RantNode);
        Rant->Comments[RantArrIndex - 1].User = GetNodeText(UserList[RantArrIndex]);

        ++Rant->CommentCount;
    };

    Rant->Loaded = true;
}

uint
GatherRants(CURL *curl, uint RantsCount, Rant Rants[], char Feed[], uint PageIndex)
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

            Rants[RantsCount].Loaded = false;

            sprintf(Rants[RantsCount].URL, "https://www.devrant.io%s", gumbo_get_attribute(&Rant->v.element.attributes, "href")->value);

            ++RantsCount;
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, Output);
    free(str.pointer);

    return RantCount;
}

// wrap: take a long input line and wrap it into multiple lines
void wrap(char s[], const int wrapline)
{
    int i, k, wraploc, lastwrap;

    lastwrap = 0; // saves character index after most recent line wrap
    wraploc = 0; // used to find the location for next word wrap

    for (i = 0; s[i] != '\0'; ++i, ++wraploc) {

        if (wraploc >= wrapline) {
            for (k = i; k > 0; --k) {
                // make sure word wrap doesn't overflow past maximum length
                if (k - lastwrap <= wrapline && s[k] == ' ') {
                    s[k] = '\n';
                    lastwrap = k+1;
                    break;
                }
            }
            wraploc = i-lastwrap;
        }

    } // end main loop
}

int main(int argc, char *argv[])
{
    CURL *curl;

    curl = curl_easy_init();

    if(curl)
    {

        void *RantMemory = malloc(sizeof(Rant)*40);

        Rant *Rants = (Rant *)RantMemory;
        uint RantCount = 0;
        WINDOW *RantWindow;

        initscr();
        raw();
        keypad(stdscr, true);
        noecho();
        cbreak();
        // timeout(10);
        curs_set(0);

        RantCount += GatherRants(curl, RantCount, Rants, (char *)"recent", 1);
        // RantCount += GatherRants(curl, RantCount, Rants, (char *)"top", 2);

        bool Running = true;

        uint CurrentRantIndex = 0;
        Rant *CurrentRant = Rants + CurrentRantIndex;
        LoadRant(curl, CurrentRant);

        int CurrentTopLine = 0;

        while(Running)
        {
            int WidthSpacing = 2;
            int HeightSpacing = 1;
            int width, height;
            getmaxyx(stdscr, height, width);
            clear();

            // WordWrap(CurrentRant->Content.Text, width);


            char LineBuffer[1000][width];
            int LineCount = 0;

            sprintf(LineBuffer[LineCount++], "----%s----\n", CurrentRant->Content.User);

            char *pch, *str;
            str = strdup(CurrentRant->Content.Text);
            wrap(str, width - (2*WidthSpacing));
            pch = strtok(str,"\n");
            while (pch != NULL)
            {
                sprintf(LineBuffer[LineCount++], "%s\n", pch);
                pch = strtok (NULL, "\n");
            }

            // sprintf(LineBuffer[LineCount++], "%s\n", CurrentRant->Content.Text);
            sprintf(LineBuffer[LineCount++], "------------------------\n");

            for(int CommentIndex = 0;
                CommentIndex < CurrentRant->CommentCount;
                ++CommentIndex)
            {
                sprintf(LineBuffer[LineCount++], "----%s----\n", CurrentRant->Comments[CommentIndex].User);

                str = strdup(CurrentRant->Comments[CommentIndex].Text);
                wrap(str, width - (2*WidthSpacing));
                pch = strtok(str,"\n");
                while (pch != NULL)
                {
                    sprintf(LineBuffer[LineCount++], "%s\n",pch);
                    pch = strtok (NULL, "\n");
                }

                // sprintf(LineBuffer[LineCount++], "%s\n", );

                sprintf(LineBuffer[LineCount++], "------------------------\n");
            }


            for(int i = 0;
                i < HeightSpacing;
                ++i)
            {
                printw("\n");
            }

            for(int LineIndex = CurrentTopLine;
                (LineIndex < LineCount) && (LineIndex < (height + CurrentTopLine));
                ++LineIndex)
            {
                for(int i = 0;
                    i < WidthSpacing;
                    ++i)
                {
                    printw(" ");
                }

                printw("%s", LineBuffer[LineIndex]);
            }

            for(int i = 0;
                i < HeightSpacing;
                ++i)
            {
                printw("\n");
            }

            int Input = getch();

            switch(Input)
            {
                case 27:
                case 3:
                case 0:
                {
                    Running = false;
                } break;

                case KEY_LEFT:
                {
                    CurrentRantIndex = MOD(--CurrentRantIndex, RantCount);
                    CurrentRant = Rants + CurrentRantIndex;
                    if(!CurrentRant->Loaded)
                    {
                        LoadRant(curl, CurrentRant);
                    }
                } break;

                case KEY_RIGHT:
                {
                    CurrentRantIndex = MOD(++CurrentRantIndex, RantCount);
                    CurrentRant = Rants + CurrentRantIndex;
                    if(!CurrentRant->Loaded)
                    {
                        LoadRant(curl, CurrentRant);
                    }
                } break;

                case KEY_UP:
                {
                    if(CurrentTopLine > 0)
                    {
                        --CurrentTopLine;
                    };
                } break;

                case KEY_DOWN:
                {
                    if((CurrentTopLine < height) && (CurrentTopLine < LineCount))
                    {
                        ++CurrentTopLine;
                    };
                } break;

                default:
                {
                    printf("\n...%d...\n", Input);
                    printw("\n...%d...\n", Input);
                } break;
            }

            refresh();
        }
        endwin();

        free(RantMemory);

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    return 0;
}
