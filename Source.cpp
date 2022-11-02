/*
    DPAK Archive Extractor/Packer by Joseph Lee
*/

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include "miniz.h"
using namespace std;

void Compression(std::string Filename, bool bDecompress)
{
    std::fstream::pos_type size = 0;
    if (bDecompress)
    {
        if (std::ifstream fileSize{ Filename, std::ios::binary | std::ios::in | std::ios::ate })
        {
            size = fileSize.tellg();
        }
        if (std::ifstream file{ Filename, std::ios::binary })
        {
            std::vector<uint8_t> fileBuffer((unsigned int)size);
            file.read((char*)fileBuffer.data(), fileBuffer.size());

            mz_ulong decompressedSizeULong = mz_ulong(size * 1032); //This is the upper byte limit
            vector<uint8_t> decompressedBuffer(decompressedSizeULong); //This is the upper byte limit

            int zerror = uncompress(decompressedBuffer.data(), &decompressedSizeULong, fileBuffer.data(), (mz_ulong)size);
            if (zerror != Z_OK)
            {
                cout << "An error has occured uncompressing DPAK (error code: " << zerror << ")\n";
            }
            std::ofstream NewFile(Filename + "_Uncompressed", std::ios::out | std::ofstream::binary);
            NewFile.write((char*)&decompressedBuffer[0], decompressedSizeULong);
        }
    }
    else
    {
        if (std::ifstream fileSize{ Filename, std::ios::binary | std::ios::in | std::ios::ate })
        {
            size = fileSize.tellg();
        }
        if (std::ifstream file{ Filename, std::ios::binary })
        {
            std::vector<uint8_t> fileBuffer((unsigned int)size);
            file.read((char*)fileBuffer.data(), fileBuffer.size());


            vector<uint8_t> compressedBuffer((unsigned int)size);
            mz_ulong compressedSizeLong = (mz_ulong)size;
            compress2(compressedBuffer.data(), &compressedSizeLong, fileBuffer.data(), (mz_ulong)size, 10);

            std::ofstream NewFile(Filename + "_Compressed", std::ios::out | std::ofstream::binary);
            NewFile.write((char*)&compressedBuffer[0], compressedSizeLong);
        }
    }
}

void ExtractDPAK(std::string Filename)
{
    if (std::ifstream file{ Filename, std::ios::binary })
    {
        uint32_t Magic;
        file.read(reinterpret_cast<char*>(&Magic), 4);
#if !NDEBUG
        cout << "Magic: " << Magic << "\n";
#endif
        if (Magic == 1146110283 || Magic == 1262571588) //DPAK (Both Big and Little Endian)
        {
            std::vector<int> AssetSizes;
            std::vector<string> AssetIDs;

            uint16_t AssetCount;
            file.read(reinterpret_cast<char*>(&AssetCount), sizeof(AssetCount));
#if !NDEBUG
            cout << "Asset Count: " << AssetCount << "\n";
#endif
            for (int i = 0; i < AssetCount; i++)
            {
                uint32_t AssetSize;
                file.read(reinterpret_cast<char*>(&AssetSize), sizeof(AssetSize));
#if !NDEBUG
                cout << "Asset Size: " << AssetSize << "\n";
#endif
                AssetSizes.push_back(AssetSize);
            }
            for (int i = 0; i < AssetCount; i++)
            {
                uint16_t IDLength;
                file.read(reinterpret_cast<char*>(&IDLength), sizeof(IDLength));
#if !NDEBUG
                cout << "Asset Name Size: " << IDLength << "\n";
#endif

                std::vector<char> buffer(IDLength);
                file.read(buffer.data(), buffer.size());
                std::string s(buffer.begin(), buffer.end());
#if !NDEBUG
                cout << "Asset Name: " << s << "\n";
#endif
                AssetIDs.push_back(s);
            }

            std::string base_filename = Filename.substr(Filename.find_last_of("/\\") + 1);
            std::string::size_type const p(base_filename.find_last_of('.'));
            std::string file_without_extension = base_filename.substr(0, p);
#if !NDEBUG
            cout << "FileName: " << base_filename;
#endif

            string directory;
            const size_t last_slash_idx = Filename.rfind('\\');
            if (std::string::npos != last_slash_idx)
            {
                directory = Filename.substr(0, last_slash_idx);
            }
            directory += "\\";
            directory += file_without_extension;
            directory += "_extracted";
            for (int i = 0; i < AssetCount; i++)
            {
                string NewFilePath = directory + "\\" + AssetIDs[i];
                std::replace(NewFilePath.begin(), NewFilePath.end(), '/', '\\');
                string SubDirectory;
                const size_t last_slash_idx_Sub = NewFilePath.rfind('\\');
                if (std::string::npos != last_slash_idx_Sub)
                {
                    SubDirectory = NewFilePath.substr(0, last_slash_idx_Sub);
                }
                std::filesystem::create_directories(SubDirectory);

                std::ofstream NewFile(directory + "\\" + AssetIDs[i], std::ios::out | std::ofstream::binary);
                std::vector<char> buffer(AssetSizes[i]);
                file.read(buffer.data(), buffer.size());
                NewFile.write(buffer.data(), buffer.size());
                NewFile.close();
            }
        }
    }
}
void PackageDPAK(std::string Folder)
{
    std::filesystem::path folderPath(Folder);
    std::string fileName = folderPath.filename().string();

    ofstream createdFile(Folder + "\\..\\" + fileName + ".dpak", ios::out | ios::binary);
    uint16_t AssetCount = 0;
    std::vector<int> AssetSizes;
    std::vector<string> AssetIDs;
    std::vector<string> FileNames;
    for (const auto& entry : filesystem::recursive_directory_iterator(Folder))
    {
        AssetCount++;
        std::fstream::pos_type size = 0;
        if (std::ifstream fileSize{ entry, std::ios::binary | std::ios::in | std::ios::ate })
        {
            size = fileSize.tellg();
            fileSize.close();
        }
        AssetSizes.push_back((int)size);
        FileNames.push_back(entry.path().filename().string());
    }
    createdFile.write("KAPD", 4);
    createdFile.write(reinterpret_cast<char*>(&AssetCount), sizeof(AssetCount));
    for (int i = 0; i < AssetCount; i++)
    {
        createdFile.write(reinterpret_cast<char*>(&AssetSizes[i]), sizeof(AssetSizes[i]));
    }
    for (int i = 0; i < AssetCount; i++)
    {
        uint16_t Name = (uint16_t)FileNames[i].length();
        createdFile.write((char*)&Name, sizeof(Name));
        createdFile.write(FileNames[i].c_str(), FileNames[i].length());
    }
    for (int i = 0; i < AssetCount; i++)
    {
        if (std::ifstream fileSize{ Folder + "\\" + FileNames[i], std::ios::binary})
        {
#if !NDEBUG
            cout << Folder + "\\" + FileNames[i] << "\n";
#endif
            std::vector<uint8_t> buffer(AssetSizes[i]);
            fileSize.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            fileSize.close();
            createdFile.write((char*)&buffer[0], buffer.size());
        }
    }
    createdFile.close();
    Compression(Folder + "\\..\\" + fileName + ".dpak", false);
}

void Handler(std::string Filename, bool bDecompile)
{
    if (bDecompile)
    {
        if (std::ifstream file{ Filename, std::ios::binary })
        {
            uint32_t Magic;
            file.read(reinterpret_cast<char*>(&Magic), 4);
#if !NDEBUG
            cout << "Magic: " << Magic << "\n";
#endif
            if (Magic == 1146110283 || Magic == 1262571588) //DPAK (Both Big and Little Endian)
            {
                ExtractDPAK(Filename);
            }
            else if (Magic == 1836597052 || Magic == 1010792557) //XML File
            {
                Handler(Filename, false);
            }
            else if(uint8_t(Magic) == 120 || uint16_t(Magic) == 40056 || uint16_t(Magic) == 30876) //First is DPAK compression, Last two are XMC compression
            {
                Compression(Filename, true);
                ExtractDPAK(Filename + "_Uncompressed");
            }
            else
            {
                cout << "Unsupported filetype!" << "\n";
            }
        }
    }
    else
    {
        PackageDPAK(Filename);
    }
}

int main(int argc, char** argv)
{
    bool bIsDirectory = false;
    struct stat s;
    if (stat(argv[1], &s) == 0)
    {
        if (s.st_mode & S_IFDIR)
        {
            bIsDirectory = true;
        }
    }
    string fn = argv[1];
#if !NDEBUG
    cout << fn << "\n";
#endif
    if (bIsDirectory)
    {
        Handler(argv[1], false);
    }
    else
    {
        Handler(argv[1], true);
    }
    system("pause");
    return 0;
}