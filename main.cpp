#include <unordered_map>
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

namespace fs = std::filesystem;

struct CronChange {
    int         size_file;
    int         mode_file;
    int         id_userOwner;
    std::string timestamp;
    std::string pathToFile;
    bool operator==(const CronChange&other)
    {
        return size_file == other.size_file &&
                mode_file == other.mode_file &&
                id_userOwner == other.id_userOwner &&
                timestamp == other.timestamp &&
                pathToFile == other.pathToFile;
    }
};

std::string timeToString(std::time_t time) {
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
    return buffer;
}

void writeToFile(const std::string& filename, const std::unordered_map<std::string, CronChange>& persons) {
    std::ofstream outFile(filename, std::ios::binary);
    if (outFile) {
        for (const auto &person: persons)
        {
            outFile.write(reinterpret_cast<const char*>(&person.second.size_file), sizeof(int));
            outFile.write(reinterpret_cast<const char*>(&person.second.mode_file), sizeof(int));
            outFile.write(reinterpret_cast<const char*>(&person.second.id_userOwner), sizeof(int));
            size_t len = person.second.timestamp.length();
            outFile.write((char*) &len, sizeof(size_t));
            outFile.write(person.second.timestamp.data(), len);
            len = person.second.pathToFile.length();
            outFile.write((char*) &len, sizeof(size_t));
            outFile.write(person.second.pathToFile.data(), len);
        }
        outFile.close();
    } else {
        std::cerr << "Не удалось открыть файл для записи!\n";
    }
}

std::unordered_map<std::string, CronChange> readFromFile(const std::string& filename) {
    std::unordered_map<std::string, CronChange> persons;
    std::ifstream inFile(filename, std::ios::binary);
    if (inFile) {
        int sizeFile, modeFile,idUser;
        size_t lenStr;
        while(inFile.read(reinterpret_cast<char*>(&sizeFile), sizeof(int)))
        {
            CronChange person;
            person.size_file = sizeFile;
            inFile.read(reinterpret_cast<char*>(&modeFile), sizeof(int));
            person.mode_file = modeFile;
            inFile.read(reinterpret_cast<char*>(&idUser), sizeof(int));
            person.id_userOwner = idUser;
            inFile.read(reinterpret_cast<char*>(&lenStr), sizeof(size_t));
            char buf[lenStr+1];
            inFile.read(buf, lenStr);
            buf[lenStr] = 0;
            person.timestamp = buf;
            inFile.read(reinterpret_cast<char*>(&lenStr), sizeof(size_t));
            char buf2[lenStr+1];
            inFile.read(buf2, lenStr);
            buf2[lenStr] = 0;
            person.pathToFile = buf2;
            persons[buf2] = person;
        }
        inFile.close();
    }
    return persons;
}


int main(int argc, char *argv[])
{
    std::unordered_map<std::string, CronChange> currentChanges;
    for (const auto& entry : fs::directory_iterator("/var/spool/cron/crontabs"))
    {
        struct stat statFile;
        std::string pathFile = entry.path();
        stat(pathFile.c_str(),&statFile);
        CronChange cronElement;
        cronElement.size_file = statFile.st_size;
        cronElement.mode_file = statFile.st_mode & 0777;
        cronElement.id_userOwner = statFile.st_uid;
        cronElement.timestamp = timeToString(statFile.st_mtime);
        cronElement.pathToFile = pathFile;
        currentChanges[pathFile] = cronElement;
    }
    auto listState = readFromFile("state_cron.dat");
    //writeToFile("state_cron.dat",currentChanges);



    return 0;
}
