#include <iostream>
#include <filesystem>
#include <fstream>

#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>

#define OPENSSL_API_COMPAT 10100
#include <openssl/sha.h>

using namespace std;
using namespace std::filesystem;

void dirListener(vector<path>& paths)
{
  path directory = current_path();

  if(exists(directory) && is_directory(directory)) {
    for(const auto& entry: recursive_directory_iterator(directory)) {
      if(is_regular_file(entry)) {
        paths.push_back(entry.path());
      }
    }
  }
  else {
    cout << "Directory not found! " << '\n';
  }
}

string calculateSHA256(const path& filename) {
  ifstream file(filename, ios::binary);
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  
  SHA256_Init(&sha256);
  char buffer[4096];
  while(file.read(buffer ,sizeof(buffer))) {
    SHA256_Update(&sha256 ,buffer ,file.gcount());
  }
  SHA256_Update(&sha256, buffer, file.gcount());
  SHA256_Final(hash, &sha256);

  stringstream ss;
  for(size_t i = 0;i < SHA256_DIGEST_LENGTH; i++) {
    ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
  }
  return ss.str();
}

void replaceWithHardLinks(unordered_map<string,vector<path>>& hashmap) {
  for(const auto& [hash, files] : hashmap) {
    if(files.size() > 1) {
      const path& original =  files[0];
      for(size_t i = 1;i < files.size();i++) {
        const path& duplicate = files[i];
        remove(duplicate);
        create_hard_link(original,duplicate);
        cout << "Replaced with hard link: " << duplicate << '\n';
      }
    }
  }
}

int main(int argc, char **argv)
{
  vector<path> paths;
  unordered_map<string, vector<path>> hashmap;
  dirListener(paths);
  for (const auto& p: paths) {
    string hash = calculateSHA256(p);
    hashmap[hash].push_back(p);
    cout << "File: " << p << " Hash: " << hash << '\n';
  }
  replaceWithHardLinks(hashmap);

  return 0;
}
