#ifndef MapReduce_word_counter_h
#define MapReduce_word_counter_h

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <unordered_map>

class user_mapper {
private:
    std::string get_file_text (const std::string& file_path) {
        std::ifstream stream(file_path, std::ifstream::in);
        stream.open(file_path);
        std::stringstream str_stream;
        str_stream << stream.rdbuf();
        return str_stream.str();
    }
    
    std::vector<std::string> split_by(char splitter, const std::string& text) {
        std::vector<std::string> splitted_text;
        auto last_pos = text.begin();
        for (auto letter = text.begin(); letter != text.end(); ++letter) {
            if (*letter == splitter) {
                if (std::string(last_pos, letter) != "") {
                    splitted_text.push_back(std::string(last_pos, letter));
                }
                last_pos = letter + 1;
            }
        }
        splitted_text.push_back(std::string(last_pos, text.end()));
        return splitted_text;
    }
    
    void map_data(std::vector<std::pair<std::string, std::string>>& mapped_data, const std::string& text) {
        std::vector<std::string> words = split_by(' ', text);
        for (auto word = words.begin(); word != words.end(); ++word) {
            mapped_data.emplace_back(std::make_pair(*word, "1"));
        }
    }
    
public:
    std::vector<std::pair<std::string, std::string>> operator () (const std::pair<std::string, std::string>& key_val) {
        std::vector<std::pair<std::string, std::string>> mapped_data;
        if (key_val.first == "" && key_val.second == "") {
            throw std::invalid_argument("No text exception\n");
        }
        if (key_val.second != "") { // If text is from string variable.
            map_data(mapped_data, key_val.second);
        } else { // If text is from file.
            std::string text = get_file_text(key_val.first);
            map_data(mapped_data, text);
        }
        return mapped_data;
    }
};

class user_reducer {
public:
    std::vector<std::pair<std::string, std::string>> operator () (std::vector<std::pair<std::string, std::string>>& mapped_data) {
        std::vector<std::pair<std::string, std::string>> answer;
        size_t word_occurences = 0;
        std::string last_key = mapped_data[0].first;
        std::sort(mapped_data.begin(), mapped_data.end());
        for (auto iter = mapped_data.begin(); iter != mapped_data.end(); ++iter) {
            if (iter->first != last_key) {
                answer.push_back(std::make_pair(last_key, std::to_string(word_occurences)));
                last_key = iter->first;
                word_occurences = 1;
            } else {
                ++word_occurences;
            }
            if (iter == mapped_data.end() - 1) {
                answer.push_back(std::make_pair(last_key, std::to_string(word_occurences)));
            }
        }
        
        
        
        
        
        
//        std::string last_key = mapped_data.begin()->first;
//        for (auto iter = mapped_data.begin(); iter != mapped_data.end(); ++iter) {
//            if (last_key == iter->first) {
//                word_occurences += std::stoi(iter->second);
//            } else {
//                answer.emplace_back(std::make_pair(last_key, std::to_string(word_occurences)));
//                last_key = iter->first;
//                word_occurences = 1;
//            }
//        }
//        answer.emplace_back(std::make_pair(last_key, std::to_string(word_occurences)));
        return answer;
    }
};

#endif
