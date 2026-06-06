/*
 * AFCService.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef AFCSERVICE_H_
#define AFCSERVICE_H_

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/libimobiledevice.h>

namespace PP {

/** One entry from an AFC directory listing. */
struct AFCFileEntry {
    std::string name;
    bool isDirectory = false;
};

/**
 * @class AFCService
 * @brief Apple File Conduit service for file transfer operations
 *
 * AFC (Apple File Conduit) is a protocol used by iOS devices to allow
 * file system access. This class provides functionality to upload and
 * download files to/from iOS devices in normal mode.
 *
 * @note The device must be in normal mode and unlocked for this service to work.
 * @note All methods throw std::runtime_error on failure.
 */
class AFCService {
public:
    /**
     * @brief Construct an AFCService instance
     * @param udid The UDID (Unique Device Identifier) of the target device
     * @throws std::runtime_error if connection to the device fails
     */
    AFCService(const std::string& udid) {
        if (idevice_new_with_options(&device_, udid.c_str(), IDEVICE_LOOKUP_USBMUX) != IDEVICE_E_SUCCESS) {
            throw std::runtime_error("Failed to connect to device");
        }
        if (afc_client_start_service(device_, &afc_client_, "purplepois0n") != AFC_E_SUCCESS) {
            idevice_free(device_);
            device_ = nullptr;
            throw std::runtime_error("Failed to create AFC client");
        }
    }

    ~AFCService() {
        if (afc_client_ != nullptr) {
            afc_client_free(afc_client_);
        }
        if (device_ != nullptr) {
            idevice_free(device_);
        }
    }

    /**
     * @brief Upload a file from the local filesystem to the device
     * @param localPath Path to the local file to upload
     * @param remotePath Destination path on the device
     * @throws std::runtime_error if the upload fails
     */
    void uploadFile(const std::string& localPath, const std::string& remotePath) {
        uint64_t handle = 0;
        afc_error_t err = afc_file_open(afc_client_, remotePath.c_str(), AFC_FOPEN_WRONLY, &handle);
        if (err != AFC_E_SUCCESS) {
            throw std::runtime_error("Failed to open remote file for writing");
        }

        FILE* fp = fopen(localPath.c_str(), "rb");
        if (!fp) {
            afc_file_close(afc_client_, handle);
            throw std::runtime_error("Failed to open local file for reading");
        }

        const size_t BUF_SIZE = 64 * 1024;
        std::vector<char> buf(BUF_SIZE);
        while (true) {
            size_t read_size = fread(&buf[0], 1, BUF_SIZE, fp);
            if (read_size == 0) {
                break;
            }
            uint32_t bytes_written = 0;
            if (afc_file_write(afc_client_, handle, &buf[0],
                               static_cast<uint32_t>(read_size), &bytes_written) != AFC_E_SUCCESS ||
                bytes_written != read_size) {
                fclose(fp);
                afc_file_close(afc_client_, handle);
                throw std::runtime_error("Failed to write to remote file");
            }
        }

        afc_file_close(afc_client_, handle);
        fclose(fp);
    }

    /**
     * @brief Download a file from the device to the local filesystem
     * @param remotePath Path to the file on the device
     * @param localPath Destination path on the local filesystem
     * @throws std::runtime_error if the download fails
     */
    void downloadFile(const std::string& remotePath, const std::string& localPath) {
        uint64_t handle = 0;
        afc_error_t err = afc_file_open(afc_client_, remotePath.c_str(), AFC_FOPEN_RDONLY, &handle);
        if (err != AFC_E_SUCCESS) {
            throw std::runtime_error("Failed to open remote file for reading");
        }

        FILE* fp = fopen(localPath.c_str(), "wb");
        if (!fp) {
            afc_file_close(afc_client_, handle);
            throw std::runtime_error("Failed to open local file for writing");
        }

        const size_t BUF_SIZE = 64 * 1024;
        std::vector<char> buf(BUF_SIZE);
        while (true) {
            uint32_t read_size = 0;
            afc_error_t afc_err = afc_file_read(afc_client_, handle, &buf[0], BUF_SIZE, &read_size);
            if (afc_err != AFC_E_SUCCESS) {
                fclose(fp);
                afc_file_close(afc_client_, handle);
                throw std::runtime_error("Failed to read from remote file");
            }
            if (read_size == 0) {
                break;
            }
            fwrite(&buf[0], 1, read_size, fp);
        }

        fclose(fp);
        afc_file_close(afc_client_, handle);
    }

    /**
     * @brief List entries in a remote directory
     * @param remotePath Directory path on the device (e.g. "/" or "/Books")
     * @return Vector of file/directory names (skips "." and "..")
     * @throws std::runtime_error if the listing fails
     */
    std::vector<AFCFileEntry> listDirectory(const std::string& remotePath) {
        char** list = nullptr;
        const afc_error_t err = afc_read_directory(afc_client_, remotePath.c_str(), &list);
        if (err != AFC_E_SUCCESS || list == nullptr) {
            throw std::runtime_error("Failed to read AFC directory: " + remotePath);
        }

        std::vector<AFCFileEntry> entries;
        for (char** p = list; *p != nullptr; ++p) {
            const char* name = *p;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }
            AFCFileEntry entry;
            entry.name = name;
            if (name[0] != '\0' && name[strlen(name) - 1] == '/') {
                entry.isDirectory = true;
                if (!entry.name.empty()) {
                    entry.name.resize(entry.name.size() - 1);
                }
            } else {
                char** info = nullptr;
                std::string fullPath = remotePath;
                if (fullPath.empty() || fullPath[fullPath.size() - 1] != '/') {
                    fullPath += '/';
                }
                fullPath += entry.name;
                if (afc_get_file_info(afc_client_, fullPath.c_str(), &info) == AFC_E_SUCCESS &&
                    info != nullptr) {
                    for (char** q = info; *q != nullptr; q += 2) {
                        if (*q != nullptr && strcmp(*q, "st_ifmt") == 0 && *(q + 1) != nullptr &&
                            strcmp(*(q + 1), "S_IFDIR") == 0) {
                            entry.isDirectory = true;
                            break;
                        }
                    }
                    afc_dictionary_free(info);
                }
            }
            entries.push_back(entry);
        }
        afc_dictionary_free(list);
        return entries;
    }

private:
    idevice_t device_ = nullptr;
    afc_client_t afc_client_ = nullptr;
};

} /* namespace PP */

#endif /* AFCSERVICE_H_ */
