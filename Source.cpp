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

void Compression(string Filename, bool bDecompress)
{
    fstream::pos_type size = 0;
    if (ifstream fileSize{ Filename, ios::binary | ios::in | ios::ate })
    {
        size = fileSize.tellg();
    }
    if (bDecompress)
    {
        if (ifstream file{ Filename, ios::binary })
        {
            vector<uint8_t> fileBuffer((unsigned int)size);
            file.read((char*)fileBuffer.data(), fileBuffer.size());

            mz_ulong decompressedSizeULong = mz_ulong(size * 1032); //This is the upper byte limit
            vector<uint8_t> decompressedBuffer(decompressedSizeULong); //This is the upper byte limit

            int zerror = uncompress(decompressedBuffer.data(), &decompressedSizeULong, fileBuffer.data(), (mz_ulong)size);
            if (zerror != Z_OK)
            {
                cout << "An error has occured uncompressing DPAK (error code: " << zerror << ")\n";
            }
            ofstream NewFile(Filename + "_Uncompressed", ios::out | ofstream::binary);
            NewFile.write((char*)&decompressedBuffer[0], decompressedSizeULong);
        }
    }
    else
    {
        if (ifstream file{ Filename, ios::binary })
        {
            vector<uint8_t> fileBuffer((unsigned int)size);
            file.read((char*)fileBuffer.data(), fileBuffer.size());

            vector<uint8_t> compressedBuffer((unsigned int)size);
            mz_ulong compressedSizeLong = (mz_ulong)size;
            compress(compressedBuffer.data(), &compressedSizeLong, fileBuffer.data(), (mz_ulong)size);

            ofstream NewFile(Filename + "_Compressed", ios::out | ofstream::binary);
            NewFile.write((char*)&compressedBuffer[0], compressedSizeLong);
        }
    }
}

void ExtractDPAK(string Filename)
{
    if (ifstream file{ Filename, ios::binary })
    {
        uint32_t Magic;
        file.read(reinterpret_cast<char*>(&Magic), 4);
#if !NDEBUG
        cout << "Magic: " << Magic << "\n";
#endif
        if (Magic == 1146110283 || Magic == 1262571588) //DPAK (Both Big and Little Endian)
        {
            vector<int> AssetSizes;
            vector<string> AssetIDs;

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

                vector<char> buffer(IDLength);
                file.read(buffer.data(), buffer.size());
                string s(buffer.begin(), buffer.end());
#if !NDEBUG
                cout << "Asset Name: " << s << "\n";
#endif
                AssetIDs.push_back(s);
            }

            string base_filename = Filename.substr(Filename.find_last_of("/\\") + 1);
            string::size_type const p(base_filename.find_last_of('.'));
            string file_without_extension = base_filename.substr(0, p);
#if !NDEBUG
            cout << "FileName: " << base_filename;
#endif

            string directory;
            const size_t last_slash_idx = Filename.rfind('\\');
            if (string::npos != last_slash_idx)
            {
                directory = Filename.substr(0, last_slash_idx);
            }
            directory += "\\";
            directory += file_without_extension;
            directory += "_extracted";
            for (int i = 0; i < AssetCount; i++)
            {
                string NewFilePath = directory + "\\" + AssetIDs[i];
                replace(NewFilePath.begin(), NewFilePath.end(), '/', '\\');
                string SubDirectory;
                const size_t last_slash_idx_Sub = NewFilePath.rfind('\\');
                if (string::npos != last_slash_idx_Sub)
                {
                    SubDirectory = NewFilePath.substr(0, last_slash_idx_Sub);
                }
                filesystem::create_directories(SubDirectory);

                ofstream NewFile(directory + "\\" + AssetIDs[i], ios::out | ofstream::binary);
                vector<char> buffer(AssetSizes[i]);
                file.read(buffer.data(), buffer.size());
                NewFile.write(buffer.data(), buffer.size());
                NewFile.close();
            }
        }
    }
}

void PackageDPAK(string Folder)
{
    filesystem::path folderPath(Folder);
    string fileName = folderPath.filename().string();

    ofstream createdFile(Folder + "\\..\\" + fileName + ".dpak", ios::out | ios::binary);
    uint16_t AssetCount = 0;
    vector<int> AssetSizes;
    vector<string> AssetIDs;
    vector<string> FileNames;
    for (const auto& entry : filesystem::recursive_directory_iterator(Folder))
    {
        AssetCount++;
        fstream::pos_type size = 0;
        if (ifstream fileSize{ entry, ios::binary | ios::in | ios::ate })
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
        if (ifstream fileSize{ Folder + "\\" + FileNames[i], ios::binary})
        {
#if !NDEBUG
            cout << Folder + "\\" + FileNames[i] << "\n";
#endif
            vector<uint8_t> buffer(AssetSizes[i]);
            fileSize.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            fileSize.close();
            createdFile.write((char*)&buffer[0], buffer.size());
        }
    }
    createdFile.close();
    Compression(Folder + "\\..\\" + fileName + ".dpak", false);
}

void Handler(string Filename, bool bDecompile)
{
    if (bDecompile)
    {
        if (ifstream file{ Filename, ios::binary })
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
                Compression(Filename, false);
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
    Handler(argv[1], !filesystem::is_directory(argv[1]));
    system("pause");
    return 0;
}