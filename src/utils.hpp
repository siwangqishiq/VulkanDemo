#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <vector>
#include <fstream>
#include <vector>

static const std::vector<const char *> convertVectorStringToC(const std::vector<std::string> &list){
    std::vector<const char *> result;
    for(auto &str : list){
        result.push_back((const char *)str.c_str());
    }
    return result;
}

//读取文件为原始二进制格式
static std::vector<char> readFile(const std::string &path){

    std::ifstream file(path , std::ios::ate | std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("open file " + path +" error");
    }

    size_t fileSize = file.tellg();
    //std::cout << path << " filesize = " << fileSize << std::endl;
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data() , fileSize);
    file.close();
    
    return buffer;
}

#endif




