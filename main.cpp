#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <set>
#include <map>

using namespace std;

// Constants for B+ Tree
const int MAX_KEY_LENGTH = 64;
const int ORDER = 32;  // Maximum number of children per internal node
const int LEAF_CAPACITY = 64;  // Maximum number of key-value pairs per leaf

const string DATABASE_FILE = "bptree.db";

// Node types
enum NodeType {
    INTERNAL = 0,
    LEAF = 1
};

// B+ Tree Node structure
struct Node {
    NodeType type;
    int key_count;
    int parent;  // Node ID of parent, -1 if root
    int next;    // For leaf nodes: next leaf node ID
    
    // For internal nodes
    int children[ORDER + 1];  // Child node IDs
    char keys[ORDER][MAX_KEY_LENGTH];  // Keys
    
    // For leaf nodes
    char leaf_keys[LEAF_CAPACITY][MAX_KEY_LENGTH];  // Keys
    int leaf_values[LEAF_CAPACITY];  // Values
    
    Node() : type(INTERNAL), key_count(0), parent(-1), next(-1) {
        memset(children, -1, sizeof(children));
        memset(keys, 0, sizeof(keys));
        memset(leaf_keys, 0, sizeof(leaf_keys));
        memset(leaf_values, 0, sizeof(leaf_values));
    }
};

class BPlusTree {
private:
    fstream file;
    int root_id;
    int next_node_id;
    vector<int> free_nodes;
    
    // File operations
    void open_file() {
        file.open(DATABASE_FILE, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            // Create new file
            file.open(DATABASE_FILE, ios::out | ios::binary);
            file.close();
            file.open(DATABASE_FILE, ios::in | ios::out | ios::binary);
            initialize_file();
        } else {
            read_header();
        }
    }
    
    void close_file() {
        if (file.is_open()) {
            write_header();
            file.close();
        }
    }
    
    void initialize_file() {
        root_id = -1;
        next_node_id = 0;
        free_nodes.clear();
        write_header();
    }
    
    void write_header() {
        file.seekp(0);
        file.write(reinterpret_cast<char*>(&root_id), sizeof(root_id));
        file.write(reinterpret_cast<char*>(&next_node_id), sizeof(next_node_id));
        
        int free_count = free_nodes.size();
        file.write(reinterpret_cast<char*>(&free_count), sizeof(free_count));
        for (int node_id : free_nodes) {
            file.write(reinterpret_cast<char*>(&node_id), sizeof(node_id));
        }
    }
    
    void read_header() {
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&root_id), sizeof(root_id));
        file.read(reinterpret_cast<char*>(&next_node_id), sizeof(next_node_id));
        
        int free_count;
        file.read(reinterpret_cast<char*>(&free_count), sizeof(free_count));
        free_nodes.resize(free_count);
        for (int i = 0; i < free_count; ++i) {
            file.read(reinterpret_cast<char*>(&free_nodes[i]), sizeof(free_nodes[i]));
        }
    }
    
    int get_header_size() {
        return sizeof(root_id) + sizeof(next_node_id) + sizeof(int) + free_nodes.size() * sizeof(int);
    }
    
    int allocate_node() {
        int node_id;
        if (!free_nodes.empty()) {
            node_id = free_nodes.back();
            free_nodes.pop_back();
        } else {
            node_id = next_node_id++;
        }
        return node_id;
    }
    
    void free_node(int node_id) {
        free_nodes.push_back(node_id);
    }
    
    void write_node(int node_id, const Node& node) {
        int offset = get_header_size() + node_id * sizeof(Node);
        file.seekp(offset);
        file.write(reinterpret_cast<const char*>(&node), sizeof(Node));
        file.flush();
    }
    
    void read_node(int node_id, Node& node) {
        int offset = get_header_size() + node_id * sizeof(Node);
        file.seekg(offset);
        file.read(reinterpret_cast<char*>(&node), sizeof(Node));
    }
    
    // Key comparison
    int compare_keys(const char* key1, const char* key2) {
        return strcmp(key1, key2);
    }
    
    // B+ Tree operations
    void insert_into_leaf(Node& leaf, const char* key, int value) {
        int pos = 0;
        while (pos < leaf.key_count && compare_keys(leaf.leaf_keys[pos], key) < 0) {
            pos++;
        }
        
        // Check for duplicate
        if (pos < leaf.key_count && compare_keys(leaf.leaf_keys[pos], key) == 0) {
            // Check if value already exists
            for (int i = pos; i < leaf.key_count && compare_keys(leaf.leaf_keys[i], key) == 0; ++i) {
                if (leaf.leaf_values[i] == value) {
                    return; // Duplicate entry, ignore
                }
            }
            // Insert value in sorted order within same key
            while (pos < leaf.key_count && compare_keys(leaf.leaf_keys[pos], key) == 0 && leaf.leaf_values[pos] < value) {
                pos++;
            }
        }
        
        // Shift elements to make space
        for (int i = leaf.key_count; i > pos; --i) {
            strcpy(leaf.leaf_keys[i], leaf.leaf_keys[i - 1]);
            leaf.leaf_values[i] = leaf.leaf_values[i - 1];
        }
        
        // Insert new entry
        strcpy(leaf.leaf_keys[pos], key);
        leaf.leaf_values[pos] = value;
        leaf.key_count++;
    }
    
    void delete_from_leaf(Node& leaf, const char* key, int value) {
        int pos = 0;
        while (pos < leaf.key_count && compare_keys(leaf.leaf_keys[pos], key) < 0) {
            pos++;
        }
        
        // Find the exact entry
        while (pos < leaf.key_count && compare_keys(leaf.leaf_keys[pos], key) == 0) {
            if (leaf.leaf_values[pos] == value) {
                // Found it, delete
                for (int i = pos; i < leaf.key_count - 1; ++i) {
                    strcpy(leaf.leaf_keys[i], leaf.leaf_keys[i + 1]);
                    leaf.leaf_values[i] = leaf.leaf_values[i + 1];
                }
                leaf.key_count--;
                return;
            }
            pos++;
        }
    }
    
    vector<int> find_in_leaf(const Node& leaf, const char* key) {
        vector<int> result;
        for (int i = 0; i < leaf.key_count; ++i) {
            if (compare_keys(leaf.leaf_keys[i], key) == 0) {
                result.push_back(leaf.leaf_values[i]);
            }
        }
        return result;
    }
    
    int find_child(const Node& node, const char* key) {
        if (node.type == LEAF) return -1;
        
        for (int i = 0; i < node.key_count; ++i) {
            if (compare_keys(key, node.keys[i]) < 0) {
                return node.children[i];
            }
        }
        return node.children[node.key_count];
    }
    
public:
    BPlusTree() : root_id(-1), next_node_id(0) {
        open_file();
    }
    
    ~BPlusTree() {
        close_file();
    }
    
    void insert(const string& key, int value) {
        char key_buf[MAX_KEY_LENGTH];
        strncpy(key_buf, key.c_str(), MAX_KEY_LENGTH - 1);
        key_buf[MAX_KEY_LENGTH - 1] = '\0';
        
        if (root_id == -1) {
            // Create root leaf
            root_id = allocate_node();
            Node root;
            root.type = LEAF;
            root.parent = -1;
            insert_into_leaf(root, key_buf, value);
            write_node(root_id, root);
            return;
        }
        
        // Navigate to leaf
        int current_id = root_id;
        Node current;
        read_node(current_id, current);
        
        while (current.type == INTERNAL) {
            int child_id = find_child(current, key_buf);
            read_node(child_id, current);
            current_id = child_id;
        }
        
        // Insert into leaf
        insert_into_leaf(current, key_buf, value);
        write_node(current_id, current);
        
        // Handle leaf overflow (simplified - just create new leaf for now)
        if (current.key_count > LEAF_CAPACITY) {
            // For simplicity, we'll just split the leaf
            Node new_leaf;
            new_leaf.type = LEAF;
            new_leaf.parent = current.parent;
            new_leaf.next = current.next;
            current.next = allocate_node();
            
            // Move half the entries to new leaf
            int split_point = current.key_count / 2;
            for (int i = split_point; i < current.key_count; ++i) {
                strcpy(new_leaf.leaf_keys[i - split_point], current.leaf_keys[i]);
                new_leaf.leaf_values[i - split_point] = current.leaf_values[i];
            }
            new_leaf.key_count = current.key_count - split_point;
            current.key_count = split_point;
            
            write_node(current_id, current);
            write_node(current.next, new_leaf);
            
            // Update parent (simplified)
            if (current.parent == -1) {
                // Create new root
                Node new_root;
                new_root.type = INTERNAL;
                new_root.parent = -1;
                new_root.children[0] = current_id;
                new_root.children[1] = current.next;
                strcpy(new_root.keys[0], new_leaf.leaf_keys[0]);
                new_root.key_count = 1;
                
                int new_root_id = allocate_node();
                root_id = new_root_id;
                current.parent = new_root_id;
                new_leaf.parent = new_root_id;
                
                write_node(new_root_id, new_root);
                write_node(current_id, current);
                write_node(current.next, new_leaf);
            }
        }
    }
    
    void erase(const string& key, int value) {
        if (root_id == -1) return;
        
        char key_buf[MAX_KEY_LENGTH];
        strncpy(key_buf, key.c_str(), MAX_KEY_LENGTH - 1);
        key_buf[MAX_KEY_LENGTH - 1] = '\0';
        
        // Navigate to leaf
        int current_id = root_id;
        Node current;
        read_node(current_id, current);
        
        while (current.type == INTERNAL) {
            int child_id = find_child(current, key_buf);
            read_node(child_id, current);
            current_id = child_id;
        }
        
        // Delete from leaf
        delete_from_leaf(current, key_buf, value);
        write_node(current_id, current);
    }
    
    vector<int> find(const string& key) {
        if (root_id == -1) return {};
        
        char key_buf[MAX_KEY_LENGTH];
        strncpy(key_buf, key.c_str(), MAX_KEY_LENGTH - 1);
        key_buf[MAX_KEY_LENGTH - 1] = '\0';
        
        // Navigate to leaf
        int current_id = root_id;
        Node current;
        read_node(current_id, current);
        
        while (current.type == INTERNAL) {
            int child_id = find_child(current, key_buf);
            read_node(child_id, current);
            current_id = child_id;
        }
        
        // Search in leaf and potentially next leaves
        vector<int> result;
        while (current_id != -1) {
            vector<int> leaf_result = find_in_leaf(current, key_buf);
            result.insert(result.end(), leaf_result.begin(), leaf_result.end());
            
            // Check if we need to continue to next leaf
            bool should_continue = false;
            for (int i = 0; i < current.key_count; ++i) {
                if (compare_keys(current.leaf_keys[i], key_buf) == 0) {
                    should_continue = true;
                    break;
                }
            }
            
            if (!should_continue) break;
            
            if (current.next != -1) {
                read_node(current.next, current);
                current_id = current.next;
            } else {
                break;
            }
        }
        
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n;
    cin >> n;
    cin.ignore(); // Ignore newline after n
    
    BPlusTree tree;
    
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
            tree.insert(key, value);
        } else if (command == "delete") {
            string key;
            int value;
            iss >> key >> value;
            tree.erase(key, value);
        } else if (command == "find") {
            string key;
            iss >> key;
            
            vector<int> values = tree.find(key);
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