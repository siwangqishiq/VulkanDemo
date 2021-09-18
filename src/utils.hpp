#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <vector>


static const std::vector<const char *> convertVectorStringToC(const std::vector<std::string> &list){
    std::vector<const char *> result;
    for(auto &str : list){
        result.push_back((const char *)str.c_str());
    }
    return result;
}

#endif




