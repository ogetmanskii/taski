#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <exception>

#include "../logging/console_logging.hpp"
#include "util.hpp"
#include "zlib.h"
#include "minizip/unzip.h"

namespace fs = std::filesystem;

namespace devkit {

    void ExtractZipFile(const std::string& zipFilePath, const std::string& destDir) {
        // Открываем zip-файл
        unzFile zipFile = unzOpen(zipFilePath.c_str());
        if (!zipFile) {
            throw std::runtime_error("Could not open file: " + zipFilePath);
        }

        // Получаем глобальную информацию об архиве
        unz_global_info globalInfo;
        if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK) {
            unzClose(zipFile);
            throw std::runtime_error("Could not read archive: " + zipFilePath);
        }

        // Создаем целевую директорию, если её нет
        try {
            fs::create_directories(destDir);
        } catch (const fs::filesystem_error& e) {
            unzClose(zipFile);
            throw e;
        }

        std::vector<char> buffer(8192); // Буфер для чтения данных
        bool success = true;

        // Перебираем все файлы в архиве
        for (uLong i = 0; i < globalInfo.number_entry; ++i) {
            // Получаем информацию о текущем файле
            unz_file_info fileInfo;
            char fileName[256];

            if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName,
                sizeof(fileName), nullptr, 0, nullptr, 0) != UNZ_OK) {
                info("-- Error: could not read file {} in archive file", i);
                success = false;
                break;
            }

            std::string fullPath = destDir + "/" + fileName;

            // Проверяем, является ли запись директорией
            if (fileName[strlen(fileName) - 1] == '/') {
                // Создаем директорию
                try {
                    fs::create_directories(fullPath);
                } catch (const fs::filesystem_error& e) {
                    info("-- Error: could not create directory: {}: ", fullPath, e.what());
                    success = false;
                    break;
                }
            } else {
                // Создаем родительские директории для файла
                try {
                    fs::create_directories(fs::path(fullPath).parent_path());
                } catch (const fs::filesystem_error& e) {
                    info("-- Error: could not create parent folder: {}: {}", fullPath, e.what());
                    success = false;
                    break;
                }

                // Открываем текущий файл в архиве
                if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
                    info("-- Error: could not open file inside archive: {}", fullPath);
                    success = false;
                    break;
                }

                // Создаем выходной файл
                std::ofstream outFile(fullPath, std::ios::binary);
                if (!outFile.is_open()) {
                    info("-- Error: could not create file: {}", fullPath);
                    unzCloseCurrentFile(zipFile);
                    success = false;
                    break;
                }

                // Читаем и записываем данные
                int bytesRead;
                while ((bytesRead = unzReadCurrentFile(zipFile, buffer.data(),
                    buffer.size())) > 0) {
                    outFile.write(buffer.data(), bytesRead);
                    if (!outFile) {
                        info("-- Error: could not write to file: {}", fullPath);
                        success = false;
                        break;
                    }
                }

                outFile.close();
                unzCloseCurrentFile(zipFile);

                if (bytesRead < 0) {
                    info("-- Error: could not read archive file: {}", fileName);
                    success = false;
                    break;
                }

                if (!success) break;
            }

            // Переходим к следующему файлу в архиве
            if (i < globalInfo.number_entry - 1) {
                if (unzGoToNextFile(zipFile) != UNZ_OK) {
                    info("-- Error: could not go to next file in arhive. Corrupt archive file?");
                    success = false;
                    break;
                }
            }
        }

        unzClose(zipFile);
        if (!success) {
            throw std::runtime_error("Could not extract zip file: " + zipFilePath);
        }
    }
}