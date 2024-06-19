// stop word counter
#include <set>
#include <map>
#include <list>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <math.h>
#include <fstream>
#include <chrono>
#include <sstream>
#include "porter2_stemmer/porter2_stemmer.h"

std::string stemming(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), tolower);
    Porter2Stemmer::trim(str);
    Porter2Stemmer::stem(str);
    return str;
}

int main()
{
    unsigned long long int count = 0;
    std::set<std::string> stop_words;
    std::ifstream stop_word_file("resources/stopword/innodb");
    //for(std::string word; stop_word_file >> word;)
    //{
    //    stop_words.insert(stemming(word));
    //}
    stop_word_file.close();
    stop_word_file.open("resources/stopword/myisam");
    for(std::string word; stop_word_file >> word;)
    {
        stop_words.insert(stemming(word));
    }
    stop_word_file.close();
    std::cout << "stop word count: " << stop_words.size() << std::endl;

    std::ifstream file("resources/shakespeare.all");
    for(std::string word; file >> word;)
    {
        if(stop_words.find(stemming(word)) != stop_words.end())
        {
            count++;
        }
    }
    std::cout << "stop word count: " << count << std::endl;
    return 0;
}
