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


DIR* getCurrentDirTree(const std::string &path)
{
    DIR *dirp;
    dirp = opendir(path.c_str());

    return dirp;
}

int getLastModified(const std::string &filePath)
{
    #define YEAR(t)   (((t & 0xFE00) >> 9) + 1980)
    #define MONTH(t)  ((t & 0x01E0) >> 5)
    #define DAY(t)    (t & 0x001F)
    #define HOUR(t)   ((t & 0xF800) >> 11)
    #define MINUTE(t) ((t & 0x07E0) >> 5)
    #define SECOND(t) ((t & 0x001F) << 1)

    int handle;
    unsigned int date, time;

    if (_dos_open(filePath.c_str(), O_RDONLY, &handle) != 0)
    {
        return -1;
    }

    _dos_getftime(handle, &date, &time);
    std::cout << filePath.c_str() << " " << DAY(date) << "/" << MONTH(date) << "/" << YEAR(date) << " " << HOUR(time) << ":" << MINUTE(time) << "." << SECOND(time) << '\n';

    return date;
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
    ifconfig >> currentCommit;
    ifconfig.close();

    return currentCommit;
}

//struct
//{
    //std::string name;
    //long timestamp;
//} entry;

int main(int argc, char* argv[])
{
    std::string currentPath;
    std::string currentCommit;

    currentPath = _getcwd(NULL,0);

    if (!strcmp(argv[1], "/status"))
    {
        struct dirent* direntry = NULL;
        DIR* pdir = getCurrentDirTree(currentPath);
        if (pdir == NULL)
        {
            return 0;
        }

        while ((direntry = readdir(pdir)) != NULL)
        {
            if (!strcmp(direntry->d_name,".")   ||
                    !strcmp(direntry->d_name, "..") ||
                    !strcmp(direntry->d_name,"_config")) continue;

            getLastModified(direntry->d_name);
        }
        closedir(pdir);

        delete direntry;
        delete pdir;
        direntry = 0;
        pdir = 0;
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