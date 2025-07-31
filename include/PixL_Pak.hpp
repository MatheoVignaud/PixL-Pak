#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define IS_BIG_ENDIAN
#else
#error "Endianness inconnue"
#endif

namespace PixL_Pak
{
#define PIXL_PAK_VERSION_MAJOR 1
#define PIXL_PAK_VERSION_MINOR 0
#define PIXL_PAK_VERSION_PATCH 0

#ifdef IS_BIG_ENDIAN

    inline uint32_t ToLittleEndian32(uint32_t value)
    {
        return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24);
    }

    inline uint16_t ToLittleEndian16(uint16_t value)
    {
        return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
    }

#define FromLittleEndian32(value) ToLittleEndian32(value)
#define FromLittleEndian16(value) ToLittleEndian16(value)

#else
#define ToLittleEndian32(value) (value)
#define ToLittleEndian16(value) (value)
#define FromLittleEndian32(value) (value)
#define FromLittleEndian16(value) (value)

#endif

    struct Version
    {
        uint8_t Major = PIXL_PAK_VERSION_MAJOR;
        uint8_t Minor = PIXL_PAK_VERSION_MINOR;
        uint8_t Patch = PIXL_PAK_VERSION_PATCH;

        bool operator==(const Version &other) const
        {
            return Major == other.Major && Minor == other.Minor && Patch == other.Patch;
        }

        bool operator!=(const Version &other) const
        {
            return !(*this == other);
        }

        bool operator<(const Version &other) const
        {
            if (Major != other.Major)
                return Major < other.Major;
            if (Minor != other.Minor)
                return Minor < other.Minor;
            return Patch < other.Patch;
        }

        bool operator<=(const Version &other) const
        {
            return *this < other || *this == other;
        }

        bool operator>(const Version &other) const
        {
            return !(*this <= other);
        }

        bool operator>=(const Version &other) const
        {
            return !(*this < other);
        }
    };

    struct FileEntry
    {
        std::string name;    // Name of the file
        uint32_t size = 0;   // Size of the file in bytes
        uint32_t offset = 0; // Offset in the pak file where the file data starts
    };

    struct Context
    {
        std::string pakFileName;

        char magic[9] = "PIXL_PAK";
        Version version;
        std::vector<FileEntry> files;
        uint32_t data_offset;

        std::ifstream pakFile;

        bool extract(std::string outputDir = ".")
        {

            if (files.empty())
            {
                std::cerr << "No files to extract." << std::endl;
                return false;
            }

            if (outputDir.empty())
            {
                std::cerr << "Error: Output directory is empty." << std::endl;
                return false;
            }

            if (outputDir.back() != '/')
            {
                outputDir += '/';
            }

            for (const auto &file : files)
            {
                std::string outputPath = outputDir + file.name;
                std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

                pakFile.seekg(file.offset + data_offset, std::ios::beg);
                std::vector<char> buffer(file.size);
                pakFile.read(buffer.data(), file.size);

                if (!pakFile)
                {
                    std::cerr << "Error reading file '" << file.name << "' from pak." << std::endl;
                    return false;
                }

                std::ofstream outFile(outputPath, std::ios::binary);
                outFile.write(buffer.data(), file.size);
                outFile.close();
            }

            return true;
        };

        std::vector<char> readFile(const std::string &fileName)
        {
            for (const auto &file : files)
            {
                if (file.name == fileName)
                {
                    std::vector<char> buffer(file.size);
                    pakFile.seekg(file.offset + data_offset, std::ios::beg);
                    pakFile.read(buffer.data(), file.size);
                    return buffer;
                }
            }
            std::cerr << "File '" << fileName << "' not found in pak." << std::endl;
            return {};
        }

        ~Context()
        {
            if (pakFile.is_open())
            {
                pakFile.close();
            }
        }
    };

    inline std::vector<std::string> ExploreFolder(std::string folderPath)
    {
        std::vector<std::string> fileList;

        if (!folderPath.empty() && folderPath.back() != '/')
        {
            folderPath += '/';
        }

        try
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    fileList.push_back(entry.path().string());
                }
            }
        }
        catch (const std::filesystem::filesystem_error &ex)
        {
            std::cerr << "Error exploring folder '" << folderPath << "': " << ex.what() << std::endl;
        }

        return fileList;
    }

    inline uint8_t Pak(std::string sourceDir, std::string pakFileName, bool map_file = false)
    {
        Context context;
        context.pakFileName = pakFileName;

        if (sourceDir.empty())
        {
            std::cerr << "Error: Source directory is empty." << std::endl;
            return 1;
        }

        if (sourceDir.back() != '/')
        {
            sourceDir = sourceDir + '/';
        }

        std::vector<std::string> files = ExploreFolder(sourceDir);

        uint32_t offset = 0;

        for (const auto &filePath : files)
        {
            size_t index = filePath.find_first_of(sourceDir);
            if (index != std::string::npos)
            {
                std::string relativePath = filePath.substr(index + sourceDir.length());
                FileEntry entry;
                entry.name = relativePath;
                entry.size = static_cast<uint32_t>(std::filesystem::file_size(filePath));
                entry.offset = offset;
                offset += entry.size;
                context.files.push_back(entry);
            }
            else
            {
                index = filePath.find_first_of(sourceDir + "/");
                if (index != std::string::npos)
                {
                    std::string relativePath = filePath.substr(index + sourceDir.length() + 1);
                    FileEntry entry;
                    entry.name = relativePath;
                    entry.size = static_cast<uint32_t>(std::filesystem::file_size(filePath));
                    entry.offset = offset;
                    offset += entry.size;
                    context.files.push_back(entry);
                }
                else
                {
                    std::cerr << "Error: File '" << filePath << "' is not in the source directory." << std::endl;
                    return 1;
                }
            }
        }

        std::ofstream pak(pakFileName, std::ios::binary);

        if (!pak)
        {
            std::cerr << "Error creating pak file '" << pakFileName << "'." << std::endl;
            return 1;
        }

        uint32_t data_offset = 0;

        pak.write(context.magic, sizeof(context.magic));

        pak.write(reinterpret_cast<const char *>(&context.version), sizeof(context.version));

        uint32_t data_offset_le = ToLittleEndian32(data_offset);
        pak.write(reinterpret_cast<const char *>(&data_offset_le), sizeof(data_offset_le));

        uint32_t fileCount = static_cast<uint32_t>(context.files.size());
        uint32_t fileCount_le = ToLittleEndian32(fileCount);
        pak.write(reinterpret_cast<const char *>(&fileCount_le), sizeof(fileCount_le));

        for (const auto &file : context.files)
        {
            pak.write(file.name.c_str(), file.name.size() + 1);
            uint32_t size_le = ToLittleEndian32(file.size);
            uint32_t offset_le = ToLittleEndian32(file.offset);
            pak.write(reinterpret_cast<const char *>(&size_le), sizeof(size_le));
            pak.write(reinterpret_cast<const char *>(&offset_le), sizeof(offset_le));
        }

        data_offset = pak.tellp();
        pak.seekp(12);
        uint32_t data_offset_final_le = ToLittleEndian32(data_offset);
        pak.write(reinterpret_cast<const char *>(&data_offset_final_le), sizeof(data_offset_final_le));

        uint32_t filesize = pak.tellp();
        pak.seekp(0, std::ios::end);

        for (const auto &file : context.files)
        {
            std::ifstream inputFile(sourceDir + file.name, std::ios::binary);
            if (!inputFile)
            {
                std::cerr << "Error opening file '" << file.name << "' for reading." << std::endl;
                return 1;
            }
            pak << inputFile.rdbuf();
        }

        pak.close();

        size_t pakFileSize = std::filesystem::file_size(pakFileName);

        if (map_file)
        {
            std::ofstream mapFile(pakFileName + ".map", std::ios::out | std::ios::trunc);
            if (!mapFile)
            {
                std::cerr << "Error creating map file '" << pakFileName << ".map'." << std::endl;
                return 1;
            }
            float filesizeMB = static_cast<float>(pakFileSize) / (1024 * 1024);
            mapFile << "Pak file: " << pakFileName << std::endl;
            mapFile << "Size: " << std::fixed << std::setprecision(6) << filesizeMB << " MB" << std::endl;
            mapFile << "Header: " << std::endl;
            mapFile << "Magic: " << context.magic << std::endl;
            mapFile << "Version: " << static_cast<int>(context.version.Major) << "."
                    << static_cast<int>(context.version.Minor) << "."
                    << static_cast<int>(context.version.Patch) << std::endl;
            mapFile << "Data Offset: " << data_offset << std::endl;
            mapFile << "File Count: " << fileCount << std::endl;
            mapFile << "Files:" << std::endl;
            for (const auto &file : context.files)
            {
                mapFile << "  Name: " << file.name << ", Size: " << file.size
                        << ", Offset: " << file.offset << std::endl;
            }
            mapFile.close();
        }

        return 0;
    }

    inline Context *Open(std::string pakFileName)
    {
        Context *context = new Context();
        context->pakFileName = pakFileName;

        std::ifstream pak(pakFileName, std::ios::binary);
        if (!pak)
        {
            std::cerr << "Error opening pak file '" << pakFileName << "'." << std::endl;
            delete context;
            return nullptr;
        }

        char magic_buffer[9];
        pak.read(magic_buffer, 9);
        if (!pak || pak.gcount() != 9)
        {
            std::cerr << "Error reading magic number from pak file '" << pakFileName << "'." << std::endl;
            delete context;
            return nullptr;
        }

        if (magic_buffer[8] != '\0')
        {
            std::cerr << "Invalid pak file format. Magic number not properly null-terminated." << std::endl;
            return nullptr;
        }

        if (std::string(magic_buffer) != "PIXL_PAK")
        {
            std::cerr << "Invalid pak file format. Expected 'PIXL_PAK', got '" << magic_buffer << "'." << std::endl;
            delete context;
            return nullptr;
        }

        std::memcpy(context->magic, magic_buffer, 9);

        pak.read(reinterpret_cast<char *>(&context->version), sizeof(context->version));
        if (!pak || pak.gcount() != sizeof(context->version))
        {
            std::cerr << "Error reading version from pak file '" << pakFileName << "'." << std::endl;
            delete context;
            return nullptr;
        }

        if (context->version < Version{PIXL_PAK_VERSION_MAJOR, PIXL_PAK_VERSION_MINOR, 0})
        {
            std::cerr << "Unsupported pak file version: " << static_cast<int>(context->version.Major) << "."
                      << static_cast<int>(context->version.Minor) << "."
                      << static_cast<int>(context->version.Patch) << std::endl;
            delete context;
            return nullptr;
        }

        uint32_t data_offset = 0;
        pak.read(reinterpret_cast<char *>(&data_offset), sizeof(data_offset));
        if (!pak || pak.gcount() != sizeof(data_offset))
        {
            std::cerr << "Error reading data offset from pak file '" << pakFileName << "'." << std::endl;
            delete context;
            return nullptr;
        }
        data_offset = FromLittleEndian32(data_offset);

        context->data_offset = data_offset;

        uint32_t fileCount = 0;
        pak.read(reinterpret_cast<char *>(&fileCount), sizeof(fileCount));
        if (!pak || pak.gcount() != sizeof(fileCount))
        {
            std::cerr << "Error reading file count from pak file '" << pakFileName << "'." << std::endl;
            delete context;
            return nullptr;
        }
        fileCount = FromLittleEndian32(fileCount);

        if (fileCount > 1000000)
        {
            std::cerr << "File count too large: " << fileCount << ". Possible corruption." << std::endl;
            delete context;
            return nullptr;
        }

        for (uint32_t i = 0; i < fileCount; ++i)
        {
            FileEntry entry;

            std::string filename;
            char c;
            while (pak.read(&c, 1) && c != '\0')
            {
                filename += c;
                if (filename.length() > 1024)
                {
                    std::cerr << "Filename too long in file entry " << i << ". Possible corruption." << std::endl;
                    delete context;
                    return nullptr;
                }
            }

            if (!pak)
            {
                std::cerr << "Error reading filename for file entry " << i << std::endl;
                delete context;
                return nullptr;
            }

            entry.name = filename;

            pak.read(reinterpret_cast<char *>(&entry.size), sizeof(entry.size));
            if (!pak || pak.gcount() != sizeof(entry.size))
            {
                std::cerr << "Error reading size for file entry " << i << " (" << entry.name << ")" << std::endl;
                delete context;
                return nullptr;
            }
            entry.size = FromLittleEndian32(entry.size);

            pak.read(reinterpret_cast<char *>(&entry.offset), sizeof(entry.offset));
            if (!pak || pak.gcount() != sizeof(entry.offset))
            {
                std::cerr << "Error reading offset for file entry " << i << " (" << entry.name << ")" << std::endl;
                delete context;
                return nullptr;
            }
            entry.offset = FromLittleEndian32(entry.offset);

            context->files.push_back(entry);
        }

        pak.close();

        context->pakFile.open(pakFileName, std::ios::binary);
        if (!context->pakFile)
        {
            std::cerr << "Error reopening pak file '" << pakFileName << "' for reading." << std::endl;
            delete context;
            return nullptr;
        }

        return context;
    }
}

#ifdef IS_BIG_ENDIAN
#undef IS_BIG_ENDIAN
#endif