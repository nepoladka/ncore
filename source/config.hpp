#pragma once
#include "base64.hpp"

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

#ifndef NCORE_CONFIG_VALID_KEY_CHARS
#define NCORE_CONFIG_VALID_KEY_CHARS "abcdefghijklmnopqrstuvwxyz_-0123456789"
#endif

#ifndef NCORE_CONFIG_WHITESPACES
#define NCORE_CONFIG_WHITESPACES " \t\r\n\v\b\f"
#endif

#ifndef NCORE_CONFIG_SEPARATOR
#define NCORE_CONFIG_SEPARATOR ": "
#endif

namespace ncore {
    using string_t = std::string;

    static constexpr const char const __configSeparator[] = { NCORE_CONFIG_SEPARATOR };

    template<typename _key_t = string_t, typename _value_t = string_t> class config {
    private:
        std::map<_key_t, _value_t> _map;
        string_t _path;
        bool _save;

    public:
        __forceinline config(const string_t& path, bool load = true, bool autoSave = false) {
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

        __forceinline const string_t& path() {
            return _path;
        }

        __forceinline void load() {
            _map.clear();

            std::ifstream infile(_path, std::ios::binary);

            if (infile.is_open()) {
                while (!infile.eof()) {
                    std::pair<_key_t, _value_t> pair;
                    infile.read(reinterpret_cast<char*>(&pair), sizeof(pair));
                    _map.insert(pair);
                }
                infile.close();
            }
        }

        __forceinline void save(bool clear = false) {
            std::ofstream outfile(_path, std::ios::binary);

            if (outfile.is_open()) {
                auto data = this->data();
                outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
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
        std::unordered_map<string_t, string_t> _map;
        std::vector<string_t> _orders;
        string_t _path;
        bool _save;
        bool _loaded;


        __forceinline string_t get_data(bool sortAlphabetically = false) {
            std::ostringstream buffer;

            if (sortAlphabetically) {
                for (auto it : _map) {
                    buffer << it.first << __configSeparator << it.second << std::endl;
                }
            }
            else {
                for (auto key : _orders) {
                    buffer << key << __configSeparator << _map[key] << std::endl;
                }
            }

            return buffer.str();
        }

    public:
        __forceinline readable_config(const string_t& path, bool autoSave = false) noexcept : _path(path) {
            _save = autoSave;
            _loaded = load(_path);
        }

        __forceinline readable_config(const std::ifstream& file) noexcept : _path() {
            _loaded = load(*((std::istream*)&file));
        }

        __forceinline readable_config(const std::istringstream& data) noexcept : _path() {
            _save = false;

            _loaded = load(*((std::istream*)&data));
        }

        __forceinline readable_config() noexcept : _path()  {
            _save = _loaded = false;
        }

        __forceinline ~readable_config() noexcept {
            if (_save)
                save();
        }

        __forceinline string_t path() const noexcept {
            return _path;
        }

        __forceinline bool loaded() const noexcept {
            return _loaded;
        }

        __forceinline bool load(std::istream& data) noexcept {
            auto line = string_t();

            for (auto i = index_t(); std::getline(data, line); i++) {
                string_t key, value, lower_line = line;
                std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
                //std::istringstream stream(line);

                auto key_begin = lower_line.find_first_of(NCORE_CONFIG_VALID_KEY_CHARS);
                auto key_end = lower_line.find_first_not_of(NCORE_CONFIG_VALID_KEY_CHARS, key_begin);
                if (key_begin == std::string::npos || key_end == std::string::npos) continue;
                key = lower_line.substr(key_begin, key_end);

                auto value_begin = lower_line.find_first_not_of(NCORE_CONFIG_WHITESPACES, key_end + sizeof(__configSeparator) - 1);
                if (value_begin == std::string::npos || value_begin == line.length() + 1) continue;
                value = line.substr(value_begin, std::string::npos);

                set(key, value);
            }

            return true;
        }

        __forceinline bool load(const string_t& path) noexcept {
            std::ifstream file;
            file.open(path, std::ifstream::in, std::ifstream::_Openprot);

            auto result = load(file);

            if (file.is_open()) {
                file.close();
            }

            return result;
        }

        __forceinline bool load() noexcept {
            return load(_path);
        }

        __forceinline void clear() noexcept {
            _map.clear();
            _orders.clear();
        }

        __forceinline bool empty() const noexcept {
            return _map.empty();
        }

        __forceinline string_t data(bool sort = false) noexcept {
            return ((readable_config*)this)->get_data(sort);
        }

        __forceinline string_t data(bool sort = false) const noexcept {
            return ((readable_config*)this)->get_data(sort);
        }

        __forceinline bool save(bool sort = false) noexcept {
            return _path.empty() ? false : save(_path, sort);
        }

        __forceinline bool save(std::ofstream& file, bool sort = false) noexcept {
            if (!file.is_open()) return false;

            file << data(sort) << std::endl;

            return true;
        }

        __forceinline bool save(const string_t& path, bool sort = false) noexcept {
            std::ofstream file;
            file.open(path, std::ifstream::trunc);

            auto result = save(file, sort);

            if (file.is_open()) {
                file.close();
            }

            return result;
        }

        __forceinline string_t get(const string_t& key, const string_t& default_value = string_t()) const noexcept {
            for (auto& part : _map) {
                if (part.first == key) return part.second;
            }
            return default_value;
        }

        __forceinline bool fget(const string_t& key, const char* const format, ...) const noexcept {
            auto value = get(key);
            if (value.empty()) return false;

            va_list arguments_list;
            __crt_va_start(arguments_list, format);
            auto result = _vsscanf_s_l(value.c_str(), format, nullptr, arguments_list);
            __crt_va_end(arguments_list);

            return result;
        }

        __forceinline readable_config iget(const string_t& key) const noexcept {
            auto encoded = get(key);
            return encoded.empty() ? readable_config() : readable_config(std::istringstream(base64::decode(encoded)));
        }

        __forceinline bool set(const string_t& key, const string_t& value) noexcept {
            auto it = _map.insert(std::pair<string_t, string_t>(key, value));
            if (it.second) {
                _orders.push_back(key);
            }
            else {
                (*it.first).second = value;
            }
            return it.second;
        }

        template<size_t _bufferSize = 512> __forceinline bool fset(const string_t& key, const char* const format, ...) noexcept {
            const auto buffer_size = _bufferSize ? _bufferSize : 512;
            auto buffer = new char[buffer_size];

            va_list arguments_list;
            __crt_va_start(arguments_list, format);
            vsnprintf(buffer, buffer_size, format, arguments_list);
            __crt_va_end(arguments_list);

            auto result = set(key, buffer);
            delete[] buffer;

            return result;
        }

        __forceinline bool iset(const string_t& key, const readable_config& inner) noexcept {
            if (inner.empty()) return false;
            auto value = inner.data();
            return set(key, base64::encode(value.data(), value.size()));
        }
    };
}
