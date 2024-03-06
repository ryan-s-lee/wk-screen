#include "radixtree.h"
#include <iostream>

class long_wrapper {

public:
    int *bruh;
    long_wrapper(int hello){
        bruh = new int(hello);
    }
    ~long_wrapper() {
        delete bruh;
    }
};

int main(int argc, char const *argv[])
{
    llm_sys::radix_tree<std::string> bruh{};
    bruh.put("help", new std::string("me"));
    std::string result = bruh.get("help");
    std::cout << "String @ \"help\" stored should be \"me\": " << result << '\n';
    bruh.put("hell", new std::string("hole"));
    result = bruh.get("help");
    std::cout << "String @ \"help\" stored should be \"me\": " << result << '\n';
    result = bruh.get("hell");
    std::cout << "String @ \"hell\" stored should be \"hole\": " << result << '\n';
    result = bruh.get("hel");
    std::cout << "String @ \"hel\" stored should be undefined: " << result << '\n';
    bruh.put("helping hand", new std::string("meme"));
    result = bruh.get("helping hand");
    std::cout << "String @ \"helping hand\" stored should be \"meme\": " << result << '\n';
    bruh.put("helping out", new std::string("meme2"));
    result = bruh.get("helping out");
    std::cout << "String @ \"helping out\" stored should be \"meme2\": " << result << '\n';
    std::pair<std::string, std::string> result2 = bruh.get_best_match("helping");
    std::cout << "String @ \"helping\" stored should be the same as help, \"me\": " << result2.first + ", " + result2.second << '\n';
    result2 = bruh.get_best_match("helping ");
    std::cout << "String @ \"helping \" stored should be the same as help, \"me\": " << result2.first + ", " + result2.second << '\n';
    return 0;
}
