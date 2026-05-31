#include <string>
#include <curl/curl.h>
#include <fstream>
#include <stdexcept>


namespace devkit {

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        std::ofstream* file = static_cast<std::ofstream*>(userp);
        size_t totalSize = size * nmemb;
        file->write(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    void DownloadFile(
        const std::string& url,
        const std::string& localPath,
        bool ignoreSslErrors = false,
        bool useSystemCaBundle = true) {

        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize libcurl");
        }

        std::ofstream outputFile(localPath, std::ios::binary);
        if (!outputFile.is_open()) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Failed to open local file for writing: " + localPath);
        }

        // Настройка URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Настройка callback для записи данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);

        // Следовать редиректам
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Таймаут соединения (30 секунд)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);

        // Общий таймаут (300 секунд)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

        // Пользовательский агент
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-downloader/1.0");

        // Настройка SSL
        if (ignoreSslErrors) {
            // Игнорировать проверку SSL-сертификата
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        } else {
            // Проверять SSL-сертификат
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

            if (useSystemCaBundle) {
                // На Windows libcurl использует системное хранилище сертификатов 
                // по умолчанию, если собрана с поддержкой Schannel или 
                // Secure Transport. Явно указываем использовать системные сертификаты.
                curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
            }
            // Если useSystemCaBundle == false, будет использоваться стандартный 
            // путь libcurl к CA bundle (или переменная окружения CURL_CA_BUNDLE)
        }

        // Включить подробный вывод для отладки (можно закомментировать)
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // Выполнение запроса
        CURLcode res = curl_easy_perform(curl);

        // Закрываем файл перед проверкой ошибок
        outputFile.close();

        if (res != CURLE_OK) {
            std::string errorMsg = "Failed to download file: ";
            errorMsg += curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            throw std::runtime_error(errorMsg);
        }

        // Проверка HTTP-кода ответа
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_easy_cleanup(curl);

        if (httpCode >= 400) {
            throw std::runtime_error("HTTP error: " + std::to_string(httpCode));
        }
    }
}