#include "devrant_cli.h"
#include "sqlite3.h"

COMMAND_FUNC(show)
{
    sqlite3 *db;
    char *dbErrMsg = 0;
    s32 rc;

    rc = sqlite3_open("rants.db", &db);

    if(rc)
    {
        printf("Could not open db file: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
    }

    FILE *fp = fopen("test.rant", "rb");

    fseek(fp, 0L, SEEK_END);
    s32 sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    rant_array RantArray = {};
    fread(&RantArray, sizeof(rant), sz / sizeof(rant), fp);

    for(int RantIndex = 0;
        RantIndex < RantArray.length;
        ++RantIndex)
    {
        rant *Rant = (RantArray.rants + RantIndex);

        printf("\n- %s (%d) -\nScore: %d\n%s\n\nComments: \n", Rant->user.username.ptr, Rant->user.score, Rant->score, Rant->body.ptr);

        for(int CommentIndex = 0;
            CommentIndex < Rant->comments.length;
            ++CommentIndex)
        {
            rant *Comment = Rant->comments.rants + CommentIndex;

            printf("\n- %s (%d) -\nScore: %d\n%s\n", Comment->user.username.ptr, Comment->user.score, Comment->score, Comment->body.ptr);
        }

        printf("\n---\n");
    }

    return(0);
}

COMMAND_FUNC(refresh)
{
    CURL *curl = curl_easy_init();
    sqlite3 *db;
    char *dbErrMsg = 0;
    s32 rc;

    rc = sqlite3_open("rants.db", &db);
    if(rc)
    {
        printf("Could not open db file: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
    }

    char *sort  = "recent";

    if(argc > 0)
    {
        int argidx;
        for(argidx = 0;
            argidx < argc;
            ++argidx)
        {
            if((strcmp(argv[argidx], "-a") == 0) ||
               (strcmp(argv[argidx], "--algo") == 0))
            {
                sort = "algo";
            }

            if((strcmp(argv[argidx], "-r") == 0) ||
               (strcmp(argv[argidx], "--recent") == 0))
            {
                sort = "recent";
            }

            if((strcmp(argv[argidx], "-t") == 0) ||
               (strcmp(argv[argidx], "--top") == 0))
            {
                sort = "top";
            }
        }
    }

    rant_array RantArray = GetRants(curl, sort, 20, 0);

    for(s32 rantIndex = 0;
        rantIndex < RantArray.length;
        ++rantIndex)
    {
        rant *rant = RantArray.rants + rantIndex;

        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, "INSERT INTO rants(id, userid, body, upvotes, downvotes, score, createdtime, tags, comments) " \
                                    "VALUES (:id, :userid, :body, :upvotes, :downvotes, :score, :createdtime, :tags, :comments)", -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, rant->id);
        sqlite3_bind_int(stmt, 2, rant->user.id);
        sqlite3_bind_text(stmt, 3, rant->body.ptr, rant->body.length, 0);
        sqlite3_bind_int(stmt, 4, rant->upvotes);
        sqlite3_bind_int(stmt, 5, rant->downvotes);
        sqlite3_bind_int(stmt, 6, rant->score);
        sqlite3_bind_int(stmt, 7, rant->createdTime);
        sqlite3_bind_blob(stmt, 8, rant->tags, sizeof(tag)*rant->tagCount, 0);
        sqlite3_bind_blob(stmt, 9, rant->comments.rants, sizeof(u32)*rant->comments.length, 0);
    }

    curl_easy_cleanup(curl);
    return(0);
}

COMMAND_FUNC(help)
{
    printf("Actions: \n" \
           "  help          Shows this message\n" \
           "  show          Prints last unread rant\n" \
           "  refresh       Refreshes the cache of rants\n");
    return(0);
}

const struct command Commands[] =
{
    COMMAND(show),
    COMMAND(refresh),
    COMMAND(help),
};

int main(int argc, char **argv)
{
    if(argc >= 2)
    {
        int argidx;
        for(argidx = 0;
            argidx < ArrayCount(Commands);
            ++argidx)
            {
                if(strcmp(Commands[argidx].Name, argv[1]) == 0)
                {
                    return Commands[argidx].cmd(argc - 2, argv + 2);
                }
            }
    }
    else
    {
        int argidx;
        for(argidx = 0;
            argidx < ArrayCount(Commands);
            ++argidx)
            {
                if(strcmp(Commands[argidx].Name, "show") == 0)
                {
                    return Commands[argidx].cmd(0, 0);
                }
            }
    }

    return 0;
}
