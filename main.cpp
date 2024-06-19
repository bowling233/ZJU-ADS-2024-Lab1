#include <set>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <sstream>
#include <memory>
#include "porter2_stemmer/porter2_stemmer.h"

#if defined(ART)
#include "art.hpp/art.hpp"
#elif defined(HASH)
#include <unordered_map>
#endif

#ifdef BENCHMARK
using timer = std::chrono::high_resolution_clock;
std::ofstream benchmark;
#endif

/**
 * @brief 调用第三方库进行 stemming
 *
 * @param str 输入字符串
 * @return std::string 输出字符串
 */
std::string stemming(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), tolower);
    Porter2Stemmer::trim(str);
#ifndef NO_STEMMING
    Porter2Stemmer::stem(str);
#endif
    return str;
}

/**
 * @brief 倒排索引
 *
 * 成员：
 *
 * - 文档名->文档ID映射表（正确性验证用）
 * - token->postings 映射表
 *
 */
class Database
{
    using postings = std::vector<std::pair<int, int>>;
#if defined(ART)
    art::art<std::shared_ptr<postings>> invert_index_;
#elif defined(RBT)
    std::map<std::string, std::shared_ptr<postings>> invert_index_;
#elif defined(HASH)
    std::unordered_map<std::string, std::shared_ptr<postings>> invert_index_;
#endif

#ifdef CHECK
    std::vector<std::string> docIDs_;
    bool checkQueryWord(std::string word, std::shared_ptr<postings> posting);
#else
    int docIDs_ = 0;
#endif

    std::set<std::string> stop_words_;

public:
    void readDoc(std::string file);
    void readStopWord(std::string file);
    void readDocList(std::string filelist);
    void queryWord(std::string word, int topnum, std::ostream &out);
    void queryList(std::string wordfile, int topnum, std::ostream &out);
    void sort();
    void showInfo(std::ostream &out);
    void showTerms(std::ostream &out);
};

#ifdef CHECK
/**
 * @brief 检查查询结果
 * 
 * @param word 检查的单词
 * @param posting 待检查的倒排索引
 * @return true 通过检查
 * @return false 未通过检查
 */
bool Database::checkQueryWord(std::string word, std::shared_ptr<postings> posting)
{
    for (const auto &doc : *posting)
    {
        std::ifstream in(docIDs_[doc.first]);
        if (!in)
        {
            std::cerr << "[checkQueryWord] Cannot read file: " << docIDs_[doc.first] << std::endl;
            return false;
        }
        int count = 0;
        for (std::string str; in >> str;)
        {
            std::string word_ = stemming(str);
            if (word_ == word)
            {
                count++;
            }
        }
        if (count != doc.second)
        {
            std::cerr << "[checkQueryWord] Word Count Mismatch: " << word << " " << docIDs_[doc.first] << " " << doc.second << " " << count << std::endl;
            return false;
        }
    }
    return true;
}
#endif

/**
 * @brief 为指定文件建立倒排索引
 *
 * 逐个读取单词->词干提取->统计词频->插入倒排索引
 *
 * @param file 文档文件
 */
void Database::readDoc(std::string file)
{
    std::ifstream in(file);
    if (!in)
    {
        std::cerr << "[readDoc] Cannot read file: " << file << std::endl;
        return;
    }
#ifdef BENCHMARK
    timer::time_point start_time = timer::now();
#endif

#ifdef CHECK
    int docID = docIDs_.size();
    docIDs_.push_back(file);
#else
    int docID = docIDs_++;
#endif

    std::map<std::string, int> word_count;
    for (std::string str; in >> str;)
    {
        std::string word = stemming(str);
        if (word.empty() || stop_words_.count(word))
            continue;
        word_count[word]++;
    }
    for (auto &word : word_count)
    {
#if defined(ART)
        auto posting = invert_index_.get(word.first.c_str());
#elif defined(RBT) || defined(HASH)
        auto posting = invert_index_[word.first];
#endif
        if (posting)
        {
            posting->push_back({docID, word.second});
        }
        else
        {
            posting = std::make_shared<postings>();
            posting->push_back({docID, word.second});
#if defined(ART)
            invert_index_.set(word.first.c_str(), posting);
#elif defined(RBT) || defined(HASH)
            invert_index_[word.first] = posting;
#endif
        }
    }

#ifdef BENCHMARK
    timer::time_point end_time = timer::now();
    benchmark << "[benchmark] Read Doc " << file << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count()
              << "ms" << std::endl;
#endif
    in.close();
}

/**
 * @brief 从指定文件中读入停用词
 *
 * 逐个读取单词->插入停用词集合
 *
 * @param file 停用词文件
 */
void Database::readStopWord(std::string file)
{
    std::ifstream in(file);
    if (!in)
    {
        std::cerr << "[readStopWord] Cannot read file: " << file << std::endl;
        return;
    }
    for (std::string str; in >> str;)
    {
        stop_words_.insert(str);
    }
    in.close();
}

/**
 * @brief 从指定文件中读入文档列表
 *
 * 逐个读取文件名->调用readDoc
 *
 * @param filelist 文档列表文件
 */
void Database::readDocList(std::string filelist)
{
    std::ifstream in(filelist);
    if (!in)
    {
        std::cerr << "[readDocList] Cannot read file: " << filelist << std::endl;
        return;
    }
#ifdef BENCHMARK
    timer::time_point start_time = timer::now();
#endif
    for (std::string file; in >> file;)
    {
        readDoc(file);
    }
#ifdef BENCHMARK
    timer::time_point end_time = timer::now();
    benchmark << "[benchmark] Read DocList " << filelist << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count()
              << "ms" << std::endl;
#endif
    in.close();
}

/**
 * @brief 展示数据库状态
 *
 * @param out 输出流
 */
void Database::showInfo(std::ostream &out = std::cout)
{
#ifdef CHECK
    out << "[Info] Documents: " << docIDs_.size() << std::endl;
#else
    out << "[Info] Documents: " << docIDs_ << std::endl;
#endif
    //out << "[Info] Words: " << invert_index_.size() << std::endl;
    out << "[Info] Stop Words: " << stop_words_.size() << std::endl;
    out << std::endl;
}

/**
 * @brief 输出数据库 token 库
 *
 * @param out 输出流
 */
void Database::showTerms(std::ostream &out = std::cout)
{
    out << "[Info] Terms: " << std::endl;
    unsigned int count = 0;
    for (auto it = invert_index_.begin(); it != invert_index_.end(); it++)
    {
#if defined(ART)
        out << it.key() << std::endl;
#elif defined(RBT) || defined(HASH)
        out << it->first << std::endl;
#endif
        count++;
    }
    out << "[Info] Terms Count: " << count << std::endl;
}

/**
 * @brief 查找指定单词的倒排索引
 *
 * 遍历指定单词的倒排索引，输出包含该单词的文档ID和词频
 *
 * @param word 要查询的单词
 */
void Database::queryWord(std::string word, int topnum = -1, std::ostream &out = std::cout)
{
    out << "[queryWord] Querying " << word << std::endl;
    word = stemming(word);
    if (stop_words_.count(word))
    {
        out << "[queryWord] Can't Query Stop Word: " << word << std::endl;
        return;
    }
#ifdef BENCHMARK
    timer::time_point start_time = timer::now();
#endif

#if defined(ART)
    auto posting = invert_index_.get(word.c_str());
#elif defined(RBT) || defined(HASH)
    auto posting = invert_index_[word];
#endif

    if (!posting)
    {
        out << "[queryWord] Word Not Found: " << word << std::endl;
#ifdef BENCHMARK
        timer::time_point end_time = timer::now();
        benchmark << "[benchmark] Query Word Not Found " << word << ": "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         end_time - start_time)
                         .count()
                  << "ms" << std::endl;
#endif
        return;
    }

#ifdef BENCHMARK
    timer::time_point query_time = timer::now();
    benchmark << "[benchmark] Query Word " << word << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     query_time - start_time)
                     .count()
              << "ms" << std::endl;
#endif
    if (!out)
    {
        std::cerr << "[queryWord] Cannot write file." << std::endl;
        return;
    }

#ifndef NO_OUTPUT
    for (const auto &doc : *posting)
    {
        if(topnum-- == 0) break;
        out << doc.first << " " << doc.second << std::endl;
    }
#ifdef BENCHMARK
    timer::time_point end_time = timer::now();
    benchmark << "[benchmark] Print Word " << word << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - query_time)
                     .count()
              << "ms" << std::endl;
#endif
#endif

#ifdef CHECK
    if (!checkQueryWord(word, posting))
    {
        out << "[queryWord] Check Failed: " << word << std::endl;
    }
    else
    {
        out << "[queryWord] Check Passed: " << word << std::endl;
    }
#ifdef BENCHMARK
    timer::time_point check_time = timer::now();
    benchmark << "[benchmark] Check Word " << word << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
#ifndef NO_OUTPUT
                     check_time - end_time)
#else
                     check_time - query_time)
#endif
                     .count()
              << "ms" << std::endl;
#endif
#endif
}

void Database::queryList(std::string wordfile, int topnum=-1, std::ostream &out = std::cout)
{
    std::ifstream in(wordfile);
    if (!in)
    {
        std::cerr << "[queryList] Cannot read file: " << wordfile << std::endl;
        return;
    }
#ifdef BENCHMARK
    timer::time_point start_time = timer::now();
#endif
    std::string word;
    while (in >> word)
    {
        queryWord(word, topnum, out);
    }
#ifdef BENCHMARK
    timer::time_point end_time = timer::now();
    benchmark << "[benchmark] Query List " << wordfile << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count()
              << "ms" << std::endl;
#endif
}

void Database::sort()
{
#ifdef BENCHMARK
    timer::time_point start_time = timer::now();
#endif
    for (auto it = invert_index_.begin(); it != invert_index_.end(); it++)
    {
#if defined(ART)
        auto posting = *it;
#elif defined(RBT) || defined(HASH)
        auto posting = it->second;
#endif
        std::sort(posting->begin(), posting->end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
            return a.second > b.second;
        });
    }
#ifdef BENCHMARK
    timer::time_point end_time = timer::now();
    benchmark << "[benchmark] Sort: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count()
              << "ms" << std::endl;
#endif
}

int main(int argc, char **argv)
{
    Database db;

#ifdef BENCHMARK
#if defined(ART)
    benchmark.open(std::string("result/art_") + argv[1] + std::string(".benchmark"));
#elif defined(RBT)
    benchmark.open(std::string("result/rbt_") + argv[1] + std::string(".benchmark"));
#elif defined(HASH)
    benchmark.open(std::string("result/hash_") + argv[1] + std::string(".benchmark"));
#endif
    if (!benchmark)
    {
        std::cerr << "[benchmark] Cannot write file." << std::endl;
        return 1;
    }
    timer::time_point total_start_time = timer::now();
#endif

    std::string instruction;
    std::getline(std::cin, instruction);
    while (instruction != "END")
    {
        std::stringstream ss(instruction);
        std::vector<std::string> cmd;
        for (std::string str; ss >> str; cmd.push_back(str))
            ;
        if (cmd.empty())
        {
            std::getline(std::cin, instruction);
            continue;
        }

        if (cmd[0] == "Read")
        {
            if (cmd[1] == "DocList")
            {
                db.readDocList(cmd[2]);
            }
            else if (cmd[1] == "StopWord")
            {
                db.readStopWord(cmd[2]);
            }
            else if (cmd[1] == "Doc")
            {
                db.readDoc(cmd[2]);
            }
        }
        else if (cmd[0] == "Query")
        {
            if (cmd[1] == "Word")
            {
                if(cmd.size() == 3)
                    db.queryWord(cmd[2]);
                else
                    db.queryWord(cmd[2], std::stoi(cmd[3]));
            }
            else if (cmd[1] == "List")
            {
                if(cmd.size() == 3)
                    db.queryList(cmd[2]);
                else
                    db.queryList(cmd[2], std::stoi(cmd[3]));
            }
        }
        else if (cmd[0] == "Show")
        {
            if (cmd[1] == "Info")
            {
                db.showInfo();
            }
            else if (cmd[1] == "Term")
            {
                db.showTerms();
            }
        }
        else if (cmd[0] == "Sort")
        {
            db.sort();
        }
        std::getline(std::cin, instruction);
    }

#ifdef BENCHMARK
    timer::time_point total_end_time = timer::now();
    benchmark << "[benchmark] Total: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     total_end_time - total_start_time)
                     .count()
              << "ms" << std::endl;
#endif

    return 0;
}
