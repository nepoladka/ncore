#pragma once
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

#ifndef CONFIG_VALID_KEY_CHARS
#define CONFIG_VALID_KEY_CHARS "abcdefghijklmnopqrstuvwxyz_-0123456789"
#endif

#ifndef CONFIG_WHITESPACES
#define CONFIG_WHITESPACES " \t\r\n\v\b\f"
#endif

#ifndef CONFIG_SEPARATOR
static constexpr const char __configSeparator[] = { ": " };
#define CONFIG_SEPARATOR __configSeparator
#endif

namespace ncore {
    using namespace std;

    template<typename _key_t = string, typename _value_t = string> class config {
    private:
        map<_key_t, _value_t> _map;
        string _path;
        bool _save;

    public:
        __forceinline config(const string& path, bool load = true, bool autoSave = false) {
            _path = path;
            _save = autoSave;

            if (load)
                this->load();
        }

        __forceinline config(byte_t* data, size_t dataSize) {
            auto current = data;
            auto end = current + dataSize;

            while (current < end) {
                auto pair = *reinterpret_cast<std::pair<_key_t, _value_t>*>(current);
                _map.insert(pair);
                current += sizeof(pair);
            }
        }

        __forceinline ~config() {
            if (_save)
                this->save();

            _map.clear();
            _path.clear();
        }

        __forceinline const string& path() {
            return _path;
        }

        __forceinline void load() {
            _map.clear();

            ifstream infile(_path, ios::binary);

            if (infile.is_open()) {
                while (!infile.eof()) {
                    pair<_key_t, _value_t> pair;
                    infile.read(reinterpret_cast<char*>(&pair), sizeof(pair));
                    _map.insert(pair);
                }
                infile.close();
            }
        }

        __forceinline void save(bool clear = false) {
            ofstream outfile(_path, ios::binary);

            if (outfile.is_open()) {
                auto data = this->data();
                outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
                /*for (auto const& pair : _map) {
                    outfile.write(reinterpret_cast<const char*>(&pair), sizeof(pair));
                }*/
                outfile.close();
            }

            if (clear) {
                _map.clear();
            }
        }

        __forceinline std::vector<byte_t> data() {
            auto result = std::vector<byte_t>();
            for (auto& pair : _map) {
                result.insert(result.end(), reinterpret_cast<byte_t*>(&pair), reinterpret_cast<byte_t*>(&pair) + sizeof(pair));
            }
            return result;
        }

        __forceinline _value_t get(const _key_t& key) {
            for (auto& pair : _map) {
                if (pair.first == key) return pair.second;
            }
            return _value_t();
        }

        __forceinline void set(const _key_t& key, const _value_t& value) {
            for (auto& pair : _map) {
                if (pair.first == key) {
                    pair.second = value;
                    return;
                }
            }

            _map.insert({ key, value });
        }
    };

    class readable_config
    {
    private:
        unordered_map<string, string> _map;
        vector<string> _orders;
        string _path;
        bool _save;
        bool _loaded;

    public:
        __forceinline readable_config(const string& path, bool autoSave = false) : _path(path) {
            _save = autoSave;
            _loaded = load(_path);
        }

        __forceinline readable_config(const ifstream& file) : _path() {
            _loaded = load(*((istream*)&file));
        }

        __forceinline readable_config(const istringstream& data) : _path() {
            _save = false;

            _loaded = load(*((istream*)&data));
        }

        __forceinline readable_config() : _path() {
            _save = _loaded = false;
        }

        __forceinline ~readable_config() {
            if (_save)
                save();
        }

        __forceinline string path() {
            return _path;
        }

        __forceinline bool loaded() {
            return _loaded;
        }

        __forceinline bool load(istream& data) {
            string line;

            for (int i = 0; std::getline(data, line); i++)
            {
                string key, value;
                string lowerLine = line;
                transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
                istringstream iss(line);

                int keyStart = lowerLine.find_first_of(CONFIG_VALID_KEY_CHARS);
                int keyEnd = lowerLine.find_first_not_of(CONFIG_VALID_KEY_CHARS, keyStart);
                if (keyStart == string::npos || keyEnd == string::npos) continue;
                key = lowerLine.substr(keyStart, keyEnd);

                int valueStart = lowerLine.find_first_not_of(CONFIG_WHITESPACES, keyEnd + sizeof(CONFIG_SEPARATOR) - 1);
                if (valueStart == string::npos || valueStart == line.length() + 1) continue;
                value = line.substr(valueStart, string::npos);

                set(key, value);
            }

            return true;
        }

        __forceinline bool load(const string& path) {
            ifstream file;
            file.open(path, ifstream::in, ifstream::_Openprot);
            bool ret = load(file);

            if (file.is_open()) 
                file.close();

            return ret;
        }

        __forceinline bool load() {
            return load(_path);
        }

        __forceinline std::string data(bool sortAlphabetically = false) {
            ostringstream buffer;

            if (sortAlphabetically) {
                for (auto it : _map) {
                    buffer << it.first << CONFIG_SEPARATOR << it.second << endl;
                }
            }
            else {
                for (auto key : _orders) {
                    buffer << key << CONFIG_SEPARATOR << _map[key] << endl;
                }
            }

            return buffer.str();
        }

        __forceinline bool save(bool sortAlphabetically = false) {
            if (_path.empty()) return false;
            return save(_path, sortAlphabetically);
        }

        __forceinline bool save(ofstream& file, bool sortAlphabetically = false) {
            if (!file.is_open()) return false;

            file << data(sortAlphabetically) << endl;

            return true;
        }

        __forceinline bool save(const string& path, bool sortAlphabetically = false) {
            ofstream file;
            file.open(path, ifstream::trunc);
            auto ret = save(file, sortAlphabetically);

            if (file.is_open()) 
                file.close();

            return ret;
        }

        __forceinline string get(const string& key) {
            for (auto& part : _map) {
                if (part.first == key) return part.second;
            }
            return string();
        }

        __forceinline bool fget(const string& key, const char* const format, ...) {
            auto value = get(key);
            if (value.empty()) return false;

            va_list argList;
            __crt_va_start(argList, format);
            auto result = _vsscanf_s_l(value.c_str(), format, nullptr, argList);
            __crt_va_end(argList);

            return result;
        }

        __forceinline bool set(const string& key, const string& value) {
            auto it = _map.insert(pair<string, string>(key, value));
            if (it.second) {
                _orders.push_back(key);
            }
            else {
                (*it.first).second = value;
            }
            return it.second;
        }

        __forceinline bool fset(const string& key, const char* const format, ...) {
            char buffer[512] = { 0 };

            va_list argList;
            __crt_va_start(argList, format);
            vsnprintf(buffer, sizeof(buffer), format, argList);
            __crt_va_end(argList);

            return set(key, buffer);
        }
    };
}
