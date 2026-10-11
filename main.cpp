#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <map>
#include <set>

using namespace std;

// Simple file-based key-value store using in-memory map with persistence
const string DATABASE_FILE = "kvstore.db";

class SimpleFileKVStore {
private:
    map<string, set<int>> data;
    bool modified;
    
    void load_from_file() {
        ifstream file(DATABASE_FILE, ios::binary);
        if (!file.is_open()) return;
        
        data.clear();
        
        // Read number of keys
        int key_count;
        file.read(reinterpret_cast<char*>(&key_count), sizeof(key_count));
        
        for (int i = 0; i < key_count; ++i) {
            // Read key length and key
            int key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            
            string key(key_len, '\0');
            file.read(&key[0], key_len);
            
            // Read number of values for this key
            int value_count;
            file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
            
            set<int>& values = data[key];
            for (int j = 0; j < value_count; ++j) {
                int value;
                file.read(reinterpret_cast<char*>(&value), sizeof(value));
                values.insert(value);
            }
        }
        
        modified = false;
    }
    
    void save_to_file() {
        if (!modified) return;
        
        ofstream file(DATABASE_FILE, ios::binary);
        if (!file.is_open()) return;
        
        // Write number of keys
        int key_count = data.size();
        file.write(reinterpret_cast<const char*>(&key_count), sizeof(key_count));
        
        for (const auto& pair : data) {
            const string& key = pair.first;
            const set<int>& values = pair.second;
            
            // Write key length and key
            int key_len = key.length();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(key.c_str(), key_len);
            
            // Write number of values and values
            int value_count = values.size();
            file.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
            
            for (int value : values) {
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
        
        modified = false;
    }
    
public:
    SimpleFileKVStore() : modified(false) {
        load_from_file();
    }
    
    ~SimpleFileKVStore() {
        save_to_file();
    }
    
    void insert(const string& key, int value) {
        data[key].insert(value);
        modified = true;
    }
    
    void erase(const string& key, int value) {
        auto it = data.find(key);
        if (it != data.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                data.erase(it);
            }
            modified = true;
        }
    }
    
    vector<int> find(const string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return vector<int>(it->second.begin(), it->second.end());
        }
        return {};
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n;
    cin >> n;
    cin.ignore(); // Ignore newline after n
    
    SimpleFileKVStore store;
    
    for (int i = 0; i < n; ++i) {
        string line;
        getline(cin, line);
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "insert") {
            string key;
            int value;
            iss >> key >> value;
            store.insert(key, value);
        } else if (command == "delete") {
            string key;
            int value;
            iss >> key >> value;
            store.erase(key, value);
        } else if (command == "find") {
            string key;
            iss >> key;
            
            vector<int> values = store.find(key);
            if (values.empty()) {
                cout << "null";
            } else {
                for (size_t j = 0; j < values.size(); ++j) {
                    if (j > 0) cout << " ";
                    cout << values[j];
                }
            }
            cout << "\n";
        }
    }
    
    return 0;
}