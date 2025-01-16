
#include "cmdpipeio.h"
#include <iostream>
#include <sstream>
#include <string>

std::string getCommandOutput(std::string cmd)
{
    FILE *fp;
    std::string str = "";
    char path[1035];
    fp = popen(cmd.c_str(), "r");
    if (fp == NULL)
    {
        std::cout << "Failed to run command" << std::endl;
        ;
        exit(1);
    }
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        str = str + path;
    }
    pclose(fp);
    return str;
}
