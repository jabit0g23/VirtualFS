#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <ctime>
#include <fstream>
#include <sstream>

// Estructura del i-nodo
struct INode {
    std::string name;
    bool isDirectory;
    size_t size;
    int permissions; // Representar permisos como un entero
    std::time_t creationTime;
    std::string content; // Solo para archivos
    std::vector<INode*> children; // Solo para directorios
    INode* parent; // Puntero al nodo padre

    INode(std::string n, bool dir, int perm, INode* p = nullptr)
        : name(std::move(n)), isDirectory(dir), size(0), permissions(perm), parent(p) {
        creationTime = std::time(nullptr);
    }

    ~INode() {
        for (auto child : children) {
            delete child;
        }
    }
};

// Clase para el sistema de archivos
class FileSystem {
private:
    INode* root;
    INode* currentDirectory;
    std::unordered_map<int, INode*> inodes; // Mapa para rastrear los i-nodos por su ID
    std::vector<std::string> commandHistory;
    int nextInodeID = 1;

    INode* findINodeByName(INode* directory, const std::string& name) {
        for (auto child : directory->children) {
            if (child->name == name) return child;
        }
        return nullptr;
    }

    void addCommandToHistory(const std::string& command) {
        commandHistory.push_back(command);
    }

    void run(size_t index) {
        if (index < commandHistory.size()) {
            std::cout << "Executing command from history: " << commandHistory[index] << std::endl;
            executeCommand(commandHistory[index]);
        } else {
            std::cout << "Invalid command index: " << index << std::endl;
        }
    }

    void executeCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "touch") {
            std::string name, content;
            iss >> name;
            std::getline(iss, content);
            touch(name, content);
        } else if (cmd == "mkdir") {
            std::string name;
            iss >> name;
            mkdir(name);
        } else if (cmd == "mv") {
            std::string oldName, newName;
            iss >> oldName >> newName;
            mv(oldName, newName);
        } else if (cmd == "rm") {
            std::string name;
            iss >> name;
            rm(name);
        } else if (cmd == "ls") {
            std::string option;
            iss >> option;
            if (option == "-R") {
                lsRecursive();
            } else {
                ls();
            }
        } else if (cmd == "cd") {
            std::string name;
            iss >> name;
            cd(name);
    } else if (cmd == "chmod") {
        std::string name, permissions;
        iss >> name >> permissions;
        chmod(name, permissions);
    } else if (cmd == "find") {
            std::string name;
            iss >> name;
            find(name);
        } else if (cmd == "inode") {
            inode();
        } else if (cmd == "history") {
            history();
        } else if (cmd == "save") {
            save();
        } else if (cmd == "load") {
            load();
        } else if (cmd == "execute") {
            std::string name;
            iss >> name;
            execute(name);
        } else if (cmd == "search") {
            std::string name;
            iss >> name;
            search(name);
        } else if (cmd == "read") {
            std::string name;
            iss >> name;
            read(name);
        } else if (cmd == "write") {
            std::string name, content;
            iss >> name;
            std::getline(iss, content);
            write(name, content);
        } else if (cmd == "run") {
            size_t index;
            iss >> index;
            run(index);
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }

    // Comprobar permisos
    bool hasReadPermission(INode* node) {
        return (node->permissions & 0400) != 0;
    }

    bool hasWritePermission(INode* node) {
        return (node->permissions & 0200) != 0;
    }

    bool hasExecutePermission(INode* node) {
        return (node->permissions & 0100) != 0;
    }

    void lsRecursive(INode* directory, int depth = 0) {
        for (auto child : directory->children) {
            std::string indent(depth * 2, ' ');
            std::cout << indent << (child->isDirectory ? "DIR " : "FILE ") << child->name << std::endl;
            if (child->isDirectory) {
                lsRecursive(child, depth + 1); // Llamada recursiva para subdirectorios
            }
        }
    }

    void searchRecursive(INode* directory, const std::string& name, std::vector<std::string>& results, std::string currentPath) {
        for (auto child : directory->children) {
            std::string fullPath = currentPath + "/" + child->name;
            if (child->name == name) {
                results.push_back(fullPath);
            }
            if (child->isDirectory) {
                searchRecursive(child, name, results, fullPath);
            }
        }
    }

public:
    FileSystem() {
        root = new INode("/", true, 755);
        currentDirectory = root;
        inodes[0] = root;
    }

    ~FileSystem() {
        delete root;
    }

    std::string getCurrentPath() {
        if (currentDirectory == root) {
            return "/";
        }
        std::string path;
        INode* node = currentDirectory;
        while (node != nullptr && node != root) {
            path = "/" + node->name + path;
            node = node->parent;
        }
        return path;
    }

    void touch(const std::string& name, const std::string& content) {
        INode* newFile = new INode(name, false, 644, currentDirectory);
        newFile->content = content;
        newFile->size = content.size();
        currentDirectory->children.push_back(newFile);
        inodes[nextInodeID++] = newFile;
        addCommandToHistory("touch " + name + " " + content);
    }

    void mkdir(const std::string& name) {
        INode* newDir = new INode(name, true, 755, currentDirectory);
        currentDirectory->children.push_back(newDir);
        inodes[nextInodeID++] = newDir;
        addCommandToHistory("mkdir " + name);
        std::cout << "Directory created: " << name << " in " << getCurrentPath() << std::endl;
    }

    void mv(const std::string& oldName, const std::string& newName) {
        INode* item = findINodeByName(currentDirectory, oldName);
        if (item) {
            item->name = newName;
            addCommandToHistory("mv " + oldName + " " + newName);
        } else {
            std::cout << "Item not found: " << oldName << std::endl;
        }
    }

    void rm(const std::string& name) {
        auto& children = currentDirectory->children;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if ((*it)->name == name) {
                delete *it;
                children.erase(it);
                addCommandToHistory("rm " + name);
                return;
            }
        }
        std::cout << "Item not found: " << name << std::endl;
    }

    void ls() {
        for (auto child : currentDirectory->children) {
            std::cout << (child->isDirectory ? "DIR " : "FILE ") << child->name << std::endl;
        }
        addCommandToHistory("ls");
    }

    void lsRecursive() {
        lsRecursive(currentDirectory);
    }

    void cd(const std::string& name) {
        if (name == "..") {
            if (currentDirectory->parent) {
                currentDirectory = currentDirectory->parent;
            }
        } else if (name == "/") {
            currentDirectory = root;
        } else {
            INode* target = findINodeByName(currentDirectory, name);
            if (target && target->isDirectory) {
                currentDirectory = target;
            } else {
                std::cout << "Directory not found: " << name << std::endl;
            }
        }
        addCommandToHistory("cd " + name);
    }

void chmod(const std::string& name, const std::string& permissions) {
    INode* item = findINodeByName(currentDirectory, name);
    if (item) {
        int newPermissions = std::stoi(permissions, nullptr, 8); // Convertir de cadena a entero octal
        item->permissions = newPermissions;
        std::cout << "Changed permissions to " << std::oct << newPermissions << " for '" << name << "'\n";
        std::cout << "Current permissions: " << std::oct << item->permissions << "\n";
        addCommandToHistory("chmod " + name + " " + permissions);
    } else {
        std::cout << "Item not found: " << name << std::endl;
    }
}

    void find(const std::string& name) {
        findRecursive(root, name);
        addCommandToHistory("find " + name);
    }

    void findRecursive(INode* directory, const std::string& name) {
        for (auto child : directory->children) {
            if (child->name == name) {
                std::cout << "Found: " << (child->isDirectory ? "DIR " : "FILE ") << child->name << std::endl;
            }
            if (child->isDirectory) {
                findRecursive(child, name);
            }
        }
    }

    void inode() {
        for (const auto& [id, inode] : inodes) {
            std::cout << "ID: " << id << ", Name: " << inode->name << ", Type: " 
                      << (inode->isDirectory ? "DIR" : "FILE") << std::endl;
        }
        addCommandToHistory("inode");
    }

    void history() {
        for (size_t i = 0; i < commandHistory.size(); ++i) {
            std::cout << i << ": " << commandHistory[i] << std::endl;
        }
    }

    void save() {
        std::ofstream file("C:\\Users\\jabit0g23\\Desktop\\tarea3\\filesystem.txt");
        if (!file) {
            std::cerr << "Error opening file for saving" << std::endl;
            return;
        }
        
        std::cout << "Saving filesystem to filesystem.txt" << std::endl;

        for (const auto& [id, inode] : inodes) {
            file << id << " " << inode->name << " " << inode->isDirectory << " "
                 << inode->size << " " << inode->permissions << " "
                 << inode->creationTime;
            if (!inode->isDirectory) {
                file << " " << inode->content;
            }
            file << std::endl;
        }
        
        file.close();
        if (file.fail()) {
            std::cerr << "Error closing file after saving" << std::endl;
        } else {
            std::cout << "Filesystem saved successfully" << std::endl;
        }
        addCommandToHistory("save");
    }

    void load() {
        std::ifstream file("C:\\Users\\jabit0g23\\Desktop\\tarea3\\filesystem.txt");
        if (!file) {
            std::cerr << "Error opening file for loading" << std::endl;
            return;
        }

        inodes.clear();
        delete root;
        root = new INode("/", true, 755);
        currentDirectory = root;
        inodes[0] = root;
        nextInodeID = 1;

        std::string name, content;
        bool isDirectory;
        size_t size;
        int permissions;
        std::time_t creationTime;
        int id;

        while (file >> id >> name >> isDirectory >> size >> permissions >> creationTime) {
            INode* node;
            if (isDirectory) {
                node = new INode(name, true, permissions);
            } else {
                file.ignore(); 
                std::getline(file, content);
                node = new INode(name, false, permissions);
                node->content = content;
                node->size = content.size();
            }
            node->creationTime = creationTime;
            inodes[id] = node;
            if (id != 0) {
                INode* parent = root; 
                std::string path = name;
                size_t pos;
                while ((pos = path.find('/')) != std::string::npos) {
                    std::string dir = path.substr(0, pos);
                    path.erase(0, pos + 1);
                    parent = findINodeByName(parent, dir);
                }
                parent->children.push_back(node);
                node->parent = parent;
            }
        }
        file.close();
        addCommandToHistory("load");
    }

    void search(const std::string& name) {
        std::vector<std::string> results;
        searchRecursive(root, name, results, "");
        if (results.empty()) {
            std::cout << "No files or directories found with the name: " << name << std::endl;
        } else {
            std::cout << "Found files or directories:\n";
            for (const auto& path : results) {
                std::cout << path << std::endl;
            }
        }
        addCommandToHistory("search " + name);
    }

void read(const std::string& name) {
    INode* item = findINodeByName(currentDirectory, name);
    if (item) {
        if (hasReadPermission(item)) {
            std::cout << "Reading file: " << item->content << std::endl;
        } else {
            std::cout << "Permission denied" << std::endl;
        }
    } else {
        std::cout << "File not found: " << name << std::endl;
    }
}

    void write(const std::string& name, const std::string& content) {
        INode* item = findINodeByName(currentDirectory, name);
        if (item) {
            if (hasWritePermission(item)) {
                item->content = content;
                item->size = content.size();
                addCommandToHistory("write " + name + " " + content);
            } else {
                std::cout << "Permission denied" << std::endl;
            }
        } else {
            std::cout << "File not found: " << name << std::endl;
        }
    }

    void execute(const std::string& name) {
        INode* item = findINodeByName(currentDirectory, name);
        if (item) {
            if (hasExecutePermission(item)) {
                std::cout << "Executing file: " << item->name << std::endl;
            } else {
                std::cout << "Permission denied" << std::endl;
            }
        } else {
            std::cout << "File not found: " << name << std::endl;
        }
    }

    void prompt() {
        while (true) {
            std::cout << getCurrentPath() << "$ ";
            std::string command;
            std::getline(std::cin, command);
            if (command == "exit") break;
            executeCommand(command);
        }
    }
};

int main() {
    FileSystem fs;
    fs.prompt();
    return 0;
}
