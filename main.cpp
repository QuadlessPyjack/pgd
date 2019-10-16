#include <sys/types.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <fcntl.h>

#include <iostream.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


#define COMMIT_LENGTH 5

DIR* getCurrentDirTree(const std::string &path)
{
    DIR *dirp;
    dirp = opendir(path.c_str());

    return dirp;
}

struct LastModifiedEntry
{
    std::string name;
    long timestamp;
};

std::vector<LastModifiedEntry> cwdList;
std::vector<LastModifiedEntry> commitList;

LastModifiedEntry getLastModified(const std::string &filePath)
{
    #define YEAR(t)   (((t & 0xFE00) >> 9) + 1980)
    #define MONTH(t)  ((t & 0x01E0) >> 5)
    #define DAY(t)    (t & 0x001F)
    #define HOUR(t)   ((t & 0xF800) >> 11)
    #define MINUTE(t) ((t & 0x07E0) >> 5)
    #define SECOND(t) ((t & 0x001F) << 1)

    int handle;
    unsigned int date, time;
    LastModifiedEntry entry;

    if (_dos_open(filePath.c_str(), O_RDONLY, &handle) != 0)
    {
        entry.name = "";
        entry.timestamp = -1;
        return entry;
    }

    _dos_getftime(handle, &date, &time);
    //std::cout << filePath.c_str() << " " << DAY(date) << "/" << MONTH(date) << "/" << YEAR(date) << " " << HOUR(time) << ":" << MINUTE(time) << "." << SECOND(time) << '\n';

    entry.name = filePath;
    entry.timestamp = date + time;

    return entry;
}

char* cleanString(char* const input, unsigned int length)
{
    if (input == NULL)
    {
        return NULL;
    }

    char* str = new char[strlen(input)];
    strcpy(str, input);

    for (unsigned int idx = 0; idx < length; ++idx)
    {
        if (input[idx] == '\0' || input[idx] == '\n' || input[idx] == '\r' || input[idx] == ' ')
        {
            if (idx == 0)
            {
                strcpy(str, &input[1]);
                continue;
            }

            if ((idx + 1) >= length) return str;
            memmove(&str[idx], &input[idx + 1], strlen(input) - 1);
        }
    }

    return input;
}

std::string getCurrentCommit()
{
    std::string currentCommit = "";
    std::ifstream ifconfig("_config\\config.inf");
    if (!ifconfig.good())
    {
        return "";
    }

    ifconfig.seekg(10);
    // workaround because >> operator currently not supported
    char *rawCurrentCommit = (char*)malloc(COMMIT_LENGTH + 1);
    ifconfig.read(rawCurrentCommit, COMMIT_LENGTH);

    rawCurrentCommit = cleanString(rawCurrentCommit, COMMIT_LENGTH);
    std::cout << "raw= " << rawCurrentCommit << '\n';
    currentCommit.assign(rawCurrentCommit);

    free(rawCurrentCommit);
    ifconfig.close();
    std::cout << " |" << currentCommit.c_str() << "| ";
    return currentCommit;
}

std::vector<LastModifiedEntry> getLastModifiedEntriesInPath(std::string path)
{
    std::vector<LastModifiedEntry> entryList;
    struct dirent* direntry = NULL;
    DIR* pdir = getCurrentDirTree(path);
    if (pdir == NULL)
    {
        return entryList;
    }

    while ((direntry = readdir(pdir)) != NULL)
    {
        if (!strcmp(direntry->d_name,".")   ||
            !strcmp(direntry->d_name, "..") ||
            !strcmp(direntry->d_name,"_config")) continue;

        LastModifiedEntry entry = getLastModified(direntry->d_name);
        entryList.push_back(entry);
    }
    closedir(pdir);

    delete direntry;
    delete pdir;
    direntry = 0;
    pdir = 0;

    std::cout << entryList.size() << "\n";

    return entryList;
}

int main(int argc, char* argv[])
{
    std::string currentPath;
    std::string currentCommit;

    currentPath = _getcwd(NULL,0);

    if (!strcmp(argv[1], "/status"))
    {
        //cwdList = getLastModifiedEntriesInPath(currentPath);
        currentCommit = getCurrentCommit();
        std::string currentCommitPath = currentPath + std::string("\\_config\\") + currentCommit;
        //TODO: trim special chars '\n' '\0' from I\O strings
        std::cout << currentCommitPath.c_str();
        commitList = getLastModifiedEntriesInPath(currentCommitPath);
    }

    if (!strcmp(argv[1], "/init"))
    {
        std::cout << "Initializing repository in " << currentPath.c_str() << '\n';
        system("echo off");
        system("mkdir _config");
        //int errno = _mkdir("_config");
        if (errno == -1)
        {
            // version control config dir already exists
        }

        system("mkdir _config\\base");
        //errno = _mkdir("_config\\base");
        if (errno == -1)
        {
            // base already set up
        }

        struct dirent *direntp;
        DIR* dirp = getCurrentDirTree(currentPath);
        int numFiles = 0;
        std::ofstream fmetadata("_config\\base\\meta.inf");
        fmetadata.seekp(13);
        fmetadata << "[listing]\n";
        if (dirp != NULL)
        {
            while ((direntp = readdir(dirp)) != NULL)
            {
                if (!strcmp(direntp->d_name,".")   ||
                    !strcmp(direntp->d_name, "..") ||
                    !strcmp(direntp->d_name,"_config")) continue;

                std::string cmd = std::string("copy ") + std::string(direntp->d_name) + std::string(" _config\\base");
                system(cmd.c_str());
                std::cout << direntp->d_name << '\n';

                numFiles++;
                fmetadata << direntp->d_name << '\n';
            }
            closedir(dirp);
        }

        fmetadata.seekp(0);
        fmetadata << "[files]\n" << numFiles << '\n';
        fmetadata.close();

        currentCommit = "base";

        std::ofstream fconfig("_config\\config.inf");
        fconfig << "[current]\n" << currentCommit.c_str();
        fconfig.close();

        system("echo on");
        delete direntp;
        delete dirp;
        direntp = 0;
        dirp = 0;
    }
    return 0;
}